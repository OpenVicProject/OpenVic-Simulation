#!/usr/bin/env python
import decimal
import os
from argparse import ArgumentParser
from math import e
from typing import List

BIT_COUNT = 64
MAX_VALUE = 2**BIT_COUNT

DEFAULT_DIVISOR_BASE = 2
DEFAULT_DIVISOR_POWER = 16
DEFAULT_EXP_BASE = e


def generate_exp_lut(divisor_base: int, divisor_power: int, exp_base: decimal.Decimal):
    divisor: int = divisor_base**divisor_power

    exp_lut: List[int] = []

    for index in range(BIT_COUNT):
        exponent: decimal.Decimal = (2**index) / divisor
        value: int = int(decimal.Decimal(exp_base**exponent) * decimal.Decimal(divisor) + decimal.Decimal(0.5))
        if value > MAX_VALUE:
            break
        exp_lut.append(value)

    lut_identifier: str = (
        f"{divisor_base}_{divisor_power}_EXP_{'e' if exp_base == e else ('%g' % exp_base).replace('.', 'p')}"
    )
    lut_size: int = len(exp_lut)

    generated_options = ""
    if (
        DEFAULT_DIVISOR_BASE is not divisor_base
        or DEFAULT_DIVISOR_POWER is not divisor_power
        or DEFAULT_EXP_BASE is not exp_base
    ):
        generated_options += " with `"

        if DEFAULT_DIVISOR_BASE is not divisor_base:
            generated_options += f"-b {divisor_base}"
            if DEFAULT_DIVISOR_POWER is not divisor_power or DEFAULT_EXP_BASE is not exp_base:
                generated_options += " "

        if DEFAULT_DIVISOR_POWER is not divisor_power:
            generated_options += f"-p {divisor_power}"
            if DEFAULT_EXP_BASE is not exp_base:
                generated_options += " "

        if DEFAULT_EXP_BASE is not exp_base:
            generated_options += f"-e {exp_base:g}"

        generated_options += "`"

    source: str = f"""// This file was generated using the `misc/scripts/exp_lut_generator.py` script{generated_options}.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// clang-format off
namespace OpenVic::_detail::LUT {{
\tstatic constexpr uint64_t _{lut_identifier}_DIVISOR = {divisor};
\tstatic constexpr size_t _{lut_identifier}_SIZE = {lut_size};

\tstatic constexpr std::array<int64_t, _{lut_identifier}_SIZE> _{lut_identifier} {{
"""

    for value in exp_lut[:-1]:
        source += f"\t\t{value},\n"

    source += f"\t\t{exp_lut[-1]}\n"
    source += "\t};\n"
    source += "}\n// clang-format on\n"

    fixed_point_lut_path: str = os.path.join(
        os.path.dirname(__file__), f"../../src/openvic-simulation/core/object/FixedPoint/LUT_{lut_identifier}.hpp"
    )
    with open(fixed_point_lut_path, "w", newline="\n") as file:
        file.write(source)

    print(f"`FixedPoint{lut_identifier}.hpp` generated successfully.")


if __name__ == "__main__":
    parser = ArgumentParser(
        prog="Fixed Point Exp LUT Generator", description="Fixed-Point Exponential Look-Up Table generator"
    )
    parser.add_argument(
        "-b",
        "--base",
        type=int,
        default=DEFAULT_DIVISOR_BASE,
        choices=range(2, 65),
        help="The base of the fixed point divisor",
    )
    parser.add_argument(
        "-p",
        "--power",
        type=int,
        default=DEFAULT_DIVISOR_POWER,
        choices=range(1, 65),
        help="The power of the fixed point divisor",
    )
    parser.add_argument(
        "-e",
        "--exp",
        type=float,
        default=DEFAULT_EXP_BASE,
        help="The base of the exponential function the look-up table represents",
    )
    args = parser.parse_args()

    generate_exp_lut(args.base, args.power, args.exp)
    exit(0)
