[![🖥️ Builds](https://github.com/OpenVicProject/OpenVic-Simulation/actions/workflows/builds.yml/badge.svg)](https://github.com/OpenVicProject/OpenVic-Simulation/actions/workflows/builds.yml)

# OpenVic-Simulation
Repo of the OpenVic-Simulation Library for [OpenVic](https://github.com/OpenVicProject/OpenVic)

## Quickstart Guide
For detailed instructions, view the OpenVic Contributor Quickstart Guide [here](https://github.com/OpenVicProject/OpenVic/blob/master/docs/contribution-quickstart-guide.md)

## Required
* [CMake](https://cmake.org/) 3.28+
* [Ninja](https://ninja-build.org/)
* Python 3 (used by the build for code generation)

## Build Instructions
1. Run the command `git submodule update --init` to retrieve the first-party submodules (openvic-dataloader, lexy-vdf). Third-party dependencies are fetched automatically by CMake.
2. Pick a configure preset from `CMakePresets.json` (`windows-x64-md`, `windows-x64-mt`, `linux-x64`, `macos-universal`) and run `cmake --preset <preset>` in the project root.
3. Run `cmake --build --preset <preset>-debug` (or `<preset>-release`). The static library, headless executable, and unit tests land in `out/build/<preset>/bin/<Config>/`.
4. Run the tests with `ctest --preset <preset>-debug`.

The headless executable and tests are built by default in standalone builds; disable with `-DOPENVIC_SIM_BUILD_HEADLESS=OFF` / `-DOPENVIC_SIM_BUILD_TESTS=OFF`. Benchmarks are opt-in: `-DOPENVIC_SIM_BUILD_BENCHMARKS=ON`.

## Link Instructions
Use CMake: `add_subdirectory(openvic-simulation)` and link against `openvic::simulation`.
