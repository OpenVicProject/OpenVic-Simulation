#include "Government.hpp"

#include <set>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/GameManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GovernmentType::GovernmentType(std::string_view new_identifier, std::vector<Ideology const*> new_ideologies, bool new_elections, bool new_appoint_ruling_party, Timespan new_election_duration, std::string_view new_flag_type_identifier)
    : HasIdentifier { new_identifier }, ideologies { new_ideologies }, elections { new_elections }, appoint_ruling_party { new_appoint_ruling_party }, election_duration { new_election_duration }, flag_type_identifier { new_flag_type_identifier } {}

bool GovernmentType::is_ideology_compatible(Ideology const* ideology) const {
    return std::find(ideologies.begin(), ideologies.end(), ideology) != ideologies.end();
}

std::vector<Ideology const*> const& GovernmentType::get_ideologies() const {
    return ideologies;
}

bool GovernmentType::holds_elections() const {
    return elections;
}

bool GovernmentType::can_appoint_ruling_party() const {
    return appoint_ruling_party;
}

Timespan GovernmentType::get_election_duration() const {
    return election_duration;
}

std::string_view GovernmentType::get_flag_type() const {
    return flag_type_identifier;
}

GovernmentTypeManager::GovernmentTypeManager() : government_types { "government types" } {}

bool GovernmentTypeManager::add_government_type(std::string_view identifier, std::vector<Ideology const*> ideologies, bool elections, bool appoint_ruling_party, Timespan election_duration, std::string_view flag_type) {
    if (identifier.empty()) {
        Logger::error("Invalid government type identifier - empty!");
        return false;
    }

    if (ideologies.empty()) {
        Logger::error("No compatible ideologies defined for government type ", identifier);
        return false;
    }

    if (elections && election_duration == 0) {
        Logger::error("No or invalid election duration for government type ", identifier);
        return false;
    }

    return government_types.add_item({ identifier, ideologies, elections, appoint_ruling_party, election_duration, flag_type });
}

/* REQUIREMENTS: FS-525, SIM-27 */
bool GovernmentTypeManager::load_government_types_file(IdeologyManager const& ideology_manager, ast::NodeCPtr root) {
    bool ret = expect_dictionary(
        [this, &ideology_manager](std::string_view government_type_identifier, ast::NodeCPtr value) -> bool {
            std::vector<Ideology const*> ideologies;
            bool elections = false, appoint_ruling_party = false;
            uint16_t election_duration = 0; /* in months */
            std::string_view flag_type_identifier = "republic";

            bool ret = expect_dictionary_keys(
                ALLOW_OTHER_KEYS,
                "election", ONE_EXACTLY, expect_bool(assign_variable_callback(elections)),
                "duration", ZERO_OR_ONE, expect_uint(assign_variable_callback_uint(election_duration)),
                "appoint_ruling_party", ONE_EXACTLY, expect_bool(assign_variable_callback(appoint_ruling_party)),
                "flagType", ZERO_OR_ONE, expect_identifier(assign_variable_callback(flag_type_identifier))
            )(value);

            ret &= expect_dictionary(
                [this, &ideology_manager, &ideologies, government_type_identifier](std::string_view key, ast::NodeCPtr value) -> bool {
                    static const std::set<std::string, std::less<void>> reserved_keys = {
						"election", "duration", "appoint_ruling_party", "flagType"
					};
                    if (reserved_keys.find(key) != reserved_keys.end()) return true;
                    Ideology const* ideology = ideology_manager.get_ideology_by_identifier(key);
                    if (ideology == nullptr) {
                        Logger::error("When loading government type ", government_type_identifier, ", specified ideology ", key, " is invalid!");
                        return false;
                    }
                    ideologies.push_back(ideology);
                    return true;
                }
            )(value);

            ret &= add_government_type(government_type_identifier, ideologies, elections, appoint_ruling_party, Timespan(election_duration * 30), flag_type_identifier);
            return ret;
        }
    )(root);
    lock_government_types();

    return ret;
}