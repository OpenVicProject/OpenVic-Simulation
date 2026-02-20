#include "Government.hpp"

#include "openvic-simulation/politics/Ideology.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GovernmentType::GovernmentType(
	index_t new_index,
	std::string_view new_identifier,
	memory::vector<Ideology const*>&& new_ideologies,
	bool new_holds_elections,
	bool new_can_appoint_ruling_party,
	Timespan new_term_duration,
	std::string_view new_flag_type_identifier
) : HasIndex { new_index },
	HasIdentifier { new_identifier },
	ideologies { std::move(new_ideologies) },
	holds_elections { new_holds_elections },
	can_appoint_ruling_party { new_can_appoint_ruling_party },
	term_duration { new_term_duration },
	flag_type_identifier { new_flag_type_identifier } {}

bool GovernmentType::is_ideology_compatible(Ideology const* ideology) const {
	return std::find(ideologies.begin(), ideologies.end(), ideology) != ideologies.end();
}

bool GovernmentTypeManager::add_government_type(
	std::string_view identifier, memory::vector<Ideology const*>&& ideologies, bool elections, bool appoint_ruling_party,
	Timespan term_duration, std::string_view flag_type
) {
	spdlog::scope scope { fmt::format("government type {}", identifier) };
	if (identifier.empty()) {
		spdlog::error_s("Invalid government type identifier - empty!");
		return false;
	}

	if (ideologies.empty()) {
		spdlog::error_s("No compatible ideologies defined for government type {}", identifier);
		return false;
	}

	if (elections && term_duration < 0) {
		spdlog::error_s("No or invalid term duration for government type {}", identifier);
		return false;
	}

	const bool ret = government_types.emplace_item(
		identifier,
		GovernmentType::index_t { get_government_type_count() }, identifier, std::move(ideologies), elections, appoint_ruling_party, term_duration, flag_type
	);

	/* flag_type can be empty here for default/non-ideological flag */
	if (ret) {
		flag_types.emplace(flag_type);
	}

	return ret;
}

/* REQUIREMENTS: FS-525, SIM-27 */
bool GovernmentTypeManager::load_government_types_file(IdeologyManager const& ideology_manager, ast::NodeCPtr root) {
	spdlog::scope scope { "common/governments.txt" };
	bool ret = expect_dictionary_reserve_length(
		government_types,
		[this, &ideology_manager](std::string_view government_type_identifier, ast::NodeCPtr government_type_value) -> bool {
			memory::vector<Ideology const*> ideologies;
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
						spdlog::error_s(
							"When loading government type {}, specified ideology {} is invalid!",
							government_type_identifier, key
						);
						return false;
					}
					return expect_bool([&ideologies, ideology, government_type_identifier](bool val) -> bool {
						if (val) {
							if (std::find(ideologies.begin(), ideologies.end(), ideology) == ideologies.end()) {
								ideologies.push_back(ideology);
								return true;
							}
							spdlog::error_s(
								"Government type {} marked as supporting ideology {}",
								government_type_identifier, *ideology
							);
							return false;
						}
						spdlog::error_s(
							"Government type {} redundantly marked as not supporting ideology {} multiple times",
							government_type_identifier, *ideology
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
	return std::any_of(flag_types.begin(), flag_types.end(), [type](memory::string const& flag_type) -> bool {
		return flag_type == type;
	});
}
