#pragma once

#include "openvic-simulation/dataloader/Node_forwarded.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ModifierManager;

	struct GoodDefinitionManager {
	private:
		IdentifierRegistry<GoodCategory> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(good_category, good_categories);
		IdentifierRegistry<GoodDefinition> IDENTIFIER_REGISTRY(good_definition);

	public:
		bool add_good_category(std::string_view identifier, size_t expected_goods_in_category);

		bool add_good_definition(
			std::string_view identifier, colour_t colour, GoodCategory& category, fixed_point_t base_price,
			bool is_available_from_start, bool is_tradeable, bool is_money, bool has_overseas_penalty
		);

		bool load_goods_file(ovdl::v2script::ast::Node const* root);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
