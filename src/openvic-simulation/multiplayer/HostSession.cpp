#include "HostSession.hpp"

#include <utility>

#include "openvic-simulation/multiplayer/Constants.hpp"
#include "openvic-simulation/player/PlayerManager.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

HostSession::HostSession(memory::string game_name) : game_name { game_name } {}

HostSession::~HostSession() {
	this->disconnect_all();
}

HostSession::HostSession(HostSession&& other) {
	std::swap(game_name, other.game_name);
	std::swap(players, other.players);
	other.session_changed();
}

HostSession& HostSession::operator=(HostSession&& other) {
	std::swap(game_name, other.game_name);
	std::swap(players, other.players);
	session_changed();
	return *this;
}

void HostSession::set_game_name(memory::string new_game_name) {
	game_name = new_game_name;
	session_changed();
}

Player* HostSession::add_player(client_id_type client_id, memory::string player_name) {
	std::pair<decltype(players)::iterator, bool> success =
		players.insert(std::make_pair(client_id, Player { client_id, player_name }));
	OV_ERR_FAIL_COND_V(!success.second, nullptr);
	success.first.value().player_changed.connect([this]() {
		session_changed();
	});
	session_changed();
	return &success.first.value();
}

bool HostSession::remove_player(client_id_type client_id) {
	OV_ERR_FAIL_COND_V(players.unordered_erase(client_id) != 1, false);
	session_changed();
	return true;
}

Player* HostSession::get_player_by(client_id_type client_id) {
	decltype(players)::iterator it = players.find(client_id);
	OV_ERR_FAIL_COND_V(it == players.end(), nullptr);
	return &it.value();
}
