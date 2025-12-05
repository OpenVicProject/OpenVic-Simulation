#include <filesystem>

#include <openvic-dataloader/detail/CallbackOStream.hpp>

#include <lexy-vdf/KeyValues.hpp>
#include <lexy-vdf/Parser.hpp>

#include <fmt/std.h>

#include <spdlog/common.h>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/ConstexprIntToStr.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Logger.hpp"

#include "Dataloader.hpp"

#ifdef _WIN32
#include <Windows.h>

#include "Vic2PathSearch_Windows.hpp"
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#endif

using namespace OpenVic;
using namespace ovdl;

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
#define FILESYSTEM_CASE_INSENSITIVE
#endif

#if !defined(_WIN32)
#define FILESYSTEM_NEEDS_FORWARD_SLASHES
#endif

// Windows and Mac by default act like case insensitive filesystems
static constexpr bool path_equals(std::string_view lhs, std::string_view rhs) {
#if defined(FILESYSTEM_CASE_INSENSITIVE)
	return ascii_equal_case_insensitive(lhs, rhs);
#else
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
#endif
}

template<typename T>
concept is_filename = std::same_as<T, std::filesystem::path> || std::convertible_to<T, std::string_view>;

static bool filename_equals(is_filename auto const& lhs, is_filename auto const& rhs) {
	auto left = [&lhs] {
		if constexpr (std::same_as<std::decay_t<decltype(lhs)>, std::filesystem::path>) {
			return lhs.filename().string();
		} else {
			return lhs;
		}
	}();
	auto right = [&rhs] {
		if constexpr (std::same_as<std::decay_t<decltype(rhs)>, std::filesystem::path>) {
			return rhs.filename().string();
		} else {
			return rhs;
		}
	}();
	return path_equals(left, right);
}

static fs::path _search_for_game_path(fs::path hint_path = {});

template<typename... Args>
struct search_log {
	bool _warn;

	search_log(
		bool warn, spdlog::format_string_t<Args...> fmt, Args&&... args,
		std::source_location const& location = std::source_location::current()
	) {
		_warn = warn;
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
		spdlog::memory_buf_t buf;
		fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));

		spdlog::default_logger_raw()->log(
			spdlog::source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() },
			warn ? spdlog::level::warn : spdlog::level::info, spdlog::string_view_t(buf.data(), buf.size())
		);
#else
		if (warn) {
			spdlog::memory_buf_t buf;
			fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));

			spdlog::default_logger_raw()->log(
				spdlog::source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() },
				spdlog::level::warn, spdlog::string_view_t(buf.data(), buf.size())
			);
		}
#endif
	}

	fs::path result() const {
		if (!_warn) {
			return _search_for_game_path();
		}
		return {};
	}
};

template<typename... Args>
search_log(bool warn, spdlog::format_string_t<Args...> fmt, Args&&... args) -> search_log<Args...>;

