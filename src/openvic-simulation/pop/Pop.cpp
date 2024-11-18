#define KEEP_DO_FOR_ALL_TYPES_OF_INCOME
#include "Pop.hpp"
#undef KEEP_DO_FOR_ALL_TYPES_OF_INCOME

#include "openvic-simulation/DefinitionManager.hpp"

using namespace OpenVic;

PopBase::PopBase(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
	fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
) : type { &new_type }, culture { new_culture }, religion { new_religion }, size { new_size }, militancy { new_militancy },
	consciousness { new_consciousness }, rebel_type { new_rebel_type } {}

Pop::Pop(PopBase const& pop_base, decltype(ideologies)::keys_t const& ideology_keys)
  : PopBase { pop_base },
	location { nullptr },
	total_change { 0 },
	num_grown { 0 },
	num_promoted { 0 },
	num_demoted { 0 },
	num_migrated_internal { 0 },
	num_migrated_external { 0 },
	num_migrated_colonial { 0 },
	ideologies { &ideology_keys },
	issues {},
	votes { nullptr },
	unemployment { 0 },
	cash { 0 },
	income { 0 },
	expenses { 0 },
	savings { 0 },
	life_needs_fulfilled { 0 },
	everyday_needs_fulfilled { 0 },
	luxury_needs_fulfilled { 0 },
	#define INITALIZE_POP_INCOME_STORES(name)\
		name { 0 },

	DO_FOR_ALL_TYPES_OF_POP_INCOME(INITALIZE_POP_INCOME_STORES)
	#undef INITALIZE_POP_INCOME_STORES
	max_supported_regiments { 0 } {}

void Pop::setup_pop_test_values(IssueManager const& issue_manager) {
	/* Returns +/- range% of size. */
	const auto test_size = [this](int32_t range) -> pop_size_t {
		return size * ((rand() % (2 * range + 1)) - range) / 100;
	};

	num_grown = test_size(5);
	num_promoted = test_size(1);
	num_demoted = test_size(1);
	num_migrated_internal = test_size(3);
	num_migrated_external = test_size(1);
	num_migrated_colonial = test_size(2);

	total_change =
		num_grown + num_promoted + num_demoted + num_migrated_internal + num_migrated_external + num_migrated_colonial;

	/* Generates a number between 0 and max (inclusive) and sets map[&key] to it if it's at least min. */
	auto test_weight =
		[]<typename T, typename U>(T& map, U const& key, int32_t min, int32_t max) -> void {
			const int32_t value = rand() % (max + 1);
			if (value >= min) {
				if constexpr (utility::is_specialization_of_v<T, IndexedMap>) {
					map[key] = value;
				} else {
					map.emplace(&key, value);
				}
			}
		};

	/* All entries equally weighted for testing. */
	ideologies.clear();
	for (Ideology const& ideology : *ideologies.get_keys()) {
		test_weight(ideologies, ideology, 1, 5);
	}
	ideologies.normalise();

	issues.clear();
	for (Issue const& issue : issue_manager.get_issues()) {
		test_weight(issues, issue, 3, 6);
	}
	for (Reform const& reform : issue_manager.get_reforms()) {
		if (!reform.get_reform_group().get_type().is_uncivilised()) {
			test_weight(issues, reform, 3, 6);
		}
	}
	normalise_fixed_point_map(issues);

	if (votes.has_keys()) {
		votes.clear();
		for (CountryParty const& party : *votes.get_keys()) {
			test_weight(votes, party, 4, 10);
		}
		votes.normalise();
	}

	/* Returns a fixed point between 0 and max. */
	const auto test_range = [](fixed_point_t max = 1) -> fixed_point_t {
		return (rand() % 256) * max / 256;
	};

	unemployment = test_range();
	cash = test_range(20);
	income = test_range(5);
	expenses = test_range(5);
	savings = test_range(15);
	life_needs_fulfilled = test_range();
	everyday_needs_fulfilled = test_range();
	luxury_needs_fulfilled = test_range();
}

bool Pop::convert_to_equivalent() {
	PopType const* const equivalent = get_type()->get_equivalent();
	if (equivalent == nullptr) {
		Logger::error("Tried to convert pop of type ", get_type()->get_identifier(), " to equivalent, but there is no equivalent.");
		return false;
	}
	
	type = equivalent;
	return true;
}

void Pop::set_location(ProvinceInstance const& new_location) {
	if (location != &new_location) {
		location = &new_location;

		// TODO - update location dependent attributes

		votes.set_keys(
			location->get_owner() != nullptr ? &location->get_owner()->get_country_definition()->get_parties() : nullptr
		);
		// TODO - calculate vote distribution
	}
}

void Pop::update_gamestate(
	DefineManager const& define_manager, CountryInstance const* owner, const fixed_point_t pop_size_per_regiment_multiplier
) {
	if (type->get_can_be_recruited()) {
		MilitaryDefines const& military_defines = define_manager.get_military_defines();

		if (
			size < military_defines.get_min_pop_size_for_regiment() || owner == nullptr ||
			!RegimentType::allowed_cultures_check_culture_in_country(owner->get_allowed_regiment_cultures(), culture, *owner)
		) {
			max_supported_regiments = 0;
		} else {
			max_supported_regiments = (fixed_point_t::parse(size) / (
				fixed_point_t::parse(military_defines.get_pop_size_per_regiment()) * pop_size_per_regiment_multiplier
			)).to_int64_t() + 1;
		}
	}
}

#define DEFINE_ADD_INCOME_FUNCTIONS(name)\
	void Pop::add_##name(const fixed_point_t pop_income){\
		name += pop_income;\
		income += pop_income;\
	}

DO_FOR_ALL_TYPES_OF_POP_INCOME(DEFINE_ADD_INCOME_FUNCTIONS)
#undef DEFINE_ADD_INCOME_FUNCTIONS

#define SET_ALL_INCOME_TO_ZERO(name)\
	name = fixed_point_t::_0();

void Pop::clear_all_income(){
	DO_FOR_ALL_TYPES_OF_POP_INCOME(SET_ALL_INCOME_TO_ZERO)
	#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
	#undef SET_ALL_INCOME_TO_ZERO
}