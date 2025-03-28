// This file was generated using the `misc/scripts/exp_lut_generator.py` script with `-e 2001`.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// clang-format off
namespace OpenVic::_detail::LUT {
	static constexpr uint64_t _2_16_EXP_2001_DIVISOR = 65536;
	static constexpr size_t _2_16_EXP_2001_SIZE = 19;

	static constexpr std::array<int64_t, _2_16_EXP_2001_SIZE> _2_16_EXP_2001 {
		65544,
		65551,
		65566,
		65597,
		65658,
		65780,
		66024,
		66516,
		67511,
		69546,
		73801,
		83108,
		105392,
		169487,
		438320,
		2931592,
		131137536,
		262406209536,
		1050674725388353536
	};
}
// clang-format on