static fs::path _search_for_game_path(fs::path hint_path) {
	// Apparently max amount of steam libraries is 8, if incorrect please correct it to the correct max amount
	static constexpr int max_amount_of_steam_libraries = 8;
	static constexpr std::string_view Victoria_2_folder = "Victoria 2";
	static constexpr std::string_view v2_game_exe = "v2game.exe";
	static constexpr std::string_view steamapps = "steamapps";
	static constexpr std::string_view libraryfolders = "libraryfolders.vdf";
	static constexpr std::string_view vic2_appmanifest = "appmanifest_42960.acf";
	static constexpr std::string_view common_folder = "common";

	std::error_code error_code;

	// Don't waste time trying to search for Victoria 2 when supplied a valid looking Victoria 2 game directory
	if (fs::is_regular_file(hint_path / v2_game_exe, error_code)) {
		return hint_path;
	}

	const bool hint_empty = hint_path.empty();

	if (hint_empty) {
#if defined(_WIN32)
		static const fs::path registry_path =
			Windows::ReadRegValue<char>(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Paradox Interactive\\Victoria 2", "path");

		if (!registry_path.empty()) {
			return registry_path;
		}

#pragma warning(push)
#pragma warning(disable : 4996)
		static const fs::path prog_files = memory::string(std::getenv("ProgramFiles"));
		hint_path = prog_files / "Steam";
		if (!fs::is_directory(hint_path, error_code)) {
			static const fs::path prog_files_x86 = memory::string(std::getenv("ProgramFiles(x86)"));
			hint_path = prog_files_x86 / "Steam";
			if (!fs::is_directory(hint_path, error_code)) {
				spdlog::warn_s("Could not find path for Steam installation on Windows.");
				return {};
			}
		}
#pragma warning(pop)
		// Cannot support Android
		// Only FreeBSD currently unofficially supports emulating Linux
#elif (defined(__linux__) && !defined(__ANDROID__)) || defined(__FreeBSD__)
		static const fs::path home = std::getenv("HOME");
		hint_path = home / ".steam" / "steam";
		if (fs::is_symlink(hint_path, error_code)) {
			hint_path = fs::read_symlink(hint_path, error_code);
		} else if (!fs::is_directory(hint_path, error_code)) {
			hint_path = home / ".local" / "share" / "Steam";
			if (!fs::is_directory(hint_path, error_code)) {
#ifdef __FreeBSD__
				spdlog::warn_s("Could not find path for Steam installation on FreeBSD.");
#else
				spdlog::warn_s("Could not find path for Steam installation on Linux.");
#endif
				return {};
			}
		}
		// Support only Mac, cannot support iOS
#elif (defined(__APPLE__) && defined(__MACH__)) && TARGET_OS_MAC == 1
		static const fs::path home = std::getenv("HOME");
		hint_path = home / "Library" / "Application Support" / "Steam";
		if (!fs::is_directory(hint_path, error_code)) {
			spdlog::warn_s("Could not find path for Steam installation on Mac.");
			return {};
		}
		// All platforms that reach this point do not seem to even have unofficial Steam support
#else
		spdlog::warn_s("Could not find path for Steam installation on unsupported platform.");
		return {};
#endif
	}

	// Could not determine Steam install on platform
	if (hint_path.empty()) {
		spdlog::warn_s("Steam install path not found.");
		return {};
	}

	// Supplied path was useless, ignore hint_path
	if (!hint_empty && !fs::exists(hint_path, error_code)) {
		return _search_for_game_path();
	}

	// Steam Library's directory that will contain Victoria 2
	fs::path vic2_steam_lib_directory;
	fs::path current_path = std::filesystem::weakly_canonical(hint_path, error_code);

	// If hinted path is directory that contains steamapps
	bool is_steamapps = false;
	if (fs::is_directory(current_path / steamapps, error_code)) {
		current_path /= steamapps;
		is_steamapps = true;
	}

	// If hinted path is steamapps directory
	bool is_libraryfolders_vdf = false;
	if (is_steamapps || (filename_equals(steamapps, current_path) && fs::is_directory(current_path, error_code))) {
		current_path /= libraryfolders;
		is_libraryfolders_vdf = true;
	}

	bool vic2_install_confirmed = false;
	// if current_path is not a regular file, this is a non-default Steam Library, skip this parser evaluation
	if (fs::is_regular_file(current_path, error_code) &&
		(is_libraryfolders_vdf || filename_equals(libraryfolders, current_path))) {
		lexy_vdf::Parser parser;

		memory::string buffer;
		auto error_log_stream = detail::make_callback_stream<char>(
			[](void const* s, std::streamsize n, void* user_data) -> std::streamsize {
				if (s != nullptr && n > 0 && user_data != nullptr) {
					static_cast<memory::string*>(user_data)->append(static_cast<char const*>(s), n);
					return n;
				} else {
					spdlog::warn("Invalid input to parser error log callback: {} / {} / {}", s, n, user_data);
					return 0;
				}
			},
			&buffer
		);
		parser.set_error_log_to(error_log_stream);

		parser.load_from_file(current_path);
		if (!parser.parse()) {
			// Could not find or load libraryfolders.vdf, report error as warning
			if (!buffer.empty()) {
				return search_log(hint_empty, "{}", buffer).result();
			}
			return search_log(hint_empty, "Could not parse VDF at '{}'.", current_path).result();
		}
		std::optional current_node = *parser.get_key_values();

		// check "libraryfolders" list
		auto it = current_node.value().find("libraryfolders");
		if (it == current_node.value().end()) {
			return search_log(hint_empty, "Expected libraryfolders.vdf to contain a libraryfolders key.").result();
		}

		static constexpr auto visit_node = [](auto&& arg) -> std::optional<lexy_vdf::KeyValues> {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, lexy_vdf::KeyValues>) {
				return arg;
			} else {
				return std::nullopt;
			}
		};

		current_node = std::visit(visit_node, it->second);

		if (!current_node.has_value()) {
			return search_log(hint_empty, "Expected libraryfolders.vdf's libraryfolders key to be a KeyValue dictionary.").result();
		}

		// Array of strings contain "0" to std::to_string(max_amount_of_steam_libraries - 1)
		static constexpr auto library_indexes = OpenVic::ConstexprIntToStr::make_itosv_array<max_amount_of_steam_libraries>();

		for (auto const& index : library_indexes) {
			decltype(current_node) node = std::nullopt;

			auto it = current_node.value().find(index);
			if (it != current_node.value().end()) {
				node = std::visit(visit_node, it->second);
			}

			// check "apps" list
			decltype(node) apps_node = std::nullopt;
			if (node.has_value()) {
				it = node.value().find("apps");
				if (it != node.value().end()) {
					apps_node = std::visit(visit_node, it->second);
				}
			}

			bool lib_contains_victoria_2 = false;
			if (apps_node.has_value()) {
				lib_contains_victoria_2 = apps_node.value().find("42960") != node.value().end();
			}

			if (lib_contains_victoria_2) {
				it = node.value().find("path");
				if (it != node.value().end()) {
					vic2_steam_lib_directory = std::visit(
						[](auto&& arg) -> std::string_view {
							using T = std::decay_t<decltype(arg)>;
							if constexpr (std::is_same_v<T, std::string>) {
								return arg;
							} else {
								return "";
							}
						},
						it->second
					);
					vic2_install_confirmed = true;
					break;
				}
			}
		}

		if (vic2_steam_lib_directory.empty()) {
			return search_log(hint_empty, "Steam installation appears not to contain Victoria 2.").result();
		}
	}

	// If current_path points to steamapps/libraryfolders.vdf
	if (vic2_steam_lib_directory.empty()) {
		if (is_libraryfolders_vdf || filename_equals(libraryfolders, current_path)) {
			vic2_steam_lib_directory = current_path.parent_path() / vic2_appmanifest;
		} else if (filename_equals(vic2_appmanifest, current_path)) {
			vic2_steam_lib_directory = current_path;
		}
	}

	// If we could not confirm Victoria 2 was installed via the default Steam installation
	bool is_common_folder = false;
	if (!vic2_install_confirmed && !vic2_steam_lib_directory.empty()) {
		auto parser = lexy_vdf::Parser::from_file(vic2_steam_lib_directory);
		if (!parser.parse()) {
			// Could not find or load appmanifest_42960.acf, report error as warning
			std::optional<fs::path> result;
			for (auto& error : parser.get_errors()) {
				// TODO: allow empty_fail_result_callback to use callbacks
				search_log l(hint_empty, "{}", error.message);
				if (!result) {
					result = l.result();
				}
			}
			return result.value_or(fs::path {});
		}

		// we can pretty much assume the Victoria 2 directory on Steam is valid from here
		vic2_steam_lib_directory /= common_folder;
		is_common_folder = true;
	} else if (fs::is_directory(vic2_steam_lib_directory / steamapps, error_code)) {
		vic2_steam_lib_directory /= fs::path(steamapps) / common_folder;
		is_common_folder = true;
	}

	bool is_Victoria_2_folder = false;
	if ((is_common_folder || filename_equals(common_folder, vic2_steam_lib_directory)) &&
		fs::is_directory(vic2_steam_lib_directory, error_code)) {
		vic2_steam_lib_directory /= Victoria_2_folder;
		is_Victoria_2_folder = true;
	}
	if ((is_Victoria_2_folder || filename_equals(Victoria_2_folder, vic2_steam_lib_directory)) &&
		fs::is_regular_file(vic2_steam_lib_directory / v2_game_exe, error_code)) {
		return vic2_steam_lib_directory;
	}

	// Hail Mary check ignoring the hint_path
	if (!hint_empty) {
		return _search_for_game_path();
	}

	spdlog::warn_s("Could not find Victoria 2 game path, this requires manually supplying one.");
	return {}; // The supplied path fits literally none of the criteria
}

fs::path Dataloader::search_for_game_path(fs::path hint_path) {
	struct fshash {
		size_t operator()(std::filesystem::path const& p) const noexcept {
			return std::filesystem::hash_value(p);
		}
	};
	using hint_path_t = fs::path;
	using game_path_t = fs::path;
	static ordered_map<hint_path_t, game_path_t, fshash> _cached_paths;

	auto it = _cached_paths.find(hint_path);
	if (it != _cached_paths.end()) {
		return it->second;
	}

	return _cached_paths[hint_path] = _search_for_game_path(hint_path);
}
