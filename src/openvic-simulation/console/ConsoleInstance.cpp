#include "openvic-simulation/console/ConsoleInstance.hpp"

#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/military/Wargoal.hpp"
#include "openvic-simulation/misc/Event.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;
using namespace std::string_view_literals;

ConsoleInstance::ConsoleInstance(InstanceManager& instance_manager) : ConsoleInstance(instance_manager, &default_write_func) {}

ConsoleInstance::ConsoleInstance(InstanceManager& instance_manager, write_func_t&& write_func)
	: instance_manager(instance_manager) {
	set_write_func(std::move(write_func));
	setup_commands();
}

void ConsoleInstance::write(std::string&& message) {
	write_func(current_colour, std::move(message));
}

void ConsoleInstance::set_write_func(write_func_t&& new_write_func) {
	write_func = std::move(new_write_func);
}

void ConsoleInstance::default_write_func(OpenVic::colour_t colour, std::string&& message) {
	fmt::print("{}", fmt::styled(message, fmt::bg(fmt::rgb(colour.contrast().as_rgb())) | fmt::fg(fmt::rgb(colour.as_rgb()))));
}

bool ConsoleInstance::execute(std::string_view command) {
	if (command.empty()) {
		return false;
	}

	std::vector<std::string_view> arguments;
	arguments.reserve(4);

	for (std::string_view::const_iterator it = command.begin(); it < command.end(); it++) {
		// Skip leading whitespace
		if (std::isspace(*it)) {
			continue;
		}

		// End if null termination found
		if (*it == '\0') {
			break;
		}

		char quote = '\0';

		// Support ' and " quoting
		if (*it == '\'' || *it == '"') {
			quote = *it;
			it++;
			// Accessing command.end() is UB
			if (it == command.end()) {
				break;
			}
		}

		char const* arg_start = &*it;

		for (; it < command.end(); it++) {
			// End argument if null termination found
			if (*it == '\0') {
				break;
			}

			// If quote is valid then check quote, else check space
			if (quote != '\0' ? *it == quote : std::isspace(*it)) {
				// Argument is finished
				break;
			}

			// Backslash Escape
			// Accessing command.end() is UB
			if (*it == '\\' && ((it + 1) != command.end()) && *(it + 1) != '\0') {
				it++;
			}
		}

		// Accessing command.end() is UB
		// Standard does not promise iterators can be treated as pointers, MSVC is the only one that doesn't support that
		char const* arg_end = it == command.end() ? (&*(it - 1) + 1) : &*(it + 1);

		if (quote) {
			// Trims leading whitespace
			for (; arg_start < arg_end; arg_start++) {
				if (!std::isspace(*arg_start)) {
					break;
				}
			}

			// Trims trailing whitespace
			for (; arg_end - 1 > arg_start; arg_end--) {
				if (!std::isspace(*(arg_end - 1))) {
					break;
				}
			}
		}

		arguments.emplace_back(arg_start, arg_end);

		// Accessing command.end() is UB
		if (it == command.end()) {
			break;
		}

		// Skip the closing quote
		if (quote && *it == quote) {
			it++;
		}
	}

	if (arguments.empty()) {
		return false;
	}

	decltype(commands)::iterator cmd_it = commands.find(arguments[0]);
	if (cmd_it == commands.end()) {
		return false;
	}

	Argument arg = Argument { *this, arguments };
	return cmd_it.value()(arg);
}

bool ConsoleInstance::add_command(std::string&& identifier, execute_command_func_t execute) {
	OV_ERR_FAIL_COND_V_MSG(identifier.empty(), false, "Invalid command identifier - empty!");
	return commands.try_emplace(std::move(identifier), execute).second;
}

bool ConsoleInstance::validate_argument_size(std::span<std::string_view> arguments, size_t size) {
	return arguments.size() >= size;
}

std::optional<int64_t> ConsoleInstance::validate_integer(std::string_view value_string) {
	int64_t value = 0;
	std::from_chars_result result = std::from_chars(value_string.data(), value_string.data() + value_string.size(), value);
	if (result.ec == std::errc()) {
		return value;
	}

	write_error("Invalid argument. Value is not an integer");
	return std::nullopt;
}

std::optional<uint64_t> ConsoleInstance::validate_unsigned_integer(std::string_view value_string) {
	uint64_t value = 0;
	std::from_chars_result result = std::from_chars(value_string.data(), value_string.data() + value_string.size(), value);
	if (result.ec == std::errc()) {
		return value;
	}

	write_error("Invalid argument. Value is not an integer");
	return std::nullopt;
}

