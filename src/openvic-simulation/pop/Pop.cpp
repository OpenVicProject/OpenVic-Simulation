#include "utility/Utility.hpp"
#define KEEP_DO_FOR_ALL_TYPES_OF_INCOME
#define KEEP_DO_FOR_ALL_TYPES_OF_EXPENSES
#include "Pop.hpp"
#undef KEEP_DO_FOR_ALL_TYPES_OF_INCOME
#undef KEEP_DO_FOR_ALL_TYPES_OF_EXPENSES

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducer.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerFactoryPattern.hpp"

using namespace OpenVic;

static constexpr fixed_point_t DEFAULT_POP_LITERACY = fixed_point_t::_0_10();

PopBase::PopBase(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
	fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
) : type { &new_type }, culture { new_culture }, religion { new_religion }, size { new_size }, militancy { new_militancy },
	consciousness { new_consciousness }, rebel_type { new_rebel_type } {}

Pop::Pop(
	PopBase const& pop_base,
	decltype(ideology_distribution)::keys_type const& ideology_keys,
	ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
)
  : PopBase { pop_base },
	artisanal_producer_nullable {
		type->get_is_artisan()
			? artisanal_producer_factory_pattern.CreateNewArtisanalProducer()
			: nullptr
	},
	location { nullptr },
	total_change { 0 },
	num_grown { 0 },
	num_promoted { 0 },
	num_demoted { 0 },
	num_migrated_internal { 0 },
	num_migrated_external { 0 },
	num_migrated_colonial { 0 },
	literacy { DEFAULT_POP_LITERACY },
	ideology_distribution { &ideology_keys },
	issue_distribution {},
	vote_distribution { nullptr },
	unemployment { 0 },
	cash { 0 },
	income { 0 },
	expenses { 0 },
	savings { 0 },
	life_needs_fulfilled { 0 },
	everyday_needs_fulfilled { 0 },
	luxury_needs_fulfilled { 0 },
	#define INITALIZE_POP_MONEY_STORES(name)\
		name { 0 },

	DO_FOR_ALL_TYPES_OF_POP_INCOME(INITALIZE_POP_MONEY_STORES)
	DO_FOR_ALL_TYPES_OF_POP_EXPENSES(INITALIZE_POP_MONEY_STORES)
	#undef INITALIZE_POP_MONEY_STORES
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
	ideology_distribution.clear();
	for (Ideology const& ideology : *ideology_distribution.get_keys()) {
		test_weight(ideology_distribution, ideology, 1, 5);
	}
	ideology_distribution.rescale(size);

	issue_distribution.clear();
	for (Issue const& issue : issue_manager.get_issues()) {
		test_weight(issue_distribution, issue, 3, 6);
	}
	for (Reform const& reform : issue_manager.get_reforms()) {
		if (!reform.get_reform_group().get_type().is_uncivilised()) {
			test_weight(issue_distribution, reform, 3, 6);
		}
	}
	rescale_fixed_point_map(issue_distribution, size);

	if (vote_distribution.has_keys()) {
		vote_distribution.clear();
		for (CountryParty const& party : *vote_distribution.get_keys()) {
			test_weight(vote_distribution, party, 4, 10);
		}
		vote_distribution.rescale(size);
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

		update_location_based_attributes();
	}
}

void Pop::update_location_based_attributes() {
	if (location != nullptr) {
		CountryInstance const* owner = location->get_owner();

		if (owner != nullptr) {
			vote_distribution.set_keys(&owner->get_country_definition()->get_parties());

			// TODO - calculate vote distribution

			return;
		}
	}

	vote_distribution.set_keys(nullptr);
}

fixed_point_t Pop::get_ideology_support(Ideology const& ideology) const {
	return ideology_distribution[ideology];
}

