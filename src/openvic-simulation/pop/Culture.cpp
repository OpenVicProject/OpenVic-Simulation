#include "Culture.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GraphicalCultureType::GraphicalCultureType(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

CultureGroup::CultureGroup(
	std::string_view new_identifier, std::string_view new_leader, GraphicalCultureType const& new_unit_graphical_culture_type,
	bool new_is_overseas, CountryDefinition const* new_union_country
) : HasIdentifier { new_identifier }, leader { new_leader }, unit_graphical_culture_type { new_unit_graphical_culture_type },
	is_overseas { new_is_overseas }, union_country { new_union_country } {}

Culture::Culture(
	std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group, name_list_t&& new_first_names,
	name_list_t&& new_last_names, fixed_point_t new_radicalism, CountryDefinition const* new_primary_country
) : HasIdentifierAndColour { new_identifier, new_colour, false }, group { new_group },
	first_names { std::move(new_first_names) }, last_names { std::move(new_last_names) }, radicalism { new_radicalism },
	primary_country { new_primary_country } {}

CultureManager::CultureManager() : default_graphical_culture_type { nullptr } {}

bool CultureManager::add_graphical_culture_type(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return false;
	}
	return graphical_culture_types.add_item({ identifier });
}

bool CultureManager::add_culture_group(
	std::string_view identifier, std::string_view leader, GraphicalCultureType const* graphical_culture_type, bool is_overseas,
	CountryDefinition const* union_country
) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot register culture groups until graphical culture types are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return false;
	}
	if (leader.empty()) {
		Logger::error("Invalid culture group leader - empty!");
		return false;
	}
	if (graphical_culture_type == nullptr) {
		Logger::error("Null graphical culture type for ", identifier);
		return false;
	}

	if (culture_groups.add_item({ identifier, leader, *graphical_culture_type, is_overseas, union_country })) {
		leader_picture_counts.emplace(leader, general_admiral_picture_count_t { 0, 0 });
		return true;
	} else {
		return false;
	}
}

bool CultureManager::add_culture(
	std::string_view identifier, colour_t colour, CultureGroup const& group, name_list_t&& first_names,
	name_list_t&& last_names, fixed_point_t radicalism, CountryDefinition const* primary_country
) {
	if (!culture_groups.is_locked()) {
		Logger::error("Cannot register cultures until culture groups are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture identifier - empty!");
		return false;
	}

	// TODO - check radicalism range

	return cultures.add_item({
		identifier, colour, group, std::move(first_names), std::move(last_names), radicalism, primary_country
	});
}

bool CultureManager::load_graphical_culture_type_file(ast::NodeCPtr root) {
	const bool ret = expect_list_reserve_length(
		graphical_culture_types,
		expect_identifier(std::bind_front(&CultureManager::add_graphical_culture_type, this))
	)(root);

	lock_graphical_culture_types();

	if (graphical_culture_types_empty()) {
		Logger::error("Cannot set default graphical culture type - none loaded!");
		return false;
	}

	/* Last defined graphical culture type is used as default. */
	default_graphical_culture_type = &get_graphical_culture_types().back();

	return ret;
}

bool CultureManager::_load_culture_group(
	CountryDefinitionManager const& country_definition_manager, size_t& total_expected_cultures,
	std::string_view culture_group_key, ast::NodeCPtr culture_group_node
) {
	std::string_view leader {};
	GraphicalCultureType const* unit_graphical_culture_type = default_graphical_culture_type;
	bool is_overseas = true;
	CountryDefinition const* union_country = nullptr;

	bool ret = expect_dictionary_keys_and_default(
		increment_callback(total_expected_cultures),
		"leader", ONE_EXACTLY, expect_identifier(assign_variable_callback(leader)),
		"unit", ZERO_OR_ONE,
			expect_graphical_culture_type_identifier(assign_variable_callback_pointer(unit_graphical_culture_type)),
		"union", ZERO_OR_ONE,
			country_definition_manager.expect_country_definition_identifier(assign_variable_callback_pointer(union_country)),
		"is_overseas", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_overseas))
	)(culture_group_node);
	ret &= add_culture_group(culture_group_key, leader, unit_graphical_culture_type, is_overseas, union_country);
	return ret;
}

