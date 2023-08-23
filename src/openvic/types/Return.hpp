#pragma once

namespace OpenVic {
	using return_t = bool;

	// This mirrors godot::Error, where `OK = 0` and `FAILED = 1`.
	constexpr return_t SUCCESS = false, FAILURE = true;
}
