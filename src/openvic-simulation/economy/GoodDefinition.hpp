#pragma once

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct GoodDefinitionManager;

	struct GoodCategory : HasIdentifier {
		friend struct GoodDefinitionManager;

	private:
		GoodCategory(std::string_view new_identifier);

	public:
		GoodCategory(GoodCategory&&) = default;
	};

	/* REQUIREMENTS:
	 *
	 * ECON-3 , ECON-4 , ECON-5 , ECON-6 , ECON-7 , ECON-8 , ECON-9 , ECON-10, ECON-11, ECON-12, ECON-13, ECON-14,
	 * ECON-15, ECON-16, ECON-17, ECON-18, ECON-19, ECON-20, ECON-21, ECON-22, ECON-23, ECON-24, ECON-25, ECON-26,
	 * ECON-27, ECON-28, ECON-29, ECON-30, ECON-31, ECON-32, ECON-33, ECON-34, ECON-35, ECON-36, ECON-37, ECON-38,
	 * ECON-39, ECON-40, ECON-41, ECON-42, ECON-43, ECON-44, ECON-45, ECON-46, ECON-47, ECON-48, ECON-49, ECON-50
	 *
	 * ECON-123, ECON-124, ECON-125, ECON-126, ECON-127, ECON-128, ECON-129, ECON-130, ECON-131, ECON-132, ECON-133, ECON-134,
	 * ECON-135, ECON-136, ECON-137, ECON-138, ECON-139, ECON-140, ECON-141, ECON-142, ECON-234, ECON-235, ECON-236, ECON-237,
	 * ECON-238, ECON-239, ECON-240, ECON-241, ECON-242, ECON-243, ECON-244, ECON-245, ECON-246, ECON-247, ECON-248, ECON-249,
	 * ECON-250, ECON-251, ECON-252, ECON-253, ECON-254, ECON-255, ECON-256, ECON-257, ECON-258, ECON-259, ECON-260, ECON-261
	 */
	struct GoodDefinition : HasIdentifierAndColour, HasIndex<> {
		friend struct GoodDefinitionManager;

		using good_definition_map_t = fixed_point_map_t<GoodDefinition const*>;

	private:
		GoodCategory const& PROPERTY(category);
		const fixed_point_t PROPERTY(base_price);
		const bool PROPERTY_CUSTOM_PREFIX(available_from_start, is);
		const bool PROPERTY_CUSTOM_PREFIX(tradeable, is);
		const bool PROPERTY(money);
		const bool PROPERTY(overseas_penalty);

		GoodDefinition(
			std::string_view new_identifier, colour_t new_colour, index_t new_index, GoodCategory const& new_category,
			fixed_point_t new_base_price, bool new_available_from_start, bool new_tradeable, bool new_money,
			bool new_overseas_penalty
		);

	public:
		GoodDefinition(GoodDefinition&&) = default;
	};

	struct GoodDefinitionManager {
	private:
		IdentifierRegistry<GoodCategory> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(good_category, good_categories);
		IdentifierRegistry<GoodDefinition> IDENTIFIER_REGISTRY(good_definition);

	public:
		bool add_good_category(std::string_view identifier);

		bool add_good_definition(
			std::string_view identifier, colour_t colour, GoodCategory const& category, fixed_point_t base_price,
			bool available_from_start, bool tradeable, bool money, bool overseas_penalty
		);

		bool load_goods_file(ast::NodeCPtr root);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
