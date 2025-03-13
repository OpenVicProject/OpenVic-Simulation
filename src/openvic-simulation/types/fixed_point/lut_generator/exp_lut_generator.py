#!/usr/bin/env python
from argparse import ArgumentParser
from math import e
from sys import exit

BIT_COUNT = 64
MAX_VALUE = 2**BIT_COUNT


def generate_exp_lut(divisor_base: int, divisor_power: int, exp_base: float):
    divisor = divisor_base**divisor_power

    exp_lut = []

    for index in range(BIT_COUNT):
        exponent = (2**index) / divisor
        value = int((exp_base**exponent) * divisor + 0.5)
        if value > MAX_VALUE:
            break
        exp_lut.append(value)

    lut_identifier = (
        f"LUT_{divisor_base}_{divisor_power}_EXP_{'e' if exp_base == e else ('%g' % exp_base).replace('.', 'p')}"
    )
    lut_size = len(exp_lut)

    output = [
        "",
        f"static constexpr int64_t {lut_identifier}_DIVISOR = {divisor};",
        f"static constexpr size_t {lut_identifier}_SIZE = {lut_size};",
        "",
        f"static constexpr std::array<int64_t, {lut_identifier}_SIZE> {lut_identifier} {{",
    ]

    for value in exp_lut:
        output.append(f"\t{value},")

    output[-1] = output[-1][:-1]  # Remove the last comma
    output += ["};", ""]

    cpp_code = "\n".join(output)

    with open(f"FixedPoint{lut_identifier}.hpp", "w", newline="\n") as file:
        file.write(cpp_code)


DIVISOR_BASE = 2
DIVISOR_POWER = 16
EXP_BASE = e

if __name__ == "__main__":
    parser = ArgumentParser(
        prog="Fixed Point Exp LUT Generator", description="Fixed-Point Exponential Look-Up Table generator"
    )
    parser.add_argument(
        "-b", "--base", type=int, default=DIVISOR_BASE, choices=range(2, 65), help="The base of the fixed point divisor"
    )
    parser.add_argument(
        "-p",
        "--power",
        type=int,
        default=DIVISOR_POWER,
        choices=range(1, 65),
        help="The power of the fixed point divisor",
    )
    parser.add_argument(
        "-e",
        "--exp",
        type=float,
        default=EXP_BASE,
        help="The base of the exponential function the look-up table represents",
    )
    args = parser.parse_args()

    generate_exp_lut(args.base, args.power, args.exp)
    exit(0)
