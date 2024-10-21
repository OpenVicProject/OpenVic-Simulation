#include "BaseModifierMapping.hpp"

using namespace OpenVic;

NodeTools::node_callback_t BaseModifierMapping::expect_modifier_value(
	NodeTools::callback_t<ModifierValue&&> const& modifier_callback
) const {
	return expect_modifier_value_and_default(modifier_callback, NodeTools::key_value_invalid_callback);
}

NodeTools::node_callback_t BaseModifierMapping::expect_modifier_value_and_key_map_and_default(
	NodeTools::callback_t<ModifierValue&&> const& modifier_callback, NodeTools::key_value_callback_t const& default_callback,
	NodeTools::key_map_t&& key_map
) const {
	return [this, modifier_callback, default_callback, key_map = std::move(key_map)](
		ast::NodeCPtr node
	) mutable -> bool {
		bool ret = expect_modifier_value_and_default(
			modifier_callback, dictionary_keys_callback(key_map, default_callback)
		)(node);

		ret &= check_key_map_counts(key_map);

		return ret;
	};
}

NodeTools::node_callback_t BaseModifierMapping::expect_modifier_value_and_key_map(
	NodeTools::callback_t<ModifierValue&&> const& modifier_callback, NodeTools::key_map_t&& key_map
) const {
	return expect_modifier_value_and_key_map_and_default(
		modifier_callback, NodeTools::key_value_invalid_callback, std::move(key_map)
	);
}