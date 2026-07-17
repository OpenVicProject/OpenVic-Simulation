# Dependency wiring for the simulation. Third-party deps are fetched as
# pinned source tarballs (see scripts/cmake/OpenVicDeps.cmake) so they build
# out of the build tree's _deps/ dir, immune to working-tree churn.
# They keep EXCLUDE_FROM_ALL (via openvic_declare_dep) so their install rules
# stay out of `cmake --install` (the only install rule in the whole build is
# the OpenVic root's copy of the extension library).

# --- OpenVic libraries (first-party: own CMakeLists, composed from submodule
# so they rebuild on branch switches) ---
# openvic-dataloader first: its vendored lexy claims the lexy target names,
# so lexy-vdf reuses it (first FetchContent declaration wins).
add_subdirectory(openvic-dataloader)
add_subdirectory(lexy-vdf)

# --- header-only deps with upstream CMake ---
openvic_declare_dep(ordered-map
    REPO Tessil/ordered-map
    SHA bd8d5ef4149cd40783a486011778a2e7eedde441
    SHA256 39f458bf3808b2a83371580bcbb48b0336fbaa95cb1228502346f934a4f5d49e
)
FetchContent_MakeAvailable(ordered-map) # tsl::ordered_map

openvic_declare_dep(function2
    REPO Naios/function2
    SHA 43fc0ca473ecb081918709bd7d524d84c2ff8dce
    SHA256 7bd09f73b1d0de46885e117e9ef094ade2c771917d126e13451e3cbb9b228c20
)
FetchContent_MakeAvailable(function2) # function2::function2

openvic_declare_dep(int128
    REPO cppalliance/int128
    SHA ea0f85e693767ae667e26d66022d6ed68d3425de
    SHA256 b0685286c9b5ff29e7f94772dbf9600b865dcdcf8da96009c553a1d1da23594a
)
FetchContent_MakeAvailable(int128) # Boost::int128

# --- header-only deps without upstream CMake: fetch source-only, hand-roll
# INTERFACE targets (${<name>_SOURCE_DIR} is the extracted tarball root) ---
openvic_declare_dep(plf_colony
    REPO mattreecebentley/plf_colony
    SHA fbc8483fb785c431dc4410396815abf886728c40
    SHA256 d7006bebdc42694491d11e2d4d95417848a5fa2fff2caade2b123bb99479d098
    DOWNLOAD_ONLY
)
FetchContent_MakeAvailable(plf_colony)
add_library(ov_plf_colony INTERFACE)
target_include_directories(ov_plf_colony SYSTEM INTERFACE ${plf_colony_SOURCE_DIR})

# Consumers include std_function as <func/function.h>. Extract into a `func/`
# subdir and point the include dir at its parent to preserve that prefix.
openvic_declare_dep(std_function
    REPO skarupke/std_function
    SHA dbf2df963e6d0a47ab5e21e93c838233cd8b6b03
    SHA256 b0f2d651ca6e0d4c112434ceab78785b4f5e19422821f78c19c57e77a84f7935
    DOWNLOAD_ONLY
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/std_function-src/func
)
FetchContent_MakeAvailable(std_function)
add_library(ov_std_function INTERFACE)
get_filename_component(ov_std_function_include "${std_function_SOURCE_DIR}" DIRECTORY)
target_include_directories(ov_std_function SYSTEM INTERFACE ${ov_std_function_include})

openvic_declare_dep(xoshiro
    REPO reputeless/xoshiro-cpp
    SHA 19bcbb2ce0ed158233187f524fd0964c105a65b3
    SHA256 973884cfb77f2f979f9ac09e8846dfd758af4bc08422b7c92a0e676a3fb780ff
    DOWNLOAD_ONLY
)
FetchContent_MakeAvailable(xoshiro)
add_library(ov_xoshiro INTERFACE)
target_include_directories(ov_xoshiro SYSTEM INTERFACE ${xoshiro_SOURCE_DIR})

# --- type_safe (assertions/wrapper enabled only on template_release;
# arithmetic policy defaults to checked for dev builds, ub otherwise) ---
if(GODOTCPP_TARGET STREQUAL "template_release")
    set(TYPE_SAFE_ENABLE_ASSERTIONS ON)
    set(TYPE_SAFE_ENABLE_WRAPPER ON)
else()
    set(TYPE_SAFE_ENABLE_ASSERTIONS OFF)
    set(TYPE_SAFE_ENABLE_WRAPPER OFF)
endif()
set(TYPE_SAFE_ENABLE_PRECONDITION_CHECKS ON)
set(OPENVIC_TYPE_SAFE_ARITHMETIC_POLICY
    ""
    CACHE STRING
    "type_safe arithmetic policy: ub, checked, or default (empty = auto: checked for dev builds, ub otherwise)"
)
set_property(CACHE OPENVIC_TYPE_SAFE_ARITHMETIC_POLICY PROPERTY STRINGS ";ub;checked;default")
if(OPENVIC_TYPE_SAFE_ARITHMETIC_POLICY STREQUAL "")
    if(GODOTCPP_DEV_BUILD)
        set(TYPE_SAFE_ARITHMETIC_POLICY "checked")
    else()
        set(TYPE_SAFE_ARITHMETIC_POLICY "ub")
    endif()
elseif(OPENVIC_TYPE_SAFE_ARITHMETIC_POLICY MATCHES "^(ub|checked|default)$")
    set(TYPE_SAFE_ARITHMETIC_POLICY "${OPENVIC_TYPE_SAFE_ARITHMETIC_POLICY}")
