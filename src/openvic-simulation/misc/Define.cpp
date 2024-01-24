#include "Define.hpp"

#include <cassert>
#include <cstdlib>
#include <memory>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Define::Define(std::string_view new_identifier, std::string&& new_value, Type new_type)
	: HasIdentifier { new_identifier }, value { std::move(new_value) }, type { new_type } {}

fixed_point_t Define::get_value_as_fp() const {
	return fixed_point_t::parse(value);
}

int64_t Define::get_value_as_int() const {
	return std::strtoll(value.data(), nullptr, 10);
}

uint64_t Define::get_value_as_uint() const {
	return std::strtoull(value.data(), nullptr, 10);
}

bool DefineManager::add_define(std::string_view name, std::string&& value, Define::Type type) {
	if (name.empty()) {
		Logger::error("Invalid define identifier - empty!");
		return false;
	}
	return defines.add_item({ name, std::move(value), type }, duplicate_warning_callback);
}

Date DefineManager::get_start_date() const {
	return start_date ? *start_date : Date {};
}

Date DefineManager::get_end_date() const {
	return end_date ? *end_date : Date {};
}

bool DefineManager::in_game_period(Date date) const {
	if (start_date && end_date) {
		return date.in_range(*start_date, *end_date);
	} else {
		return false;
	}
}

bool DefineManager::add_date_define(std::string_view name, Date date) {
	if (name == "start_date") {
		start_date = date;
	} else if (name == "end_date") {
		end_date = date;
	} else {
		Logger::error("Invalid date define identifier - \"", name, "\" (must be start_date or end_date)");
		return false;
	}
	return defines.add_item({ name, date.to_string(), Define::Type::Date });
}

bool DefineManager::load_defines_file(ast::NodeCPtr root) {
	bool ret = expect_dictionary_keys(
		"defines", ONE_EXACTLY, expect_dictionary([this](std::string_view key, ast::NodeCPtr value) -> bool {
			using enum Define::Type;
			static const string_map_t<Define::Type> type_map {
				{ "country",   Country },
				{ "economy",   Economy },
				{ "military",  Military },
				{ "diplomacy", Diplomacy },
				{ "pops",      Pops },
				{ "ai",        Ai },
				{ "graphics",  Graphics },
			};

			const string_map_t<Define::Type>::const_iterator type_it = type_map.find(key);

			if (type_it != type_map.end()) {

				return expect_dictionary_reserve_length(
					defines,
					[this, &key, type = type_it->second](std::string_view inner_key, ast::NodeCPtr value) -> bool {
						std::string str_val;
						bool ret = expect_identifier_or_string(assign_variable_callback_string(str_val))(value);
						ret &= add_define(inner_key, std::move(str_val), type);
						return ret;
					}
				)(value);

			} else if (key == "start_date" || key == "end_date") {

				return expect_identifier_or_string(expect_date_str(
					std::bind_front(&DefineManager::add_date_define, this, key)
				))(value);

			} else {
				return false;
			}
		})
	)(root);

	lock_defines();

	return ret;
}
