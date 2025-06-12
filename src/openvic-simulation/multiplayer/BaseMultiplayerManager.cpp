
#include "BaseMultiplayerManager.hpp"

#include <any>

#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

BaseMultiplayerManager::BaseMultiplayerManager(GameManager* game_manager) : game_manager(game_manager) {
	packet_cache.reserve_power(15);
}

bool BaseMultiplayerManager::broadcast_packet(PacketType const& type, std::any const& argument) {
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(type), false);
	return true;
}

bool BaseMultiplayerManager::send_packet(client_id_type client_id, PacketType const& type, std::any const& argument) {
	OV_ERR_FAIL_COND_V(!PacketType::is_valid_type(type), false);
	return true;
}
