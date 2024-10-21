#pragma once

#include "modifier/mappings/country_mappings/BaseCountryModifierMapping.hpp"

namespace OpenVic {
	class BaseProvinceModifierMapping : public BaseCountryModifierMapping {
		public:
			NodeTools::node_callback_t expect_modifier_value_and_default(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_value_callback_t const& default_callback
			) const override;
	};
}