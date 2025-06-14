#pragma once

#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct GovernmentTypeManager;

	struct GovernmentType : HasIdentifier {
		friend struct GovernmentTypeManager;

	private:
		memory::vector<Ideology const*> PROPERTY(ideologies);
		const bool PROPERTY_CUSTOM_PREFIX(elections, holds);
		const bool PROPERTY_CUSTOM_PREFIX(appoint_ruling_party, can);
		const Timespan PROPERTY(term_duration);
		memory::string PROPERTY_CUSTOM_NAME(flag_type_identifier, get_flag_type);

		GovernmentType(
			std::string_view new_identifier, memory::vector<Ideology const*>&& new_ideologies, bool new_elections,
			bool new_appoint_ruling_party, Timespan new_term_duration, std::string_view new_flag_type_identifier
		);

	public:
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
} // namespace OpenVic