bool CultureManager::_load_culture(
	CountryDefinitionManager const& country_definition_manager, CultureGroup const& culture_group, std::string_view culture_key,
	ast::NodeCPtr culture_node
) {
	colour_t colour = colour_t::null();
	name_list_t first_names {}, last_names {};
	fixed_point_t radicalism = 0;
	CountryDefinition const* primary_country = nullptr;

	bool ret = expect_dictionary_keys(
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"first_names", ONE_EXACTLY, name_list_callback(move_variable_callback(first_names)),
		"last_names", ONE_EXACTLY, name_list_callback(move_variable_callback(last_names)),
		"radicalism", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(radicalism)),
		"primary", ZERO_OR_ONE,
			country_definition_manager.expect_country_definition_identifier(assign_variable_callback_pointer(primary_country))
	)(culture_node);
	ret &= add_culture(
		culture_key, colour, culture_group, std::move(first_names), std::move(last_names), radicalism, primary_country
	);
	return ret;
}

/* REQUIREMENTS:
 * POP-59,  POP-60,  POP-61,  POP-62,  POP-63,  POP-64,  POP-65,  POP-66,  POP-67,  POP-68,  POP-69,  POP-70,  POP-71,
 * POP-72,  POP-73,  POP-74,  POP-75,  POP-76,  POP-77,  POP-78,  POP-79,  POP-80,  POP-81,  POP-82,  POP-83,  POP-84,
 * POP-85,  POP-86,  POP-87,  POP-88,  POP-89,  POP-90,  POP-91,  POP-92,  POP-93,  POP-94,  POP-95,  POP-96,  POP-97,
 * POP-98,  POP-99,  POP-100, POP-101, POP-102, POP-103, POP-104, POP-105, POP-106, POP-107, POP-108, POP-109, POP-110,
 * POP-111, POP-112, POP-113, POP-114, POP-115, POP-116, POP-117, POP-118, POP-119, POP-120, POP-121, POP-122, POP-123,
 * POP-124, POP-125, POP-126, POP-127, POP-128, POP-129, POP-130, POP-131, POP-132, POP-133, POP-134, POP-135, POP-136,
 * POP-137, POP-138, POP-139, POP-140, POP-141, POP-142, POP-143, POP-144, POP-145, POP-146, POP-147, POP-148, POP-149,
 * POP-150, POP-151, POP-152, POP-153, POP-154, POP-155, POP-156, POP-157, POP-158, POP-159, POP-160, POP-161, POP-162,
 * POP-163, POP-164, POP-165, POP-166, POP-167, POP-168, POP-169, POP-170, POP-171, POP-172, POP-173, POP-174, POP-175,
 * POP-176, POP-177, POP-178, POP-179, POP-180, POP-181, POP-182, POP-183, POP-184, POP-185, POP-186, POP-187, POP-188,
 * POP-189, POP-190, POP-191, POP-192, POP-193, POP-194, POP-195, POP-196, POP-197, POP-198, POP-199, POP-200, POP-201,
 * POP-202, POP-203, POP-204, POP-205, POP-206, POP-207, POP-208, POP-209, POP-210, POP-211, POP-212, POP-213, POP-214,
 * POP-215, POP-216, POP-217, POP-218, POP-219, POP-220, POP-221, POP-222, POP-223, POP-224, POP-225, POP-226, POP-227,
 * POP-228, POP-229, POP-230, POP-231, POP-232, POP-233, POP-234, POP-235, POP-236, POP-237, POP-238, POP-239, POP-240,
 * POP-241, POP-242, POP-243, POP-244, POP-245, POP-246, POP-247, POP-248, POP-249, POP-250, POP-251, POP-252, POP-253,
 * POP-254, POP-255, POP-256, POP-257, POP-258, POP-259, POP-260, POP-261, POP-262, POP-263, POP-264, POP-265, POP-266,
 * POP-267, POP-268, POP-269, POP-270, POP-271, POP-272, POP-273, POP-274, POP-275, POP-276, POP-277, POP-278, POP-279,
 * POP-280, POP-281, POP-282, POP-283, POP-284
 */
bool CultureManager::load_culture_file(CountryDefinitionManager const& country_definition_manager, ast::NodeCPtr root) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot load culture groups until graphical culture types are locked!");
		return false;
	}

	size_t total_expected_cultures = 0;
	bool ret = expect_dictionary_reserve_length(culture_groups,
		[this, &country_definition_manager, &total_expected_cultures](
			std::string_view key, ast::NodeCPtr value
		) -> bool {
			return _load_culture_group(country_definition_manager, total_expected_cultures, key, value);
		}
	)(root);
	lock_culture_groups();
	reserve_more_cultures(total_expected_cultures);

	ret &= expect_culture_group_dictionary(
		[this, &country_definition_manager](CultureGroup const& culture_group, ast::NodeCPtr culture_group_value) -> bool {
			return expect_dictionary(
				[this, &country_definition_manager, &culture_group](std::string_view key, ast::NodeCPtr value) -> bool {
					static const string_set_t reserved_keys = { "leader", "unit", "union", "is_overseas" };
					if (reserved_keys.contains(key)) {
						return true;
					}
					return _load_culture(country_definition_manager, culture_group, key, value);
				}
			)(culture_group_value);
		}
	)(root);
	lock_cultures();
	return ret;
}

