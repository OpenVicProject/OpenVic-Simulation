
#include "ChatManager.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

ChatGroup::ChatGroup(index_t index, memory::vector<BaseMultiplayerManager::client_id_type>&& clients)
	: index { index }, clients { std::move(clients) } {}

ChatManager::ChatManager(GameManager* game_manager) : game_manager { game_manager } {}

bool ChatManager::send_private_message(BaseMultiplayerManager::client_id_type to, memory::string&& message) {
	MessageData data = PrivateMessageData { to, std::move(message) };

	bool was_sent = game_manager->get_client_manager()->broadcast_packet(PacketTypes::send_chat_message, &data);
	log_message(game_manager->get_client_manager()->get_client_id(), std::move(data));
	return was_sent;
}

bool ChatManager::send_public_message(memory::string&& message) {
	MessageData data = PublicMessageData { std::move(message) };

	bool was_sent = game_manager->get_client_manager()->broadcast_packet(PacketTypes::send_chat_message, &data);
	log_message(game_manager->get_client_manager()->get_client_id(), std::move(data));
	return was_sent;
}

bool ChatManager::send_group_message(ChatGroup const& group, memory::string&& message) {
	MessageData data = GroupMessageData { group.get_index(), std::move(message) };

	bool was_sent = game_manager->get_client_manager()->broadcast_packet(PacketTypes::send_chat_message, &data);
	log_message(game_manager->get_client_manager()->get_client_id(), std::move(data));
	return was_sent;
}

bool ChatManager::send_group_message(ChatGroup::index_t group_id, memory::string&& message) {
	OV_ERR_FAIL_INDEX_V(group_id, groups.size(), false);
	return send_group_message(groups[group_id], std::move(message));
}

void ChatManager::log_message(BaseMultiplayerManager::client_id_type from, MessageData&& message) {
	messages.try_emplace(from, std::move(message));
}

ChatGroup const& ChatManager::create_group(memory::vector<BaseMultiplayerManager::client_id_type>&& clients) {
	groups.push_back({ groups.size(), std::move(clients) });
	return groups.back();
}

ChatGroup const& ChatManager::get_group(ChatGroup::index_t group_index) const {
	return groups.at(group_index);
}

void ChatManager::delete_group(ChatGroup const& group) {
	delete_group(group.get_index());
}

void ChatManager::delete_group(ChatGroup::index_t group_id) {
	OV_ERR_FAIL_INDEX(group_id, groups.size());
	groups.erase(groups.begin() + group_id);
}