fixed_point_t Pop::get_issue_support(Issue const& issue) const {
	const decltype(issue_distribution)::const_iterator it = issue_distribution.find(&issue);

	if (it != issue_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t Pop::get_party_support(CountryParty const& party) const {
	if (vote_distribution.has_keys()) {
		return vote_distribution[party];
	} else {
		return fixed_point_t::_0();
	}
}

void Pop::update_gamestate(
	DefineManager const& define_manager, CountryInstance const* owner, const fixed_point_t pop_size_per_regiment_multiplier
) {
	static constexpr fixed_point_t MIN_MILITANCY = fixed_point_t::_0();
	static constexpr fixed_point_t MAX_MILITANCY = fixed_point_t::_10();
	static constexpr fixed_point_t MIN_CONSCIOUSNESS = fixed_point_t::_0();
	static constexpr fixed_point_t MAX_CONSCIOUSNESS = fixed_point_t::_10();
	static constexpr fixed_point_t MIN_LITERACY = fixed_point_t::_0_01();
	static constexpr fixed_point_t MAX_LITERACY = fixed_point_t::_1();

	militancy = std::clamp(militancy, MIN_MILITANCY, MAX_MILITANCY);
	consciousness = std::clamp(consciousness, MIN_CONSCIOUSNESS, MAX_CONSCIOUSNESS);
	literacy = std::clamp(literacy, MIN_LITERACY, MAX_LITERACY);

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

std::stringstream Pop::get_pop_context_text() const {
	std::stringstream pop_context {};
	pop_context << " location: ";
	if (OV_unlikely(location == nullptr)) {
		pop_context << "NULL";
	} else {
		pop_context << location->get_identifier();
	}
	pop_context << " type: " << type << " culture: " << culture << " religion: " << religion << " size: " << size;
	return pop_context;
}

#define DEFINE_ADD_INCOME_FUNCTIONS(name)\
	void Pop::add_##name(const fixed_point_t amount){\
		if (OV_unlikely(amount == 0)) { \
			Logger::warning("Adding ",#name, " of 0 to pop. Context", get_pop_context_text().str()); \
			return; \
		} \
		if (OV_unlikely(amount < 0)) { \
			Logger::error("Adding negative ", #name, " of ",amount," to pop. Context", get_pop_context_text().str()); \
			return; \
		} \
		name += amount;\
		income += amount;\
		cash += amount;\
	}

DO_FOR_ALL_TYPES_OF_POP_INCOME(DEFINE_ADD_INCOME_FUNCTIONS)
#undef DEFINE_ADD_INCOME_FUNCTIONS

#define DEFINE_ADD_EXPENSE_FUNCTIONS(name)\
	void Pop::add_##name(const fixed_point_t amount){\
		if (OV_unlikely(amount == 0)) { \
			Logger::warning("Adding ",#name, " of 0 to pop. Context", get_pop_context_text().str()); \
			return; \
		} \
		name += amount;\
		expenses += amount;\
		if (OV_unlikely(expenses < 0)) { \
			Logger::error("Total expenses became negative (",expenses,") after adding ", #name, " of ",amount," to pop. Context", get_pop_context_text().str()); \
		} \
		cash -= amount;\
		if (OV_unlikely(cash < 0)) { \
			Logger::error("Total cash became negative (",cash,") after adding ", #name, " of ",amount," to pop. Context", get_pop_context_text().str()); \
		} \
	}

DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DEFINE_ADD_EXPENSE_FUNCTIONS)
#undef DEFINE_ADD_EXPENSE_FUNCTIONS

#define SET_ALL_INCOME_TO_ZERO(name)\
	name = fixed_point_t::_0();

void Pop::pop_tick() {
	DO_FOR_ALL_TYPES_OF_POP_INCOME(SET_ALL_INCOME_TO_ZERO)
	#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
	#undef SET_ALL_INCOME_TO_ZERO

	if (artisanal_producer_nullable != nullptr) {
		artisanal_producer_nullable->artisan_tick(*this);
	}
}

#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
#undef DO_FOR_ALL_TYPES_OF_POP_EXPENSES