#!/usr/bin/env python
import math

PRECISION = 16
ONE = 1 << PRECISION
SIN_VALUE_COUNT = 512

SinLut = []

for i in range(SIN_VALUE_COUNT):
	angle = 2 * math.pi * i / SIN_VALUE_COUNT

	sin_value = math.sin(angle)
	moved_sin = sin_value * ONE
	rounded_sin = int(moved_sin + 0.5) if moved_sin > 0 else int(moved_sin - 0.5)
	SinLut.append(rounded_sin)

SinLut.append(SinLut[0])

output = [
"""#pragma once

#include <cstdint>

static constexpr int32_t SIN_LUT[] = {"""
]

lines = [SinLut[i:i+10] for i in range(0, len(SinLut), 10)]

for line in lines:
	output.append("\t" + ", ".join(str(value) for value in line) + ",")

output[-1] = output[-1][:-1]  # Remove the last comma
output.append("};\n")

cpp_code = "\n".join(output)

with open(f"FPLUT_sin_{SIN_VALUE_COUNT}.hpp", "w", newline="\n") as file:
	file.write(cpp_code)