std::optional<bool> ConsoleInstance::validate_boolean(std::string_view value_string) {
	constexpr std::string_view TRUE_VALUE = "true"sv;
	constexpr std::string_view FALSE_VALUE = "false"sv;
	constexpr std::string_view YES_VALUE = "yes"sv;
	constexpr std::string_view NO_VALUE = "no"sv;
	constexpr std::string_view ONE_VALUE = "1"sv;
	constexpr std::string_view ZERO_VALUE = "0"sv;
	constexpr std::string_view ON_VALUE = "on"sv;
	constexpr std::string_view OFF_VALUE = "off"sv;

	if (value_string == TRUE_VALUE || value_string == YES_VALUE || //
		value_string == ONE_VALUE || value_string == ON_VALUE) {
		return true;
	} else if (value_string == FALSE_VALUE || value_string == NO_VALUE || //
			   value_string == ZERO_VALUE || value_string == OFF_VALUE) {
		return false;
	}

	write_error("Invalid argument. Specify on or off.");
	return std::nullopt;
}

ProvinceInstance* ConsoleInstance::validate_province_index(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify province id");
		return nullptr;
	}

	std::optional<uint64_t> result = validate_unsigned_integer(value_string);
	if (!result) {
		return nullptr;
	}

	ProvinceInstance* province = instance_manager.get_map_instance().get_province_instance_by_index(*result);

	if (province == nullptr) {
		write_error("Unknown province id");
	}
	return province;
}

CountryInstance* ConsoleInstance::validate_country_tag(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify country tag");
		return nullptr;
	}

	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_identifier(value_string);

	if (country == nullptr) {
		write_error("Unknown country");
	}
	return country;
}

Event const* ConsoleInstance::validate_event_id(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify event id");
		return nullptr;
	}

	std::optional<uint64_t> result = validate_unsigned_integer(value_string);
	if (!result) {
		return nullptr;
	}

	Event const* event = instance_manager.get_definition_manager().get_event_manager().get_event_by_identifier(value_string);
	if (event == nullptr) {
		write_error("Unknown event with ID #{}", *result);
	}
	return event;
}

UnitType const* ConsoleInstance::validate_unit(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify unit type");
		return nullptr;
	}

	UnitType const* unit_type = //
		instance_manager
			.get_definition_manager() //
			.get_military_manager()
			.get_unit_type_manager()
			.get_unit_type_by_identifier(value_string);

	if (unit_type == nullptr) {
		write_error("Unknown unit type");
	}

	return unit_type;
}

std::string_view ConsoleInstance::validate_filename(std::string_view value_string) {
	// TODO: validate filename
	return value_string;
}

std::string_view ConsoleInstance::validate_shader_effect(std::string_view value_string) {
	// TODO: validate shader effect
	return value_string;
}

std::string_view ConsoleInstance::validate_texture_name(std::string_view value_string) {
	// TODO: validate texture name
	return value_string;
}

std::optional<Date> ConsoleInstance::validate_date(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify date");
		return std::nullopt;
	}

	Date::from_chars_result result;
	Date date = Date::from_string(value_string, &result);
	if (result.ec != std::errc {}) {
		write_error("Invalid date");
		return std::nullopt;
	}
	return date;
}

Invention const* ConsoleInstance::validate_invention(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify invention");
		return nullptr;
	}

	Invention const* invention = //
		instance_manager
			.get_definition_manager() //
			.get_research_manager()
			.get_invention_manager()
			.get_invention_by_identifier(value_string);

	if (invention == nullptr) {
		write_error("Unknown invention");
	}
	return invention;
}

WargoalType const* ConsoleInstance::validate_cb_type(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify casus belli type");
		return nullptr;
	}

	WargoalType const* cb_type = //
		instance_manager
			.get_definition_manager() //
			.get_military_manager()
			.get_wargoal_type_manager()
			.get_wargoal_type_by_identifier(value_string);

	if (cb_type == nullptr) {
		write_error("Unknown casus belli type");
	}
	return cb_type;
}

Technology const* ConsoleInstance::validate_tech_name(std::string_view value_string) {
	if (value_string.empty()) {
		write_error("Please specify technology");
		return nullptr;
	}

	Technology const* tech = //
		instance_manager
			.get_definition_manager() //
			.get_research_manager()
			.get_technology_manager()
			.get_technology_by_identifier(value_string);

	if (tech == nullptr) {
		write_error("Unknown technology");
	}
	return tech;
}

bool ConsoleInstance::setup_commands() {
	bool result = true;

	constexpr auto toggle_province_id = [](Argument& arg) -> bool {
		// TODO: toggle province id display
		return true;
	};

	result &= add_command("showprovinceid", toggle_province_id);
	result &= add_command("provid", toggle_province_id); // spellchecker:disable-line

	result &= add_command("date", [](Argument& arg) -> bool {
		if (!arg.console.validate_argument_size(arg.arguments, 2)) {
			return false;
		}

		std::optional<Date> date;
		if (date = arg.console.validate_date(arg.arguments[1]); !date) {
			return false;
		}

		arg.console.get_instance_manager().set_today_and_update(*date);
		return true;
	});

	return result;
}
