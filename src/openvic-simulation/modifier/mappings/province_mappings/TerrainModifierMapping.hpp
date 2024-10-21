#pragma once

#include "modifier/mappings/province_mappings/BaseProvinceModifierMapping.hpp"

namespace OpenVic {
	class TerrainModifierMapping final: public BaseProvinceModifierMapping {
		public:
			NodeTools::node_callback_t expect_modifier_value_and_default(
				NodeTools::callback_t<ModifierValue&&> const& modifier_callback,
				NodeTools::key_value_callback_t const& default_callback
			) const override;
	};
}