#pragma once

#include <cstdint>

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	struct HostSession {
		signal_property<HostSession, HostSession const&> session_changed;

		HostSession(memory::string game_name = "HostSession");

		HostSession(HostSession const&) = delete;
		HostSession& operator=(HostSession const&) = delete;
		HostSession(HostSession&&) = default;
		HostSession& operator=(HostSession&& other);

		template<std::endian Endian>
		size_t encode(std::span<uint8_t> span) const {
			return utility::encode<decltype(game_name), Endian>(game_name, span);
		}

		template<std::endian Endian>
		static HostSession decode(std::span<uint8_t> span, size_t& r_decode_count) {
			return { utility::decode<decltype(game_name), Endian>(span, r_decode_count) };
		}

		void set_game_name(memory::string new_game_name);

	private:
		memory::string PROPERTY(game_name);
	};
}
