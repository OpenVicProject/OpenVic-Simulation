# snitch honors these as plain variables (CMP0077 NEW via
# openvic_setup_base_flags). Only deltas from upstream defaults are set;
# everything else (SNITCH_DEFINE_MAIN, timings, reporters, ...) already
# matches upstream. Exceptions are additionally forced off at compile time by
# snitch itself when _HAS_EXCEPTIONS=0 / -fno-exceptions is in effect.
#
# NOTE: openvic-dataloader pins an older snitch under the same FetchContent
# name; first declaration wins. The two test suites are never enabled in one
# build by default (tests default ON only for the top-level project), so in
# practice each repo's own pin applies.
set(SNITCH_WITH_EXCEPTIONS OFF)
set(SNITCH_WITH_MULTITHREADING OFF)
openvic_declare_dep(snitch
    REPO snitch-org/snitch
    SHA 2a594333016b9cfc1f90b116184cb0bdbf98a480 # v1.3.1
    SHA256 7512cd7ec34d02faea463e7588ccbcfddc229a7e2243dd49e421b7158741c103
)
FetchContent_MakeAvailable(snitch)

if(OPENVIC_SIM_BUILD_TESTS)
    file(GLOB_RECURSE ovsim_tests_sources CONFIGURE_DEPENDS src/*.cpp)

    add_executable(openvic-simulation-tests ${ovsim_tests_sources})
    target_compile_definitions(openvic-simulation-tests PRIVATE OPENVIC_SIMULATION_TESTS)
    target_include_directories(openvic-simulation-tests PRIVATE src)
    target_link_libraries(openvic-simulation-tests PRIVATE openvic::simulation snitch::snitch)
    set_target_properties(
        openvic-simulation-tests
        PROPERTIES
            OUTPUT_NAME "openvic-simulation.tests"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )

    add_test(NAME openvic-simulation-tests COMMAND openvic-simulation-tests)
endif()

if(OPENVIC_SIM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
