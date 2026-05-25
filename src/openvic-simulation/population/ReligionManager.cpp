#include "ReligionManager.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool ReligionManager::add_religion_group(std::string_view identifier) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid religion group identifier - empty!");
		return false;
	}
	return religion_groups.emplace_item(identifier, identifier);
}

bool ReligionManager::add_religion(
	std::string_view identifier, colour_t colour, ReligionGroup const& group, Religion::icon_t icon, bool is_pagan
) {
	if (!religion_groups.is_locked()) {
		spdlog::error_s("Cannot register religions until religion groups are locked!");
		return false;
	}
	if (identifier.empty()) {
		spdlog::error_s("Invalid religion identifier - empty!");
		return false;
	}
	if (icon <= 0) {
		spdlog::error_s("Invalid religion icon for {}: {}", identifier, icon);
		return false;
	}
	return religions.emplace_item(
		identifier,
		identifier, colour, group, icon, is_pagan
	);
}

/* REQUIREMENTS:
 * POP-286, POP-287, POP-288, POP-289, POP-290, POP-291, POP-292,
 * POP-293, POP-294, POP-295, POP-296, POP-297, POP-298, POP-299
 */
bool ReligionManager::load_religion_file(ovdl::v2script::ast::Node const* root) {
	size_t total_expected_religions = 0;
	bool ret = expect_dictionary_reserve_length(
		religion_groups,
		[this, &total_expected_religions](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
			bool ret = expect_length(add_variable_callback(total_expected_religions))(value);
			ret &= add_religion_group(key);
			return ret;
		}
	)(root);
	lock_religion_groups();
	reserve_more_religions(total_expected_religions);
	ret &= expect_religion_group_dictionary(
		[this](ReligionGroup const& religion_group, ovdl::v2script::ast::Node const* religion_group_value) -> bool {
			return expect_dictionary([this, &religion_group](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
				colour_t colour = colour_t::null();
				Religion::icon_t icon = 0;
				bool is_pagan = false;

				bool ret = expect_dictionary_keys(
					"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
					"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
					"pagan", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_pagan))
				)(value);
				ret &= add_religion(key, colour, religion_group, icon, is_pagan);
				return ret;
			})(religion_group_value);
		}
	)(root);
	lock_religions();
	return ret;
}
