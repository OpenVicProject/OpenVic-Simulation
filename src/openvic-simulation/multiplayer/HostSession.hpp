#pragma once

#include <cstdint>
#include <span>

#include "openvic-simulation/multiplayer/Constants.hpp"
#include "openvic-simulation/player/PlayerManager.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	struct Player;

	struct HostSession : observer {
		signal_property<HostSession> session_changed;

		HostSession(memory::string game_name = "HostSession");
		~HostSession();

		HostSession(HostSession const&) = delete;
		HostSession& operator=(HostSession const&) = delete;
		HostSession(HostSession&&);
		HostSession& operator=(HostSession&& other);

		template<std::endian Endian>
		size_t encode(std::span<uint8_t> span) const {
			size_t offset = utility::encode<decltype(game_name), Endian>(game_name, span);
			offset += utility::encode<decltype(players), Endian>(players, span.empty() ? span : span.subspan(offset));
			return offset;
		}

		template<std::endian Endian>
		static HostSession decode(std::span<uint8_t> span, size_t& r_decode_count) {
			HostSession result { utility::decode<decltype(game_name), Endian>(span, r_decode_count) };
			size_t offset = r_decode_count;
			decltype(players) players = utility::decode<decltype(players), Endian>(span.subspan(offset), r_decode_count);
			r_decode_count += offset;
			result.players.swap(players);
			return result;
		}

		void set_game_name(memory::string new_game_name);

		Player* add_player(client_id_type client_id, memory::string player_name);
		bool remove_player(client_id_type client_id);

		Player* get_player_by(client_id_type client_id);

	private:
		memory::string PROPERTY(game_name);
		vector_ordered_map<client_id_type, Player> PROPERTY(players);
	};
}
