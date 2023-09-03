#!/usr/bin/env python

# This file is heavily based on https://github.com/godotengine/godot-cpp/blob/8155f35b29b4b08bc54b2eb0c57e1e9effe9f093/SConstruct
import os
import platform
import sys
import importlib

import SCons

# Local
from scripts.build.option_handler import OptionsClass
from scripts.build.glob_recursive import GlobRecursive
from scripts.build.cache import show_progress

# Try to detect the host platform automatically.
# This is used if no `platform` argument is passed
if sys.platform.startswith("linux"):
    default_platform = "linux"
elif sys.platform == "darwin":
    default_platform = "macos"
elif sys.platform == "win32" or sys.platform == "msys":
    default_platform = "windows"
elif ARGUMENTS.get("platform", ""):
    default_platform = ARGUMENTS.get("platform")
else:
    raise ValueError("Could not detect platform automatically, please specify with platform=<platform>")

is_standalone = SCons.Script.sconscript_reading == 1

BINDIR = "bin"
TOOLPATH = ["deps/openvic-dataloader/tools"]

try:
    Import("env")
    old_env = env
    env = old_env.Clone()
except:
    # Default tools with no platform defaults to gnu toolchain.
    # We apply platform specific toolchains via our custom tools.
    env = Environment(tools=["default"], PLATFORM="")
    old_env = env

env.PrependENVPath("PATH", os.getenv("PATH"))

# Default num_jobs to local cpu count if not user specified.
# SCons has a peculiarity where user-specified options won't be overridden
# by SetOption, so we can rely on this to know if we should use our default.
initial_num_jobs = env.GetOption("num_jobs")
altered_num_jobs = initial_num_jobs + 1
env.SetOption("num_jobs", altered_num_jobs)
if env.GetOption("num_jobs") == altered_num_jobs:
    cpu_count = os.cpu_count()
    if cpu_count is None:
        print("Couldn't auto-detect CPU count to configure build parallelism. Specify it with the -j argument.")
    else:
        safer_cpu_count = cpu_count if cpu_count <= 4 else cpu_count - 1
        print(
            "Auto-detected %d CPU cores available for build parallelism. Using %d cores by default. You can override it with the -j argument."
            % (cpu_count, safer_cpu_count)
        )
        env.SetOption("num_jobs", safer_cpu_count)

opts = OptionsClass(ARGUMENTS)

platforms = ("linux", "macos", "windows", "android", "ios", "javascript")
unsupported_known_platforms = ("android", "ios", "javascript")
opts.Add(
    EnumVariable(
        key="platform",
        help="Target platform",
        default=env.get("platform", default_platform),
        allowed_values=platforms,
        ignorecase=2,
    )
)

opts.Add(
    EnumVariable(
        key="target",
        help="Compilation target",
        default=env.get("target", "template_debug"),
        allowed_values=("editor", "template_release", "template_debug"),
    )
)

opts.Add(BoolVariable(key="build_ovsim_library", help="Build the openvic simulation library.", default=env.get("build_ovsim_library", not is_standalone)))
opts.Add(
    EnumVariable(
        key="precision",
        help="Set the floating-point precision level",
        default=env.get("precision", "single"),
        allowed_values=("single", "double"),
    )
)

# Add platform options
tools = {}
for pl in set(platforms) - set(unsupported_known_platforms):
    tool = Tool(pl, toolpath=TOOLPATH)
    if hasattr(tool, "options"):
        tool.options(opts)
    tools[pl] = tool

# CPU architecture options.
architecture_array = ["", "universal", "x86_32", "x86_64", "arm32", "arm64", "rv64", "ppc32", "ppc64", "wasm32"]
architecture_aliases = {
    "x64": "x86_64",
    "amd64": "x86_64",
    "armv7": "arm32",
    "armv8": "arm64",
    "arm64v8": "arm64",
    "aarch64": "arm64",
    "rv": "rv64",
    "riscv": "rv64",
    "riscv64": "rv64",
    "ppcle": "ppc32",
    "ppc": "ppc32",
    "ppc64le": "ppc64",
}
opts.Add(
    EnumVariable(
        key="arch",
        help="CPU architecture",
        default=env.get("arch", ""),
        allowed_values=architecture_array,
        map=architecture_aliases,
    )
)

opts.Add(BoolVariable("build_ovsim_headless", "Build the openvic simulation headless executable", is_standalone))

opts.Add(BoolVariable("compiledb", "Generate compilation DB (`compile_commands.json`) for external tools", False))
opts.Add(BoolVariable("verbose", "Enable verbose output for the compilation", False))
opts.Add(BoolVariable("intermediate_delete", "Enables automatically deleting unassociated intermediate binary files.", True))
opts.Add(BoolVariable("progress", "Show a progress indicator during compilation", True))

# Targets flags tool (optimizations, debug symbols)
target_tool = Tool("targets", toolpath=TOOLPATH)
target_tool.options(opts)

# Custom options and profile flags.
opts.Make(["custom.py"])
opts.Finalize(env)
Help(opts.GenerateHelpText(env))

if env["platform"] in unsupported_known_platforms:
    print("Unsupported platform: " + env["platform"]+". Only supports " + ", ".join(set(platforms) - set(unsupported_known_platforms)))
    Exit()

