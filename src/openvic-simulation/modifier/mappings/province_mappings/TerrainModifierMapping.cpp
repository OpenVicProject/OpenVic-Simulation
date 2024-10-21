#include "TerrainModifierMapping.hpp"
#include "modifier/mappings/BaseModifierMapping.hpp"

using namespace OpenVic;

NodeTools::node_callback_t TerrainModifierMapping::expect_modifier_value_and_default(
	NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
	NodeTools::key_value_callback_t const& default_callback
) const {
	//special combat_width + min_build_fort + movement_cost + defence + more?
	return BaseModifierMapping::expect_modifier_value_and_default(modifier_callback, default_callback);
}