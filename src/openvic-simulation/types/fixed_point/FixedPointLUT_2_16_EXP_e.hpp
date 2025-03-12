// This file was generated using the `misc/scripts/exp_lut_generator.py` script.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

static constexpr uint64_t LUT_2_16_EXP_e_DIVISOR = 65536;
static constexpr size_t LUT_2_16_EXP_e_SIZE = 22;

static constexpr std::array<int64_t, LUT_2_16_EXP_e_SIZE> LUT_2_16_EXP_e {
	65537,
	65538,
	65540,
	65544,
	65552,
	65568,
	65600,
	65664,
	65793,
	66050,
	66568,
	67616,
	69763,
	74262,
	84150,
	108051,
	178145,
	484249,
	3578144,
	195360063,
	582360139072,
	5174916558532153344
};
