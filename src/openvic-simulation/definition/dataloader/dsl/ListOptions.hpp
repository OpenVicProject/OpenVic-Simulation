#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

namespace OpenVic::dataloader::dsl {
	struct ListOptions {
		dataloader::DiagnosticLevel unknown_log_level = dataloader::DiagnosticLevel::WARN;
		dataloader::DiagnosticLevel duplicate_log_level = dataloader::DiagnosticLevel::ERROR;
	};

	struct warn_for_unknown_t {
		static constexpr ListOptions get_options() {
			return { .unknown_log_level = dataloader::DiagnosticLevel::WARN };
		}
	};

	inline constexpr warn_for_unknown_t warn_for_unknown {};

	struct silence_unknown_t {
		static constexpr ListOptions get_options() {
			return { .unknown_log_level = dataloader::DiagnosticLevel::NONE };
		}
	};

	inline constexpr silence_unknown_t silence_unknown {};

	struct error_for_unknown_t {
		static constexpr ListOptions get_options() {
			return { .unknown_log_level = dataloader::DiagnosticLevel::ERROR };
		}
	};

	inline constexpr error_for_unknown_t error_for_unknown {};

	template<dataloader::DiagnosticLevel Level>
	struct report_unknown_as_t {
		static constexpr ListOptions get_options() {
			return { .unknown_log_level = Level };
		}
	};

	template<dataloader::DiagnosticLevel Level>
	inline constexpr report_unknown_as_t<Level> report_unknown_as {};

	template<ListOptions Options>
	struct set_list_options_t {
		static constexpr ListOptions get_options() {
			return Options;
		}
	};

	template<ListOptions Options>
	inline constexpr set_list_options_t<Options> set_list_options {};

	template<typename T>
	concept list_option_tag = requires {
		{ T::get_options() } -> std::same_as<ListOptions>;
	};

	namespace detail {
		template<typename... Ts>
		consteval ListOptions get_options() {
			ListOptions opts {};
			(
			    [&opts]<typename T>() mutable {
				    if constexpr (list_option_tag<T>) {
					    opts = T::get_options();
				    }
			    }.template operator()<Ts>(),
			    ...);
			return opts;
		}

		template<typename... Ts>
		consteval auto pure_index_sequence() {
			constexpr std::array is_pure = [] {
				if constexpr (sizeof...(Ts) != 0) {
					return std::to_array<bool>({ !list_option_tag<Ts>... });
				} else {
					return std::array<bool, 0> {};
				}
			}();
			constexpr std::size_t N = sizeof...(Ts);

			constexpr std::size_t count = [&] {
				std::size_t c = 0;
				for (bool b : is_pure) {
					if (b) {
						++c;
					}
				}
				return c;
			}();

			constexpr auto indices = [&] {
				std::array<std::size_t, count> idx {};
				std::size_t out = 0;
				for (std::size_t i = 0; i < N; ++i) {
					if (is_pure[i]) {
						idx[out++] = i;
					}
				}
				return idx;
			}();

			return [&]<std::size_t... I>(std::index_sequence<I...>) {
				return std::index_sequence<indices[I]...> {};
			}(std::make_index_sequence<count> {});
		}
	}
}