else()
    message(FATAL_ERROR "OPENVIC_TYPE_SAFE_ARITHMETIC_POLICY must be ub, checked, default, or empty")
endif()
openvic_declare_dep(type_safe
    REPO foonathan/type_safe
    SHA 292e8c127037e33d92f8f52dab0f1184993942d0
    SHA256 622337e530b5812fd65ef91bc6df580946b2ac8b0ba865253d1dad80394e88df
)
FetchContent_MakeAvailable(type_safe) # type_safe (+ vendored external/debug_assert)

# --- foonathan/memory: hand-rolled. Upstream's CMake bakes container node
# sizes at configure time via try_compile (wrong for macOS universal builds);
# instead we fetch source-only and generate the config and node-size headers
# into the fetched tree ourselves (see OpenVicCodegen.cmake). ---
openvic_declare_dep(memory
    REPO foonathan/memory
    SHA 92a5bfc53b97955b3d685af679636e87d0f5d15a
    SHA256 8601f99ec73a2f84a16447f3e320b76c62227530217c6bf2fff44aece67d110e
    DOWNLOAD_ONLY
)
FetchContent_MakeAvailable(memory)

openvic_generate_memory_headers(
    MEMORY_DIR ${memory_SOURCE_DIR}
    DEFINES
        FOONATHAN_MEMORY_CHECK_ALLOCATION_SIZE=1
        FOONATHAN_MEMORY_IMPL_DEFAULT_ALLOCATOR=heap_allocator
        FOONATHAN_MEMORY_DEBUG_ASSERT=0
        FOONATHAN_MEMORY_DEBUG_FILL=0
        FOONATHAN_MEMORY_DEBUG_FENCE=0
        FOONATHAN_MEMORY_DEBUG_LEAK_CHECK=0
        FOONATHAN_MEMORY_DEBUG_POINTER_CHECK=0
        FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK=0
        FOONATHAN_MEMORY_EXTERN_TEMPLATE=1
        FOONATHAN_MEMORY_TEMPORARY_STACK_MODE=2
)

file(GLOB_RECURSE ov_memory_sources CONFIGURE_DEPENDS ${memory_SOURCE_DIR}/src/*.cpp)
add_library(ov_foonathan_memory STATIC ${ov_memory_sources})
target_include_directories(
    ov_foonathan_memory
    PRIVATE ${memory_SOURCE_DIR}/src ${memory_SOURCE_DIR}/include/foonathan/memory
)
target_include_directories(ov_foonathan_memory SYSTEM PUBLIC ${memory_SOURCE_DIR}/include)
target_compile_definitions(
    ov_foonathan_memory
    PUBLIC
        FOONATHAN_MEMORY=1
        FOONATHAN_MEMORY_VERSION_MAJOR=0
        FOONATHAN_MEMORY_VERSION_MINOR=7
        FOONATHAN_MEMORY_VERSION_PATCH=4
)

# --- spdlog (external fmt comes from openvic-dataloader's vendored copy) ---
set(OPENVIC_SPDLOG_LEVEL
    ""
    CACHE STRING
    "Compile-time spdlog level: trace, debug, info, warn, error, critical, or off (empty = auto: debug for dev builds, warn for template_release, info otherwise)"
)
set_property(CACHE OPENVIC_SPDLOG_LEVEL PROPERTY STRINGS ";trace;debug;info;warn;error;critical;off")
if(NOT OPENVIC_SPDLOG_LEVEL STREQUAL "")
    string(TOUPPER "${OPENVIC_SPDLOG_LEVEL}" _ov_spdlog_upper)
    if(NOT _ov_spdlog_upper MATCHES "^(TRACE|DEBUG|INFO|WARN|ERROR|CRITICAL|OFF)$")
        message(FATAL_ERROR "OPENVIC_SPDLOG_LEVEL must be trace, debug, info, warn, error, critical, off, or empty")
    endif()
    set(OV_SPDLOG_LEVEL "SPDLOG_LEVEL_${_ov_spdlog_upper}")
elseif(GODOTCPP_DEV_BUILD)
    set(OV_SPDLOG_LEVEL "SPDLOG_LEVEL_DEBUG")
elseif(GODOTCPP_TARGET STREQUAL "template_release")
    set(OV_SPDLOG_LEVEL "SPDLOG_LEVEL_WARN")
else()
    set(OV_SPDLOG_LEVEL "SPDLOG_LEVEL_INFO")
endif()

set(SPDLOG_FMT_EXTERNAL ON)
set(SPDLOG_NO_EXCEPTIONS ON)
set(SPDLOG_INSTALL OFF)
set(SPDLOG_BUILD_EXAMPLE OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SPDLOG_FWRITE_UNLOCKED OFF)
endif()
openvic_declare_dep(spdlog
    REPO gabime/spdlog
    SHA f1d748e5e3edfa4b1778edea003bac94781bc7b7
    SHA256 0804ca200b3bb4ac5cfee7fbb9ca42069b4dbf21f5f1869465b4a5bf10b90ef4
)
FetchContent_MakeAvailable(spdlog) # spdlog::spdlog
# Compile-time log level filter; not an upstream option, so applied directly.
target_compile_definitions(spdlog PUBLIC SPDLOG_ACTIVE_LEVEL=${OV_SPDLOG_LEVEL})