std::string CultureManager::make_leader_picture_name(
	std::string_view cultural_type, UnitType::branch_t branch, leader_count_t count
) {
	if (cultural_type.empty()) {
		Logger::error("Cannot construct leader picture name - empty cultural type!");
		return {};
	}

	static constexpr std::string_view GENERAL_TEXT = "_general_";
	static constexpr std::string_view ADMIRAL_TEXT = "_admiral_";

	std::string_view const* branch_text;

	using enum UnitType::branch_t;

	switch (branch) {
		case LAND:
			branch_text = &GENERAL_TEXT;
			break;
		case NAVAL:
			branch_text = &ADMIRAL_TEXT;
			break;
		default:
			Logger::error("Cannot construct leader picture name - invalid branch type: ", static_cast<uint32_t>(branch));
			return {};
	}

	return StringUtils::append_string_views(cultural_type, *branch_text, std::to_string(count));
}

std::string CultureManager::make_leader_picture_path(std::string_view leader_picture_name) {
	if (leader_picture_name.empty()) {
		Logger::error("Cannot construct leader picture path - empty name!");
		return {};
	}

	static constexpr std::string_view LEADER_PICTURES_DIR = "gfx/interface/leaders/";
	static constexpr std::string_view FILE_EXTENSION = ".dds";

	return StringUtils::append_string_views(LEADER_PICTURES_DIR, leader_picture_name, FILE_EXTENSION);
}

bool CultureManager::find_cultural_leader_pictures(Dataloader const& dataloader) {
	if (!culture_groups_are_locked()) {
		Logger::error("Cannot search for cultural leader pictures until culture groups are locked!");
		return false;
	}

	bool ret = true;

	for (auto [cultural_type, general_and_admiral_count] : mutable_iterator(leader_picture_counts)) {
		const auto search = [&dataloader, &cultural_type, &ret](
			UnitType::branch_t branch, leader_count_t& leader_count
		) -> void {
			while (
				leader_count < std::numeric_limits<leader_count_t>::max() &&
				!dataloader.lookup_file(
					make_leader_picture_path(make_leader_picture_name(cultural_type, branch, leader_count)), false
				).empty()
			) {
				leader_count++;
			}

			if (leader_count < 1) {
				Logger::error(
					"No ", UnitType::get_branched_leader_name(branch), " pictures found for cultural type \"",
					cultural_type, "\"!"
				);
				ret = false;
			}
		};

		using enum UnitType::branch_t;

		search(LAND, general_and_admiral_count.first);
		search(NAVAL, general_and_admiral_count.second);
	}

	return ret;
}

std::string CultureManager::get_leader_picture_name(std::string_view cultural_type, UnitType::branch_t branch) const {
	const decltype(leader_picture_counts)::const_iterator it = leader_picture_counts.find(cultural_type);
	if (it == leader_picture_counts.end()) {
		Logger::error("Cannot find leader picture counts for cultural type \"", cultural_type, "\"!");
		return {};
	}

	leader_count_t desired_picture_count;

	using enum UnitType::branch_t;

	switch (branch) {
	case LAND:
		desired_picture_count = it->second.first;
		break;
	case NAVAL:
		desired_picture_count = it->second.second;
		break;
	default:
		Logger::error(
			"Cannot get \"", cultural_type, "\" leader picture name - invalid branch type: ", static_cast<uint32_t>(branch)
		);
		return {};
	}

	if (desired_picture_count < 1) {
		Logger::error(
			"Cannot get \"", cultural_type, "\" ", UnitType::get_branched_leader_name(branch),
			" picture name - no pictures of this type were found during game loading!"
		);
		return {};
	}

	// This variable determines what index the leader picture name uses. It is static and increments each time it is used,
	// hopefully resulting in a variety of different leader pictures being seen in-game. This is not necessarily a permanent
	// solution, for example it may be replaced with randomly generated integers once our RNG system has been finalised.
	static leader_count_t internal_counter = 0;

	return make_leader_picture_name(cultural_type, branch, internal_counter++ % desired_picture_count);
}
