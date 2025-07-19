
#include "HostSession.hpp"

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

HostSession::HostSession(memory::string game_name) : game_name { game_name } {}

HostSession& HostSession::operator=(HostSession&& other) {
	std::swap(game_name, other.game_name);
	session_changed(*this);
	return *this;
}

void HostSession::set_game_name(memory::string new_game_name) {
	game_name = new_game_name;
	session_changed(*this);
}
