#include "openvic-simulation/ecs/SystemTypeID.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct SidA {};
	struct SidB {};
	struct SidA_dup {}; // same name as SidA below — should hash to the same id

	// Two systems with deliberately different names → different ids.
}

ECS_SYSTEM(SidA)
ECS_SYSTEM(SidB)

// Re-specialise SystemName for SidA_dup with the SAME literal as SidA to verify hash
// equality across types. (This is technically a violation of the "globally unique"
// contract, but proves the hash function is purely a function of the literal.)
namespace OpenVic::ecs {
	template<>
	struct SystemName<SidA_dup> {
		static constexpr std::string_view value = "SidA";
	};
}

TEST_CASE("system_type_id_of is FNV-1a stable", "[ecs][SystemTypeID]") {
	CONSTEXPR_CHECK(system_type_id_of<SidA>() != 0);
	CONSTEXPR_CHECK(system_type_id_of<SidA>() != system_type_id_of<SidB>());
}

TEST_CASE("Same name literal yields same id across types", "[ecs][SystemTypeID]") {
	CHECK(system_type_id_of<SidA>() == system_type_id_of<SidA_dup>());
}

TEST_CASE("ECS_SYSTEM macro stringifies its argument", "[ecs][SystemTypeID]") {
	CHECK(SystemName<SidA>::value == "SidA");
	CHECK(SystemName<SidB>::value == "SidB");
}
