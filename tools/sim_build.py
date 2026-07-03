def exists(env):
    return True


def options(opts):
    from SCons.Variables import BoolVariable

    opts.Add(BoolVariable("build_ovsim_tests", "Build the openvic simulation unit tests", True))
    opts.Add(BoolVariable("run_ovsim_tests", "Run the openvic simulation unit tests", False))
    opts.Add(BoolVariable("build_ovsim_benchmarks", "Build the openvic simulation benchmarks", False))
    opts.Add(BoolVariable("run_ovsim_benchmarks", "Run the openvic simulation benchmarks", False))
    opts.Add(BoolVariable("build_ovsim_library", "Build the openvic simulation library.", False))
    opts.Add(BoolVariable("build_ovsim_headless", "Build the openvic simulation headless executable", True))


def generate(env):
    if not env.is_standalone:
        env["run_ovsim_tests"] = False
        env["build_ovsim_tests"] = False
        env["run_ovsim_benchmarks"] = False
        env["build_ovsim_benchmarks"] = False
        env["build_ovsim_library"] = True
        env["build_ovsim_headless"] = False

    if env["run_ovsim_tests"]:
        env["build_ovsim_tests"] = True

    if env["run_ovsim_benchmarks"]:
        env["build_ovsim_benchmarks"] = True

    if env["build_ovsim_tests"] or env["build_ovsim_benchmarks"]:
        env["build_ovsim_library"] = True
