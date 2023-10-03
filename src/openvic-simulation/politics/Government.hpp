#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/politics/Ideology.hpp"

namespace OpenVic {
    struct GovernmentTypeManager;

    struct GovernmentType : HasIdentifier {
        friend struct GovernmentTypeManager;

    private:
        std::vector<std::string> ideologies;
        const bool elections, appoint_ruling_party;
        const uint16_t election_duration;
        const std::string_view flag_type_identifier;

        GovernmentType(std::string_view new_identifier, std::vector<std::string> new_ideologies, bool new_elections, bool new_appoint_ruling_party, uint16_t new_election_duration, std::string_view new_flag_type_identifier);

    public:
        GovernmentType(GovernmentType&&) = default;

        bool is_ideology_compatible(std::string_view ideology) const;
        bool holds_elections() const;
        bool can_appoint_ruling_party() const;
        uint16_t get_election_duration() const;
        std::string_view get_flag_type() const;
    };

    struct GovernmentTypeManager {
    private:
        IdentifierRegistry<GovernmentType> government_types;

    public:
        GovernmentTypeManager();

        bool add_government_type(std::string_view identifier, std::vector<std::string> ideologies, bool elections, bool appoint_ruling_party, uint16_t election_duration, std::string_view flag_type);
        IDENTIFIER_REGISTRY_ACCESSORS(GovernmentType, government_type)

        bool load_government_types_file(IdeologyManager& ideology_manager, ast::NodeCPtr root);
    };
} // namespace OpenVic 
