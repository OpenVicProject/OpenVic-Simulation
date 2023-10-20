# OpenVic-Simulation
Repo of the OpenVic-Simulation Library for [OpenVic](https://github.com/OpenVicProject/OpenVic)

## Quickstart Guide
For detailed instructions, view the OpenVic Contributor Quickstart Guide [here](https://github.com/OpenVicProject/OpenVic/blob/master/docs/contribution-quickstart-guide.md)

## Required
* [scons](https://scons.org/)

## Build Instructions
1. Install [scons](https://scons.org/) for your system.
2. Run the command `git submodule update --init --recursive` to retrieve all related submodules.
3. Run `scons` in the project root, you should see a openvic-simulation.headless file in `bin`.

## Link Instructions
1. Call `ovsim_env = SConscript("openvic-simulation/SConstruct")`
2. Use the values stored in the `ovsim_env.openvic_simulation` to link and compile against:

| Variable Name | Description                               | Correlated ENV variable   |
| ---           | ---                                       | ---                       |
| `LIBPATH`     | Library path list                         | `env["LIBPATH"]`          |
| `LIBS`        | Library files names in the library paths  | `env["LIBS"]`             |
| `INCPATH`     | Library include files                     | `env["CPPPATH"]`          |
