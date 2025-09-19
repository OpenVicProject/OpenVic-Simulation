#pragma once

#include "openvic-simulation/multiplayer/Constants.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GameManager;
	struct ClientManager;
	struct HostSession;
	struct CountryInstance;

	struct Player {
	private:
		friend struct GameManager;
		friend struct ClientManager;
		friend struct HostSession;

		memory::string PROPERTY(name);
		CountryInstance* PROPERTY_PTR(country, nullptr);
		client_id_type PROPERTY(client_id, MP_INVALID_CLIENT_ID);

		Player() = default;
		Player(client_id_type client_id, memory::string name);

	public:
		signal_property<Player> player_changed;

		static const Player INVALID_PLAYER;

		void set_name(memory::string name);
		void set_country(CountryInstance* instance);
	};
}
