#pragma once

#include <string_view>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryParty.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/PartyPolicy.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Error.hpp"

namespace OpenVic {
	class CountryParser {
		IdentifierRegistry<GovernmentType> const* government_type_registry;
		IdentifierRegistry<GraphicalCultureType> const* graphical_culture_type_registry;
		IdentifierPointerRegistry<UnitType> const* unit_type_registry;
		IdentifierRegistry<PartyPolicyGroup> const* policy_group_registry;
		IdentifierRegistry<PartyPolicy> const* party_policy_registry;
		IdentifierRegistry<Ideology> const* ideology_registry;

		ovdl::v2script::Parser list_parser;

		OV_SPEED_INLINE NodeTools::NodeCallback auto _expect_country_party( //
			IdentifierRegistry<CountryParty>& r_parties_registry
		);

	public:
		struct TagData {
			std::string_view tag;
			std::string_view path;
			bool is_dynamic = false;
		};

	private:
		memory::vector<TagData> PROPERTY(tags);

	public:
		CountryParser(
			IdentifierRegistry<GovernmentType> const& government_type_registry, //
			IdentifierRegistry<GraphicalCultureType> const& graphical_culture_type_registry, //
			IdentifierPointerRegistry<UnitType> const& unit_type_registry, //
			IdentifierRegistry<PartyPolicyGroup> const& policy_group_registry, //
			IdentifierRegistry<PartyPolicy> const& party_policy_registry, //
			IdentifierRegistry<Ideology> const& ideology_registry, //
			ovdl::v2script::Parser list_parser
		);

		Error load_country_list();
		Error load_countries_from(std::string_view common_folder, Dataloader& dataloader, CountryDefinitionManager& manager);

		NodeTools::node_callback_t expect_country_party(IdentifierRegistry<CountryParty>& r_parties_registry);

		Error load_country( //
			std::string_view tag, bool is_dynamic_tag, ast::NodeCPtr country_root, CountryDefinitionManager& manager
		);

		Error load_country_colours(ast::NodeCPtr colours_root, CountryDefinitionManager& manager);
	};
}
