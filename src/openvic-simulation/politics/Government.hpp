#pragma once

#include "openvic-simulation/politics/Ideology.hpp"

namespace OpenVic {
	struct GovernmentTypeManager;

	struct GovernmentType : HasIdentifier {
		friend struct GovernmentTypeManager;

	private:
		const std::vector<Ideology const*> PROPERTY(ideologies);
		const bool PROPERTY_CUSTOM_PREFIX(elections, holds);
		const bool PROPERTY_CUSTOM_PREFIX(appoint_ruling_party, can);
		const Timespan PROPERTY(term_duration);
		const std::string PROPERTY_CUSTOM_NAME(flag_type_identifier, get_flag_type);

		GovernmentType(
			std::string_view new_identifier, std::vector<Ideology const*>&& new_ideologies, bool new_elections,
			bool new_appoint_ruling_party, Timespan new_term_duration, std::string_view new_flag_type_identifier
		);

	public:
		GovernmentType(GovernmentType&&) = default;

		bool is_ideology_compatible(Ideology const* ideology) const;
	};

	struct GovernmentTypeManager {
	private:
		IdentifierRegistry<GovernmentType> government_types;
		std::vector<std::string> PROPERTY(flag_types);

	public:
		GovernmentTypeManager();

		bool add_government_type(
			std::string_view identifier, std::vector<Ideology const*>&& ideologies, bool elections, bool appoint_ruling_party,
			Timespan term_duration, std::string_view flag_type
		);
		IDENTIFIER_REGISTRY_ACCESSORS(government_type)

		bool load_government_types_file(IdeologyManager const& ideology_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic
