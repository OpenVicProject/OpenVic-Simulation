#!/usr/bin/env python

import os

BINDIR = "bin"

try:
    Import("build_dir")
    Export("build_dir")
except Exception:
    pass

env = SConscript("scripts/SConstruct", exports={"gen_dir": "src/openvic-simulation/gen"})

env.PrependENVPath("PATH", os.getenv("PATH"))

SConscript("deps/SCsub", "env")

env["name_prefix"] = "sim"
gen_commit_info = env.Git(
    "commit_info.gen.hpp",
    env.Value(env.GetGitInfo()),
)
gen_license_info = env.License(
    "license_info.gen.hpp",
    ["COPYRIGHT", "LICENSE.md"],
)
gen_author_info = env.Author(
    "author_info.gen.hpp",
    "AUTHORS.md",
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

pch_file = "pch"
pch_base = f"src/openvic-simulation/{pch_file}"
pch_source = f"{pch_base}.cpp"
pch_header = f"openvic-simulation/{pch_file}.hpp"

env.AddLibraryIncludes("src", add_build_dir=True)
env.AddLibraryIncludes("src/openvic-simulation", False)
env.AddLibrarySources("src/openvic-simulation", exclude=pch_source)

# Precompiled header
if env.get("is_msvc", False):
    env["PCHSTOP"] = pch_header
    env["PCH"] = env.PCH(target=os.path.join(env["build_dir"], f"{pch_base}.pch"), source=pch_source)[0]
else:
    env["GCH"] = env.GCH(target=os.path.join(env["build_dir"], f"{pch_base}.gch"), source=pch_source)[0]

if env["build_ovsim_library"]:
    library = env.BuildBaseLibrary(os.path.join(BINDIR, "libopenvic-simulation"))

if env["build_ovsim_headless"]:
    headless_program = env.BuildHeadlessProgram(
        target=os.path.join(BINDIR, "openvic-simulation"),
        src_dir="src/headless",
        defines_prefix="openvic_sim",
        include_lib_src=not env["build_ovsim_library"],
    )

if env["build_ovsim_tests"]:
    tests_env = SConscript("tests/SCsub", "env")

    if env["run_ovsim_tests"]:
        tests_env.RunUnitTest()

if env["build_ovsim_benchmarks"]:
    benchmarks_env = SConscript("tests/benchmarks/SCsub", {"env": tests_env if env["build_ovsim_tests"] else env})

    if env["run_ovsim_benchmarks"]:
        benchmarks_env.RunBenchmarks()

Return("env")
