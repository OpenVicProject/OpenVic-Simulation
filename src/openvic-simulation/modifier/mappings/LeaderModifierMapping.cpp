#include "modifier/mappings/BaseModifierMapping.hpp"

using namespace OpenVic;

NodeTools::node_callback_t BaseModifierMapping::expect_modifier_value_and_default(
	NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
	NodeTools::key_value_callback_t const& default_callback
) const {
	//parse leader modifiers
}