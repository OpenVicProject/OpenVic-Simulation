cmake_minimum_required(VERSION 3.28)
project(openvic-simulation LANGUAGES CXX)

if(NOT COMMAND openvic_setup_base_flags)
    # scripts is the shared build-tooling repo, fetched once as a pinned
    # tarball (like the other third-party deps). In a composed build the
    # outermost repo fetches + loads it first and this guard skips for the
    # nested repos; a standalone repo fetches it itself. FetchContent also
    # dedups by name, so either path yields exactly one fetch.
    #
    # Raw FetchContent (not openvic_declare_dep) on purpose: openvic_declare_dep
    # lives INSIDE scripts (OpenVicDeps.cmake), so it isn't defined yet here.
    # Local-dev escape hatch: -DFETCHCONTENT_SOURCE_DIR_OPENVIC_SCRIPTS=<path>
    # points this at a working-tree scripts checkout.
    include(FetchContent)
    FetchContent_Declare(
        openvic_scripts
        URL "https://github.com/OpenVicProject/scripts/archive/3fcac2ec942c152bd83b2c5ad279ef192e0125d7.tar.gz"
        URL_HASH SHA256=c0759a8d5eb8c6a8fa7780f01a6bd2bf079a18ef2527f0c7ec7ecc9b87aed635
    )
    FetchContent_MakeAvailable(openvic_scripts)
    include("${openvic_scripts_SOURCE_DIR}/cmake/OpenVicScripts.cmake")
endif()
openvic_setup_base_flags()
openvic_disable_rtti()

option(OPENVIC_USE_PCH "Precompile openvic-simulation/pch.hpp" ON)

add_subdirectory(deps)

file(GLOB_RECURSE sim_sources CONFIGURE_DEPENDS src/openvic-simulation/*.cpp)
list(REMOVE_ITEM sim_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/openvic-simulation/pch.cpp)

add_library(openvic-simulation STATIC ${sim_sources})
add_library(openvic::simulation ALIAS openvic-simulation)

set(sim_gen_dir ${CMAKE_CURRENT_BINARY_DIR}/gen-include)
target_include_directories(
    openvic-simulation
    PUBLIC src $<BUILD_INTERFACE:${sim_gen_dir}>
    PRIVATE src/openvic-simulation
)

openvic_generate_commit_info(
    TARGET openvic-simulation
    PREFIX sim
    REPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT ${sim_gen_dir}/openvic-simulation/gen/commit_info.gen.hpp
)
openvic_generate_license_info(
    TARGET openvic-simulation
    PREFIX sim
    COPYRIGHT ${CMAKE_CURRENT_SOURCE_DIR}/COPYRIGHT
    LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
    OUTPUT ${sim_gen_dir}/openvic-simulation/gen/license_info.gen.hpp
)
openvic_generate_author_info(
    TARGET openvic-simulation
    PREFIX sim
    AUTHORS ${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS.md
    OUTPUT ${sim_gen_dir}/openvic-simulation/gen/author_info.gen.hpp
    SECTIONS
        "Senior Developers=AUTHORS_SENIOR_DEVELOPERS"
        "Developers=AUTHORS_DEVELOPERS"
        "Contributors=AUTHORS_CONTRIBUTORS"
        "Consultants=AUTHORS_CONSULTANTS"
)

if(OPENVIC_USE_PCH)
    target_precompile_headers(openvic-simulation PRIVATE src/openvic-simulation/pch.hpp)
endif()

# PUBLIC deps are exposed to consumers (everything except lexy, which is an
# implementation detail of the parsers and stays private).
target_link_libraries(
    openvic-simulation
    PUBLIC
        openvic::dataloader
        openvic::lexy-vdf
        tsl::ordered_map
        function2::function2
        type_safe
        Boost::int128
        spdlog::spdlog
        ov_plf_colony
        ov_std_function
        ov_xoshiro
        ov_foonathan_memory
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TBB REQUIRED IMPORTED_TARGET tbb)
    target_link_libraries(openvic-simulation PUBLIC PkgConfig::TBB)
endif()

if(APPLE)
    # clang emits the TLS init aliases (_ZTH*) for inline thread_local members
    # as non-weak private-external symbols, one per TU that odr-uses them, and
    # the Xcode 26 linker rejects the duplicates unless it runs with
    # -dead_strip (the old scons build always linked with it). INTERFACE so
    # every consumer linking this archive gets the flag.
    target_link_options(openvic-simulation INTERFACE LINKER:-dead_strip)
elseif(MINGW)
    # GCC on PE has the same problem: the TLS init function for those inline
    # thread_local members is emitted non-COMDAT in every TU, and GNU ld
    # errors on the duplicates. They are identical instantiations, so keep
    # the first.
    target_link_options(openvic-simulation INTERFACE LINKER:--allow-multiple-definition)
endif()

set_target_properties(openvic-simulation PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

# Headless executable, unit tests, and benchmarks: built by default only when
# openvic-simulation is the top-level project (composed builds just want the
# library). Benchmarks are opt-in even standalone, matching the old default.
option(OPENVIC_SIM_BUILD_HEADLESS "Build the openvic-simulation headless executable" ${PROJECT_IS_TOP_LEVEL})
option(OPENVIC_SIM_BUILD_TESTS "Build the openvic-simulation unit tests" ${PROJECT_IS_TOP_LEVEL})
option(OPENVIC_SIM_BUILD_BENCHMARKS "Build the openvic-simulation benchmarks" OFF)

if(OPENVIC_SIM_BUILD_HEADLESS)
    file(GLOB_RECURSE ovsim_headless_sources CONFIGURE_DEPENDS src/headless/*.cpp)
    add_executable(openvic-simulation-headless ${ovsim_headless_sources})
    target_compile_definitions(openvic-simulation-headless PRIVATE OPENVIC_SIM_HEADLESS)
    target_include_directories(openvic-simulation-headless PRIVATE src/headless)
    target_link_libraries(openvic-simulation-headless PRIVATE openvic::simulation)
    set_target_properties(
        openvic-simulation-headless
        PROPERTIES
            OUTPUT_NAME "openvic-simulation.headless"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )
endif()

if(OPENVIC_SIM_BUILD_TESTS OR OPENVIC_SIM_BUILD_BENCHMARKS)
    enable_testing()
    add_subdirectory(tests)
endif()
