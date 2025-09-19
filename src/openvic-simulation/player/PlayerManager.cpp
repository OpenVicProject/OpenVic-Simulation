#include "PlayerManager.hpp"

#include "openvic-simulation/multiplayer/Constants.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

const Player Player::INVALID_PLAYER {};

Player::Player(client_id_type client_id, memory::string name) : client_id { client_id }, name { name } {}

void Player::set_name(memory::string name) {
	player_changed();
	this->name = name;
}

void Player::set_country(CountryInstance* instance) {
	player_changed();
	country = instance;
}
