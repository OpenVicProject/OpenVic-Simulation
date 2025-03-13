#!/usr/bin/env python
from argparse import ArgumentParser
from math import pi, sin
from sys import exit


def generate_sin_lut(precision: int, count_log2: int):
    one = 1 << precision
    count = 1 << count_log2

    SinLut = []

    for i in range(count):
        angle = 2 * pi * i / count

        sin_value = sin(angle)
        moved_sin = sin_value * one
        rounded_sin = int(moved_sin + 0.5) if moved_sin > 0 else int(moved_sin - 0.5)
        SinLut.append(rounded_sin)

    SinLut.append(SinLut[0])

    output = [
        "",
        f"static constexpr int32_t SIN_LUT_PRECISION = {precision};",
        f"static constexpr int32_t SIN_LUT_COUNT_LOG2 = {count_log2};",
        "static constexpr int32_t SIN_LUT_SHIFT = SIN_LUT_PRECISION - SIN_LUT_COUNT_LOG2;",
        "",
        "static constexpr int64_t SIN_LUT[(1 << SIN_LUT_COUNT_LOG2) + 1] = {",
    ]

    VALS_PER_LINE = 16

    lines = [SinLut[i : i + VALS_PER_LINE] for i in range(0, len(SinLut), VALS_PER_LINE)]

    for line in lines:
        output.append("\t" + ", ".join(str(value) for value in line) + ",")

    output[-1] = output[-1][:-1]  # Remove the last comma
    output += ["};", ""]

    cpp_code = "\n".join(output)

    with open("FixedPointLUT_sin.hpp", "w", newline="\n") as file:
        file.write(cpp_code)


PRECISION = 16
COUNT = 9

if __name__ == "__main__":
    parser = ArgumentParser(prog="Fixed Point Sin LUT Generator", description="Fixed-Point Sin Look-Up Table generator")
    parser.add_argument(
        "-p",
        "--precision",
        type=int,
        default=PRECISION,
        choices=range(1, 65),
        help="The number of bits after the point (fractional bits)",
    )
    parser.add_argument(
        "-c",
        "--count",
        type=int,
        default=COUNT,
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
