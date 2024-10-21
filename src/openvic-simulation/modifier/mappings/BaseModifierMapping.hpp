#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/ModifierValue.hpp"

namespace OpenVic {
	class BaseModifierMapping {
		public:
			virtual NodeTools::node_callback_t expect_modifier_value_and_default(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_value_callback_t const& default_callback
			) const = 0;

			NodeTools::node_callback_t expect_modifier_value(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback
			) const;

			// In the functions below, key_map refers to a map from identifier strings to NodeTools::dictionary_entry_t,
			// allowing any non-modifier effect keys found to be parsed in a custom way, similar to expect_dictionary_keys.
			NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_value_callback_t const& default_callback, NodeTools::key_map_t&& key_map
			) const;
			NodeTools::node_callback_t expect_modifier_value_and_key_map(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_map_t&& key_map
			) const;

			template<typename... Args>
			NodeTools::node_callback_t expect_modifier_value_and_key_map_and_default(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_value_callback_t const& default_callback, NodeTools::key_map_t&& key_map, Args... args
			) const {
				NodeTools::add_key_map_entries(key_map, args...);
				return expect_modifier_value_and_key_map_and_default(
					modifier_callback, default_callback, std::move(key_map)
				);
			}

			template<typename... Args>
			NodeTools::node_callback_t expect_modifier_value_and_keys_and_default(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_value_callback_t const& default_callback, Args... args
			) const {
				return expect_modifier_value_and_key_map_and_default(modifier_callback, default_callback, {}, args...);
			}
			template<typename... Args>
			NodeTools::node_callback_t expect_modifier_value_and_keys(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback, Args... args
			) const {
				return expect_modifier_value_and_key_map_and_default(
					modifier_callback, NodeTools::key_value_invalid_callback, {}, args...
				);
			}
	};
}