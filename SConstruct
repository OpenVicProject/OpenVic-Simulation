#!/usr/bin/env python
import os
import sys
from glob import glob
from pathlib import Path

def GlobRecursive(pattern, nodes=['.']):
    import SCons
    results = []
    for node in nodes:
        nnodes = []
        for f in Glob(str(node) + '/*', source=True):
            if type(f) is SCons.Node.FS.Dir:
                nnodes.append(f)
        results += GlobRecursive(pattern, nnodes)
        results += Glob(str(node) + '/' + pattern, source=True)
    return results

# For future reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags
env = Environment(
    CPPDEFINES=["OPENVIC_HEADLESS_SIM"]
)

# Require C++20
if sys.platform == "win32" or sys.platform == "msys":
    env.Append(CXXFLAGS=["/std:c++20", "/EHsc"])
else:
    env.Append(CXXFLAGS=["-std=c++20"])

# Tweak this if you want to use different folders, or more folders, to store your source code in.
paths = ["src"]
env.Append(CPPPATH=paths)
sources = GlobRecursive("*.cpp", paths)

program = env.Program("headless-sim", sources)


Default(program)

