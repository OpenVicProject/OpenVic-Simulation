# nanobench (the OpenVic fork with exceptions/RTTI disabled): its own
# CMakeLists only supports standalone builds, so fetch source-only and
# hand-roll a static library from the single implementation TU.
openvic_declare_dep(nanobench
    REPO Spartan322/nanobench
    SHA f49b052a68bc05d5497acf1f6c0c2d6ef65c6eef # disable/exceptions-and-rtti
    SHA256 513c81406c5dbc8e1840efbef41a924c7376d0538550bddde9bdf9ee5c62f320
    DOWNLOAD_ONLY
)
FetchContent_MakeAvailable(nanobench)
add_library(ov_nanobench STATIC ${nanobench_SOURCE_DIR}/src/test/app/nanobench.cpp)
target_include_directories(ov_nanobench SYSTEM PUBLIC ${nanobench_SOURCE_DIR}/src/include)

file(GLOB_RECURSE ovsim_benchmarks_sources CONFIGURE_DEPENDS src/*.cpp)

# Benchmarks use snitch's macros too; snitch comes from the parent
# tests/CMakeLists.txt, which always includes this directory.
add_executable(openvic-simulation-benchmarks ${ovsim_benchmarks_sources})
target_compile_definitions(openvic-simulation-benchmarks PRIVATE OPENVIC_SIMULATION_BENCHMARKS)
target_include_directories(openvic-simulation-benchmarks PRIVATE src)
target_link_libraries(openvic-simulation-benchmarks PRIVATE openvic::simulation snitch::snitch ov_nanobench)
set_target_properties(
    openvic-simulation-benchmarks
    PROPERTIES
        OUTPUT_NAME "openvic-simulation.benchmarks"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)
# No add_test: benchmarks are built in CI for compile coverage but run
# manually (they take real time and their numbers are meaningless on shared
# runners).
