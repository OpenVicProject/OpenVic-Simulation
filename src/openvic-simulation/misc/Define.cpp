#include "Define.hpp"

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

std::string_view Define::type_to_string(Type type) {
	using enum Type;

	switch (type) {
	case Date: return "date";
	case Country: return "country";
	case Economy: return "economy";
	case Military: return "military";
	case Diplomacy: return "diplomacy";
	case Pops: return "pops";
	case Ai: return "ai";
	case Graphics: return "graphics";
	default: return "unknown";
	}
}

Define::Type Define::string_to_type(std::string_view str) {
	using enum Type;

	static const string_map_t<Define::Type> type_map {
		{ "country",   Country },
		{ "economy",   Economy },
		{ "military",  Military },
		{ "diplomacy", Diplomacy },
		{ "pops",      Pops },
		{ "ai",        Ai },
		{ "graphics",  Graphics },
	};

	const string_map_t<Define::Type>::const_iterator type_it = type_map.find(str);

	if (type_it != type_map.end()) {
		return type_it->second;
	} else {
		return Unknown;
	}
}

Define::Define(std::string_view new_identifier, std::string_view new_value, Type new_type)
	: HasIdentifier { new_identifier }, value { new_value }, type { new_type } {}

Date Define::get_value_as_date(bool* successful) const {
	return Date::from_string(value, successful);
}

fixed_point_t Define::get_value_as_fp(bool* successful) const {
	return fixed_point_t::parse(value, successful);
}

int64_t Define::get_value_as_int(bool* successful) const {
	return StringUtils::string_to_int64(value, successful);
}

uint64_t Define::get_value_as_uint(bool* successful) const {
	return StringUtils::string_to_uint64(value, successful);
}

std::ostream& OpenVic::operator<<(std::ostream& os, Define::Type type) {
	return os << Define::type_to_string(type);
}

template<typename T>
bool DefineManager::load_define(T& value, Define::Type type, std::string_view name) const {
	static_assert(
		std::same_as<T, OpenVic::Date> || std::same_as<T, fixed_point_t> || std::integral<T>
	);

	Define const* define = defines.get_item_by_identifier(name);

	if (define != nullptr) {
		if (define->type != type) {
			Logger::warning("Mismatched define type for \"", name, "\" - expected ", type, ", got ", define->type);
		}

		const auto parse =
			[define, &value, &name]<typename U, U (Define::*Func)(bool*) const>(std::string_view type_name) -> bool {
				bool success = false;
				const U result = (define->*Func)(&success);
				if (success) {
					value = static_cast<T>(result);
					return true;
				} else {
					Logger::error("Failed to parse ", type_name, " \"", define->get_value(), "\" for define \"", name, "\"");
					return false;
				}
			};

		if constexpr (std::same_as<T, OpenVic::Date>) {
			return parse.template operator()<Date, &Define::get_value_as_date>("date");
		} else if constexpr (std::same_as<T, fixed_point_t>) {
			return parse.template operator()<fixed_point_t, &Define::get_value_as_fp>("fixed point");
		} else if constexpr (std::signed_integral<T>) {
			return parse.template operator()<int64_t, &Define::get_value_as_int>("signed int");
		} else if constexpr (std::unsigned_integral<T>) {
			return parse.template operator()<uint64_t, &Define::get_value_as_uint>("unsigned int");
		}
	} else {
		Logger::error("Missing define \"", name, "\"");
		return false;
	}
}

template<Timespan (*Func)(Timespan::day_t)>
bool DefineManager::_load_define_timespan(Timespan& value, Define::Type type, std::string_view name) const {
	Define const* define = defines.get_item_by_identifier(name);
	if (define != nullptr) {
		if (define->type != type) {
			Logger::warning("Mismatched define type for \"", name, "\" - expected ", type, ", got ", define->type);
		}
		bool success = false;
		const int64_t result = define->get_value_as_int(&success);
		if (success) {
			value = Func(result);
			return true;
		} else {
			Logger::error("Failed to parse days \"", define->get_value(), "\" for define \"", name, "\"");
			return false;
		}
	} else {
		Logger::error("Missing define \"", name, "\"");
		return false;
	}
}

bool DefineManager::load_define_days(Timespan& value, Define::Type type, std::string_view name) const {
	return _load_define_timespan<Timespan::from_days>(value, type, name);
}

bool DefineManager::load_define_months(Timespan& value, Define::Type type, std::string_view name) const {
	return _load_define_timespan<Timespan::from_months>(value, type, name);
}

bool DefineManager::load_define_years(Timespan& value, Define::Type type, std::string_view name) const {
	return _load_define_timespan<Timespan::from_years>(value, type, name);
}

DefineManager::DefineManager()
  : // Date
	start_date { 1836, 1, 1 },
	end_date { 1936, 1, 1 },

	// Country
	great_power_rank { 8 },
	lose_great_power_grace_days { Timespan::from_years(1) },
	secondary_power_rank { 16 }

	// Economy

	// Military

	// Diplomacy

	// Pops

	// Ai

	// Graphics

	{}

bool DefineManager::add_define(std::string_view name, std::string_view value, Define::Type type) {
	if (name.empty()) {
		Logger::error("Invalid define identifier - empty!");
		return false;
	}

	if (value.empty()) {
		Logger::error("Invalid define value for \"", name, "\" - empty!");
		return false;
	}

	return defines.add_item({ name, value, type }, duplicate_warning_callback);
}

bool DefineManager::in_game_period(Date date) const {
	return date.in_range(start_date, end_date);
}

bool DefineManager::load_defines_file(ast::NodeCPtr root) {
	using enum Define::Type;

	bool ret = expect_dictionary_keys(
		"defines", ONE_EXACTLY, expect_dictionary([this](std::string_view key, ast::NodeCPtr value) -> bool {

			const Define::Type type = Define::string_to_type(key);

			if (type != Unknown) {

				return expect_dictionary_reserve_length(
					defines,
					[this, type](std::string_view inner_key, ast::NodeCPtr value) -> bool {
						return expect_identifier_or_string(
							[this, &inner_key, type](std::string_view value) -> bool {
								return add_define(inner_key, value, type);
							}
						)(value);
					}
				)(value);

			} else if (key == "start_date" || key == "end_date") {

				return expect_identifier_or_string(
					[this, &key](std::string_view value) -> bool {
						return add_define(key, value, Date);
					}
				)(value);

			} else {

				Logger::error("Invalid define type - \"", key, "\"");
				return false;

			}
		})
	)(root);

	lock_defines();

	// Date
	ret &= load_define(start_date, Date, "start_date");
	ret &= load_define(end_date, Date, "end_date");

	// Country
	ret &= load_define(great_power_rank, Country, "GREAT_NATIONS_COUNT");
	ret &= load_define_days(lose_great_power_grace_days, Country, "GREATNESS_DAYS");
	ret &= load_define(secondary_power_rank, Country, "COLONIAL_RANK");

	// Economy

	// Military

	// Diplomacy

	// Pops

	// Ai

	// Graphics

	return ret;
}
