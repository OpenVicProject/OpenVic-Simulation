
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

Define::Define(
	std::string_view new_identifier,
	std::string&& new_value,
	Type new_type
) : HasIdentifier { new_identifier },
	value { std::move(new_value) },
	type { new_type } {
}

fixed_point_t Define::get_value_as_fp() const {
	return fixed_point_t::parse(value);
}

int64_t Define::get_value_as_int() const {
	return std::strtoll(value.data(), nullptr, 10);
}

uint64_t Define::get_value_as_uint() const {
	return std::strtoull(value.data(), nullptr, 10);
}

DefineManager::DefineManager() : defines { "defines" } {
}

bool DefineManager::add_define(std::string_view name, std::string&& value, Define::Type type) {
	return defines.add_item({ name, std::move(value), type }, duplicate_warning_callback);
}

const Date& DefineManager::get_start_date() const {
	return *start_date;
}

const Date& DefineManager::get_end_date() const {
	return *end_date;
}

bool DefineManager::add_date_define(std::string_view name, Date date) {
	if (name != "start_date" && name != "end_date") return false;

	bool ret = defines.add_item({ name, date.to_string(), Define::Type::None });

	if (name == "start_date")
		start_date.reset(new Date(date));
	else if (name == "end_date")
		end_date.reset(new Date(date));

	return ret;
}

bool DefineManager::load_defines_file(ast::NodeCPtr root) {
	bool ret = expect_dictionary_keys(
		"defines", ONE_EXACTLY, expect_dictionary([this](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "country" || key == "economy" || key == "military" || key == "diplomacy" || key == "pops" || key == "ai" || key == "graphics") {
				return expect_dictionary([this, &key](std::string_view inner_key, ast::NodeCPtr value) -> bool {
					std::string str_val;

					bool ret = expect_identifier_or_string(assign_variable_callback_string(str_val))(value);

					Define::Type type;
					switch (key[0]) {
						using enum Define::Type;
						case 'c': // country
							type = Country;
							break;
						case 'e': // economy
							type = Economy;
							break;
						case 'm': // military
							type = Military;
							break;
						case 'd': // diplomacy
							type = Diplomacy;
							break;
						case 'p': // pops
							type = Pops;
							break;
						case 'a': // ai
							type = Ai;
							break;
						case 'g': // graphics
							type = Graphics;
							break;
						default:
							Logger::error("Unknown define type ", key, " found in defines!");
							return false;
					}

					ret &= add_define(inner_key, std::move(str_val), type);
					return ret;
				})(value);
			} else if (key == "start_date" || key == "end_date") {
				using namespace std::placeholders;

				return expect_identifier_or_string(expect_date_str([this, &key](Date date) -> bool {
					return add_date_define(key, date);
				}))(value);
			} else {
				return false;
			}
		})
	)(root);
	lock_defines();
	return ret;
}