#include "Government.hpp"

#include "openvic-simulation/utility/LogScope.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GovernmentType::GovernmentType(
	std::string_view new_identifier, std::vector<Ideology const*>&& new_ideologies, bool new_elections,
	bool new_appoint_ruling_party, Timespan new_term_duration, std::string_view new_flag_type_identifier
) : HasIdentifier { new_identifier }, ideologies { std::move(new_ideologies) }, elections { new_elections },
	appoint_ruling_party { new_appoint_ruling_party }, term_duration { new_term_duration },
	flag_type_identifier { new_flag_type_identifier } {}

bool GovernmentType::is_ideology_compatible(Ideology const* ideology) const {
	return std::find(ideologies.begin(), ideologies.end(), ideology) != ideologies.end();
}

bool GovernmentTypeManager::add_government_type(
	std::string_view identifier, std::vector<Ideology const*>&& ideologies, bool elections, bool appoint_ruling_party,
	Timespan term_duration, std::string_view flag_type
) {
	const LogScope log_scope { fmt::format("government type {}", identifier) };
	if (identifier.empty()) {
		Logger::error("Invalid government type identifier - empty!");
		return false;
	}

	if (ideologies.empty()) {
		Logger::error("No compatible ideologies defined for government type ", identifier);
		return false;
	}

	if (elections && term_duration < 0) {
		Logger::error("No or invalid term duration for government type ", identifier);
		return false;
	}

	const bool ret = government_types.add_item({
		identifier, std::move(ideologies), elections, appoint_ruling_party, term_duration, flag_type
	});

	/* flag_type can be empty here for default/non-ideological flag */
	if (ret) {
		flag_types.emplace(flag_type);
	}

	return ret;
}

/* REQUIREMENTS: FS-525, SIM-27 */
bool GovernmentTypeManager::load_government_types_file(IdeologyManager const& ideology_manager, ast::NodeCPtr root) {
	const LogScope log_scope { "common/governments.txt" };
	bool ret = expect_dictionary_reserve_length(
		government_types,
		[this, &ideology_manager](std::string_view government_type_identifier, ast::NodeCPtr government_type_value) -> bool {
			std::vector<Ideology const*> ideologies;
			bool elections = false, appoint_ruling_party = false;
			Timespan term_duration = 0;
			std::string_view flag_type_identifier;

			size_t total_expected_ideologies = 0;
			bool ret = expect_dictionary_keys_and_default(
				increment_callback(total_expected_ideologies),
				"election", ZERO_OR_ONE, expect_bool(assign_variable_callback(elections)),
				"duration", ZERO_OR_ONE, expect_months(assign_variable_callback(term_duration)),
				"appoint_ruling_party", ONE_EXACTLY, expect_bool(assign_variable_callback(appoint_ruling_party)),
				"flagType", ZERO_OR_ONE, expect_identifier(assign_variable_callback(flag_type_identifier))
			)(government_type_value);
			ideologies.reserve(total_expected_ideologies);

			ret &= expect_dictionary(
				[this, &ideology_manager, &ideologies, government_type_identifier](
					std::string_view key, ast::NodeCPtr value
				) -> bool {
					static const string_set_t reserved_keys = { "election", "duration", "appoint_ruling_party", "flagType" };
					if (reserved_keys.contains(key)) {
						return true;
					}
					Ideology const* ideology = ideology_manager.get_ideology_by_identifier(key);
					if (ideology == nullptr) {
						Logger::error(
							"When loading government type ", government_type_identifier, ", specified ideology ", key,
							" is invalid!"
						);
						return false;
					}
					return expect_bool([&ideologies, ideology, government_type_identifier](bool val) -> bool {
						if (val) {
							if (std::find(ideologies.begin(), ideologies.end(), ideology) == ideologies.end()) {
								ideologies.push_back(ideology);
								return true;
							}
							Logger::error(
								"Government type ", government_type_identifier, " marked as supporting ideology ",
								ideology->get_identifier()
							);
							return false;
						}
						Logger::error(
							"Government type ", government_type_identifier, " redundantly marked as not supporting ideology ",
							ideology->get_identifier(), " multiple times"
						);
						return false;
					})(value);
				}
			)(government_type_value);

			ret &= add_government_type(
				government_type_identifier, std::move(ideologies), elections, appoint_ruling_party, term_duration,
				flag_type_identifier
			);
			return ret;
		}
	)(root);
	lock_government_types();

	return ret;
}

bool GovernmentTypeManager::is_valid_flag_type(std::string_view type) const {
	return std::any_of(flag_types.begin(), flag_types.end(), [type](std::string const& flag_type) -> bool {
		return flag_type == type;
	});
}
