#include "Government.hpp"

#include <set>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/GameManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GovernmentType::GovernmentType(std::string_view new_identifier, std::vector<std::string> new_ideologies, bool new_elections, bool new_appoint_ruling_party, uint16_t new_election_duration, std::string_view new_flag_type_identifier)
    : HasIdentifier { new_identifier }, ideologies { new_ideologies }, elections { new_elections }, appoint_ruling_party { new_appoint_ruling_party }, election_duration { new_election_duration }, flag_type_identifier { new_flag_type_identifier } {}

bool GovernmentType::is_ideology_compatible(std::string_view ideology) const {
    return std::find(ideologies.begin(), ideologies.end(), ideology) != ideologies.end();
}

bool GovernmentType::holds_elections() const {
    return elections;
}

bool GovernmentType::can_appoint_ruling_party() const {
    return appoint_ruling_party;
}

uint16_t GovernmentType::get_election_duration() const {
    return election_duration;
}

std::string_view GovernmentType::get_flag_type() const {
    return flag_type_identifier;
}

GovernmentTypeManager::GovernmentTypeManager() : government_types { "government types" } {}

bool GovernmentTypeManager::add_government_type(std::string_view identifier, std::vector<std::string> ideologies, bool elections, bool appoint_ruling_party, uint16_t election_duration, std::string_view flag_type) {
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

bool GovernmentTypeManager::load_government_types_file(IdeologyManager& ideology_manager, ast::NodeCPtr root) {
    bool ret = expect_dictionary(
        [this, &ideology_manager](std::string_view government_type_identifier, ast::NodeCPtr value) -> bool {
            std::vector<std::string> ideologies;
            bool elections = false, appoint_ruling_party = false;
            uint16_t election_duration = 0;
            std::string_view flag_type_identifier = "republic";

            bool ret = expect_dictionary_keys_and_length(
                default_length_callback,
                ALLOW_OTHER_KEYS,
                "election", ONE_EXACTLY, expect_bool(assign_variable_callback(elections)),
                "duration", ZERO_OR_ONE, expect_uint(assign_variable_callback_uint(election_duration)),
                "appoint_ruling_party", ONE_EXACTLY, expect_bool(assign_variable_callback(appoint_ruling_party)),
                "flagType", ZERO_OR_ONE, expect_identifier(assign_variable_callback(flag_type_identifier))
            )(value);

            ret &= expect_dictionary(
                [this, &ideology_manager, &ideologies](std::string_view key, ast::NodeCPtr value) -> bool {
                    static const std::set<std::string, std::less<void>> reserved_keys = {
						"election", "duration", "appoint_ruling_party", "flagType"
					};
                    if (reserved_keys.find(key) != reserved_keys.end()) return true;
                    return expect_assign(
                        [this, &ideology_manager, &ideologies](std::string_view ideology_key, ast::NodeCPtr value) -> bool {
                            if (ideology_manager.get_ideology_by_identifier(ideology_key) == nullptr) { return false; }
                            ideologies.push_back(std::string{ideology_key});
                            return true;
                        }
                    )(value);
                }
            )(value);

            ret &= add_government_type(government_type_identifier, ideologies, elections, appoint_ruling_party, election_duration, flag_type_identifier);
            return ret;
        }
    )(root);
    lock_government_types();

    return ret;
}