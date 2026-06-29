#!/usr/bin/env python

import os

BINDIR = "bin"

env = SConscript("scripts/SConstruct")

env.PrependENVPath("PATH", os.getenv("PATH"))

opts = env.SetupOptions()

opts.Add(BoolVariable("build_ovsim_tests", "Build the openvic simulation unit tests", env.is_standalone))
opts.Add(BoolVariable("run_ovsim_tests", "Run the openvic simulation unit tests", False))
opts.Add(BoolVariable("build_ovsim_benchmarks", "Build the openvic simulation benchmarks", False))
opts.Add(BoolVariable("run_ovsim_benchmarks", "Run the openvic simulation benchmarks", False))
opts.Add(
    BoolVariable(
        key="build_ovsim_library",
        help="Build the openvic simulation library.",
        default=env.get("build_ovsim_library", not env.is_standalone),
    )
)
opts.Add(BoolVariable("build_ovsim_headless", "Build the openvic simulation headless executable", env.is_standalone))

env.FinalizeOptions()

suffix = ".{}.{}".format(env["platform"], env["target"])
if env.dev_build:
    suffix += ".dev"
if env["precision"] == "double":
    suffix += ".double"
suffix += "." + env["arch"]
if env["platform"] == "windows":
    if env.get("debug_crt", False):
        suffix += ".mdd"
    elif env.get("use_static_cpp", False):
        suffix += ".mt"
    else:
        suffix += ".md"
if env.get("use_asan", False):
    suffix += ".san"
env["suffix"] = suffix

build_dir = env.Dir("build/" + suffix.lstrip(".")).abspath.replace("\\", "/")
env["build_dir"] = build_dir

env.exposed_includes = []

SConscript("deps/SCsub", "env")

env.openvic_simulation = {}

# Tweak this if you want to use different folders, or more folders, to store your source code in.
source_path = "src/openvic-simulation"
include_path = "src"
# Out-of-source build: variant tree holds object files and generated headers,
# not copies of the source. Compile diagnostics therefore reference original
# source paths.
sim_variant = build_dir + "/" + source_path  # forward slashes so VariantDir matches
sim_variant_parent = build_dir + "/" + include_path  # variant of "src/"
env.VariantDir(sim_variant, source_path, duplicate=False)

env.Append(CPPPATH=[[env.Dir(p) for p in [sim_variant, sim_variant_parent, source_path, include_path]]])

gen_commit_info = env.CommandNoCache(
    sim_variant + "/gen/commit_info.gen.hpp",
    env.Value(env.get_git_info()),
    env.Run(env.git_builder),
    name_prefix="sim",
)
gen_license_info = env.CommandNoCache(
    sim_variant + "/gen/license_info.gen.hpp",
    ["COPYRIGHT", "LICENSE.md"],
    env.Run(env.license_builder),
    name_prefix="sim",
)
gen_author_info = env.CommandNoCache(
    sim_variant + "/gen/author_info.gen.hpp",
    "AUTHORS.md",
    env.Run(env.author_builder),
    name_prefix="sim",
    sections={
        "Senior Developers": "AUTHORS_SENIOR_DEVELOPERS",
        "Developers": "AUTHORS_DEVELOPERS",
        "Contributors": "AUTHORS_CONTRIBUTORS",
        "Consultants": "AUTHORS_CONSULTANTS",
    },
)
gen_files = gen_commit_info + gen_license_info + gen_author_info
Default(gen_commit_info, gen_license_info, gen_author_info)

# For future reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Exclude pch.cpp from the regular source list so it isn't compiled twice
# (env.PCH below builds it once with /Yc).
pch_cpp_source = source_path + "/pch.cpp"
pch_cpp_variant = sim_variant + "/pch.cpp"
sources = env.GlobRecursiveVariant("*.cpp", source_path, sim_variant, pch_cpp_source)
env.simulation_sources = sources

library = None
env["OBJSUFFIX"] = suffix + env["OBJSUFFIX"]
library_name = "libopenvic-simulation{}{}".format(suffix, env["LIBSUFFIX"])

# Precompiled header
if env.get("is_msvc", False):
    pch_header_rel = "openvic-simulation/pch.hpp"
    env["PCHSTOP"] = pch_header_rel
    pch_pch, pch_obj = env.PCH(pch_cpp_variant)
    env["PCH"] = pch_pch
    env.Append(CCFLAGS=["/FI" + pch_header_rel])
    sources.append(pch_obj)

default_args = []

if env["run_ovsim_tests"]:
    env["build_ovsim_tests"] = True

if env["run_ovsim_benchmarks"]:
    env["build_ovsim_benchmarks"] = True

if env["build_ovsim_tests"] or env["build_ovsim_benchmarks"]:
    env["build_ovsim_library"] = True

if env["build_ovsim_library"]:
    library = env.StaticLibrary(target=env.File(os.path.join(BINDIR, library_name)), source=sources)
    default_args += [library]

    # Ensure `scons -c` wipes the per-config build dir, including any stragglers
    # (gen headers, .pch caches) not directly attached to the library target.
    env.Clean(library, env.Dir(build_dir))

    env.Append(LIBPATH=[env.Dir(BINDIR)])
    env.Prepend(LIBS=[library_name])

    env.openvic_simulation["LIBPATH"] = env["LIBPATH"]
    env.openvic_simulation["LIBS"] = env["LIBS"]
    # Variant parent for generated headers (gen/*.gen.hpp); source parent for
    # authored headers (MSVC's preprocessor needs both physically in -I).
    env.openvic_simulation["INCPATH"] = [env.Dir(sim_variant_parent), env.Dir(include_path)] + env.exposed_includes
    env.openvic_simulation["GEN_FILES"] = gen_files

headless_program = None
env["PROGSUFFIX"] = suffix + env["PROGSUFFIX"]

if env["build_ovsim_headless"]:
    headless_name = "openvic-simulation"
    headless_env = env.Clone()
    headless_src = "src/headless"
    headless_variant = build_dir + "/" + headless_src
    headless_env.VariantDir(headless_variant, headless_src, duplicate=False)
    headless_env.Append(CPPDEFINES=["OPENVIC_SIM_HEADLESS"])
    headless_env.Append(CPPPATH=[headless_env.Dir(headless_variant), headless_env.Dir(headless_src)])
    headless_env.headless_sources = env.GlobRecursiveVariant("*.cpp", headless_src, headless_variant)
    if not env["build_ovsim_library"]:
        headless_env.headless_sources += sources
    headless_program = headless_env.Program(
        target=os.path.join(BINDIR, headless_name),
        source=headless_env.headless_sources,
        PROGSUFFIX=".headless" + env["PROGSUFFIX"],
    )
    default_args += [headless_program]

    # Also wipe the per-config build dir when the headless target is cleaned,
    # in case the library target isn't built in this configuration.
    headless_env.Clean(headless_program, headless_env.Dir(build_dir))

if env["build_ovsim_tests"]:
    tests_env = SConscript("tests/SCsub", "env")

    if env["run_ovsim_tests"]:
        tests_env.RunUnitTest()

if env["build_ovsim_benchmarks"]:
    benchmarks_env = SConscript("tests/benchmarks/SCsub", {"env": tests_env if env["build_ovsim_tests"] else env})

    if env["run_ovsim_benchmarks"]:
        benchmarks_env.RunBenchmarks()

# Add compiledb if the option is set
if env.get("compiledb", False):
    default_args += ["compiledb"]

Default(*default_args)

if "env" in locals():
    # FIXME: This method mixes both cosmetic progress stuff and cache handling...
    env.show_progress(env)

Return("env")
