#pragma once

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct Ideology;
	struct IdeologyManager;

	struct GovernmentType : HasIndex<GovernmentType, government_type_index_t>, HasIdentifier {
	private:
		memory::vector<Ideology const*> SPAN_PROPERTY(ideologies);
		memory::string PROPERTY_CUSTOM_NAME(flag_type_identifier, get_flag_type);

	public:
		const bool holds_elections;
		const bool can_appoint_ruling_party;
		const Timespan term_duration;

		GovernmentType(
			index_t new_index,
			std::string_view new_identifier,
			memory::vector<Ideology const*>&& new_ideologies,
			bool new_holds_elections,
			bool new_can_appoint_ruling_party,
			Timespan new_term_duration,
			std::string_view new_flag_type_identifier
		);
		GovernmentType(GovernmentType&&) = default;

		bool is_ideology_compatible(Ideology const* ideology) const;
	};

	struct GovernmentTypeManager {
	private:
		IdentifierRegistry<GovernmentType> IDENTIFIER_REGISTRY(government_type);
		string_set_t PROPERTY(flag_types);

	public:
		bool add_government_type(
			std::string_view identifier, memory::vector<Ideology const*>&& ideologies, bool elections, bool appoint_ruling_party,
			Timespan term_duration, std::string_view flag_type
		);

		bool load_government_types_file(IdeologyManager const& ideology_manager, ast::NodeCPtr root);

		bool is_valid_flag_type(std::string_view type) const;
	};
}
