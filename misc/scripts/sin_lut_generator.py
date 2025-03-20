#!/usr/bin/env python
import decimal
import os
from argparse import ArgumentParser

DEFAULT_PRECISION = 16
DEFAULT_COUNT = 9


def decimal_pi() -> decimal.Decimal:
    """Compute Pi to the current precision.

    >>> print(pdecimal_pii())
    3.141592653589793238462643383

    """
    decimal.getcontext().prec += 2  # extra digits for intermediate steps
    three: decimal.Decimal = decimal.Decimal(3)  # substitute "three=3.0" for regular floats
    lasts: decimal.Decimal = decimal.Decimal(0)
    t: decimal.Decimal = three
    n, na, d, da = 1, 0, 0, 24
    s: decimal.Decimal = three
    while s != lasts:
        lasts = s
        n, na = n + na, na + 8
        d, da = d + da, da + 32
        t = (t * n) / d
        s += decimal.Decimal(t)
    decimal.getcontext().prec -= 2
    return +s  # unary plus applies the new precision


def decimal_sin(x: decimal.Decimal) -> decimal.Decimal:
    """Return the sine of x as measured in radians.

    The Taylor series approximation works best for a small value of x.
    For larger values, first compute x = x % (2 * pi).

    >>> print(decimal_sin(Decimal('0.5')))
    0.4794255386042030002732879352
    >>> print(decimal_sin(0.5))
    0.479425538604
    >>> print(decimal_sin(0.5+0j))
    (0.479425538604+0j)

    """
    decimal.getcontext().prec += 2
    i, fact, num, sign = 1, 1, x, 1
    s: decimal.Decimal = x
    lasts: decimal.Decimal = decimal.Decimal(0)
    while s != lasts:
        lasts = s
        i += 2
        fact *= i * (i - 1)
        num *= x * x
        sign *= -1
        s += num / fact * sign
    decimal.getcontext().prec -= 2
    return +s


def generate_sin_lut(precision: int, count_log2: int):
    one = 1 << precision
    count = 1 << count_log2

    SinLut = []

    for i in range(count):
        angle: decimal.Decimal = 2 * decimal_pi() * i / count

        sin_value: decimal.Decimal = decimal_sin(angle)  # sin(angle)
        moved_sin: decimal.Decimal = sin_value * one
        rounded_sin: int = (
            int(moved_sin + decimal.Decimal(0.5)) if moved_sin > 0 else int(moved_sin - decimal.Decimal(0.5))
        )
        SinLut.append(rounded_sin)

    SinLut.append(SinLut[0])

    generated_options = ""
    if DEFAULT_PRECISION is not precision or DEFAULT_COUNT is not count_log2:
        generated_options += " with `"

        if DEFAULT_PRECISION is not precision:
            generated_options += f"-p {precision}"
            if DEFAULT_COUNT is not count_log2:
                generated_options += " "

        if DEFAULT_COUNT is not count_log2:
            generated_options += f"-c {count_log2}"

        generated_options += "`"

    source = f"""// This file was generated using the `misc/scripts/sin_lut_generator.py` script{generated_options}.

#pragma once

#include <array>
#include <cstdint>

// clang-format off
namespace OpenVic::_detail::LUT {{
\tstatic constexpr uint32_t SIN_PRECISION = {precision};
\tstatic constexpr uint32_t SIN_COUNT_LOG2 = {count_log2};
\tstatic constexpr int32_t SIN_SHIFT = SIN_PRECISION - SIN_COUNT_LOG2;

\tstatic constexpr std::array<int64_t, (1 << SIN_COUNT_LOG2) + 1> SIN = {{
"""

    VALS_PER_LINE = 16

    lines = [SinLut[i : i + VALS_PER_LINE] for i in range(0, len(SinLut), VALS_PER_LINE)]

    for line in lines[:-1]:
        source += f"\t\t{', '.join(str(value) for value in line)},\n"

    source += f"\t\t{', '.join(str(value) for value in lines[-1])}\n"
    source += "\t};\n"
    source += "}\n// clang-format on\n"

    fixed_point_sin_path: str = os.path.join(
        os.path.dirname(__file__), "../../src/openvic-simulation/types/fixed_point/FixedPointLUT_sin.hpp"
    )
    with open(fixed_point_sin_path, "w", newline="\n") as file:
        file.write(source)

    print("`FixedPointLUT_sin.hpp` generated successfully.")


if __name__ == "__main__":
    parser = ArgumentParser(prog="Fixed Point Sin LUT Generator", description="Fixed-Point Sin Look-Up Table generator")
    parser.add_argument(
        "-p",
        "--precision",
        type=int,
        default=DEFAULT_PRECISION,
        choices=range(1, 65),
        help="The number of bits after the point (fractional bits)",
    )
    parser.add_argument(
        "-c",
        "--count",
        type=int,
        default=DEFAULT_COUNT,
        choices=range(1, 65),
        help="The base 2 log of the number of values in the look-up table (must be <= precision)",
    )
    args = parser.parse_args()

    if args.precision < args.count:
        print("ERROR: invalid count ", args.count, " - can't be greater than precision (", args.precision, ")")
        exit(-1)
    else:
        generate_sin_lut(args.precision, args.count)
        exit(0)