# Process CPU architecture argument.
if env["arch"] == "":
    # No architecture specified. Default to arm64 if building for Android,
    # universal if building for macOS or iOS, wasm32 if building for web,
    # otherwise default to the host architecture.
    if env["platform"] in ["macos", "ios"]:
        env["arch"] = "universal"
    elif env["platform"] == "android":
        env["arch"] = "arm64"
    elif env["platform"] == "javascript":
        env["arch"] = "wasm32"
    else:
        host_machine = platform.machine().lower()
        if host_machine in architecture_array:
            env["arch"] = host_machine
        elif host_machine in architecture_aliases.keys():
            env["arch"] = architecture_aliases[host_machine]
        elif "86" in host_machine:
            # Catches x86, i386, i486, i586, i686, etc.
            env["arch"] = "x86_32"
        else:
            print("Unsupported CPU architecture: " + host_machine)
            Exit()

tool = Tool(env["platform"], toolpath=TOOLPATH)

if tool is None or not tool.exists(env):
    raise ValueError("Required toolchain not found for platform " + env["platform"])

tool.generate(env)
target_tool.generate(env)

print("Building for architecture " + env["arch"] + " on platform " + env["platform"])

# Require C++20
if env.get("is_msvc", False):
    env.Append(CXXFLAGS=["/std:c++20"])
else:
    env.Append(CXXFLAGS=["-std=c++20"])

if env["precision"] == "double":
    env.Append(CPPDEFINES=["REAL_T_IS_DOUBLE"])

scons_cache_path = os.environ.get("SCONS_CACHE")
if scons_cache_path != None:
    CacheDir(scons_cache_path)
    print("Scons cache enabled... (path: '" + scons_cache_path + "')")

if env["compiledb"]:
    # Generating the compilation DB (`compile_commands.json`) requires SCons 4.0.0 or later.
    from SCons import __version__ as scons_raw_version

    scons_ver = env._get_major_minor_revision(scons_raw_version)

    if scons_ver < (4, 0, 0):
        print("The `compiledb=yes` option requires SCons 4.0 or later, but your version is %s." % scons_raw_version)
        Exit(255)

    env.Tool("compilation_db")
    env.Alias("compiledb", env.CompilationDatabase())

ovdl_env = SConscript("deps/openvic-dataloader/SConstruct")

env.Append(LIBPATH=ovdl_env.openvic_dataloader["LIBPATH"])
env.Append(LIBS=ovdl_env.openvic_dataloader["LIBS"])
env.Append(CPPPATH=ovdl_env.openvic_dataloader["INCPATH"])

env.openvic_simulation = {}

# For future reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Tweak this if you want to use different folders, or more folders, to store your source code in.
source_path = "src/openvic"
include_path = "src"
env.Append(CPPPATH=[[env.Dir(p) for p in [source_path, include_path]]])
sources = GlobRecursive("*.cpp", [source_path])
env.simulation_sources = sources

from glob import glob
from pathlib import Path
def remove_extension(file : str):
    if file.find(".") == -1: return file
    return file[:file.rindex(".")]

found_one = False
for path in [source_path]:
    for obj_file in [file[:-len(".obj")] for file in glob(path + "*.obj", recursive=True)]:
        found = False
        for source_file in sources:
            if remove_extension(str(source_file)) == obj_file:
                found = True
                break
        if not found:
            if not found_one:
                found_one = True
                print("Unassociated intermediate files found...")
            print("Removing "+obj_file+".obj")
            os.remove(obj_file+".obj")

suffix = ".{}.{}".format(env["platform"], env["target"])
if env.dev_build:
    suffix += ".dev"
if env["precision"] == "double":
    suffix += ".double"
suffix += "." + env["arch"]

# Expose it when included from another project
env["suffix"] = suffix

library = None
env["OBJSUFFIX"] = suffix + env["OBJSUFFIX"]
library_name = "libopenvic-simulation{}{}".format(suffix, env["LIBSUFFIX"])

if env["build_ovsim_library"]:
    library = env.StaticLibrary(target=env.File(os.path.join(BINDIR, library_name)), source=sources)
    Default(library)

    env.Append(LIBPATH=[env.Dir(BINDIR)])
    env.Append(LIBS=[library_name])

    env.openvic_simulation["LIBPATH"] = env["LIBPATH"]
    env.openvic_simulation["LIBS"] = env["LIBS"]
    env.openvic_simulation["INCPATH"] = [env.Dir(include_path)] + ovdl_env.openvic_dataloader["INCPATH"]

headless_program = None
env["PROGSUFFIX"] = suffix + env["PROGSUFFIX"]

if env["build_ovsim_headless"]:
    headless_name = "openvic-simulation"
    headless_env = env.Clone()
    headless_path = ["src/headless"]
    headless_env.Append(CPPDEFINES=["OPENVIC_SIM_HEADLESS"])
    headless_env.Append(CPPPATH=[headless_env.Dir(headless_path)])
    headless_env.headless_sources = GlobRecursive("*.cpp", headless_path)
    if not env["build_ovsim_library"]:
        headless_env.headless_sources += sources
    headless_env.headless_sources,
    headless_program = headless_env.Program(
        target=os.path.join(BINDIR, headless_name),
        source=headless_env.headless_sources,
        PROGSUFFIX=".headless" + env["PROGSUFFIX"]
    )
    Default(headless_program)


if "env" in locals():
    # FIXME: This method mixes both cosmetic progress stuff and cache handling...
    show_progress(env)

Return("env")
