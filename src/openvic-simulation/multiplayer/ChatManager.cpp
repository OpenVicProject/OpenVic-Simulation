
#include "ChatManager.hpp"

#include <chrono>
#include <utility>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/PacketType.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

ChatGroup::ChatGroup(index_t index, memory::vector<BaseMultiplayerManager::client_id_type>&& clients)
	: index { index }, clients { std::move(clients) } {}

ChatManager::ChatManager(GameManager* game_manager) : game_manager { game_manager } {}

bool ChatManager::send_private_message(BaseMultiplayerManager::client_id_type to, memory::string&& message) {
	MessageData data = PrivateMessageData { to, std::move(message) };

	ChatMessageLog const& log = log_message(game_manager->get_client_manager()->get_client_id(), std::move(data));
	bool was_sent = game_manager->get_client_manager()->broadcast_packet(PacketTypes::send_chat_message, &log);
	return was_sent;
}

bool ChatManager::send_public_message(memory::string&& message) {
	MessageData data = PublicMessageData { std::move(message) };

	ChatMessageLog const& log = log_message(game_manager->get_client_manager()->get_client_id(), std::move(data));
	bool was_sent = game_manager->get_client_manager()->broadcast_packet(PacketTypes::send_chat_message, &log);
	return was_sent;
}

bool ChatManager::send_group_message(ChatGroup const& group, memory::string&& message) {
	MessageData data = GroupMessageData { group.get_index(), std::move(message) };

	ChatMessageLog const& log = log_message(game_manager->get_client_manager()->get_client_id(), std::move(data));
	bool was_sent = game_manager->get_client_manager()->broadcast_packet(PacketTypes::send_chat_message, &log);
	return was_sent;
}

bool ChatManager::send_group_message(ChatGroup::index_t group_id, memory::string&& message) {
	OV_ERR_FAIL_INDEX_V(group_id, groups.size(), false);
	return send_group_message(groups[group_id], std::move(message));
}

ChatMessageLog const& ChatManager::log_message( //
	BaseMultiplayerManager::client_id_type from, MessageData&& message,
	std::chrono::time_point<std::chrono::system_clock> timestamp
) {
	ChatMessageLog const& log = messages.emplace_back(
		from, std::move(message), std::chrono::time_point_cast<std::chrono::milliseconds>(timestamp).time_since_epoch().count()
	);
	message_logged(log);
	return log;
}

ChatMessageLog const& ChatManager::_log_message(ChatMessageLog&& log) {
	ChatMessageLog const& moved_log = messages.emplace_back(std::move(log));
	message_logged(moved_log);
	return moved_log;
}

memory::vector<ChatMessageLog> const& ChatManager::get_message_logs() const {
	return messages;
}

void ChatManager::create_group(memory::vector<BaseMultiplayerManager::client_id_type>&& clients) {
	game_manager->get_client_manager()->broadcast_packet(PacketTypes::add_chat_group, clients);
}

void ChatManager::_create_group(memory::vector<BaseMultiplayerManager::client_id_type>&& clients) {
	groups.push_back({ groups.size(), std::move(clients) });
	ChatGroup const& last = groups.back();
	group_created(last);
}

void ChatManager::set_group(ChatGroup::index_t group_id, memory::vector<BaseMultiplayerManager::client_id_type>&& clients) {
	OV_ERR_FAIL_INDEX(group_id, groups.size());
	game_manager->get_client_manager()->broadcast_packet(PacketTypes::modify_chat_group, clients);
}

void ChatManager::_set_group(ChatGroup::index_t group_id, memory::vector<BaseMultiplayerManager::client_id_type>&& clients) {
	ChatGroup& group = groups[group_id];
	std::swap(group.clients, clients);
	group_modified(group, clients);
}

ChatGroup const& ChatManager::get_group(ChatGroup::index_t group_index) const {
	return groups.at(group_index);
}

void ChatManager::delete_group(ChatGroup::index_t group_id) {
	OV_ERR_FAIL_INDEX(group_id, groups.size());
	game_manager->get_client_manager()->broadcast_packet(
		PacketTypes::delete_chat_group,
		PacketType::argument_type { std::in_place_index<PacketTypes::delete_chat_group.packet_id>, group_id }
	);
}

void ChatManager::_delete_group(ChatGroup::index_t group_id) {
	groups.erase(groups.begin() + group_id);
	group_deleted(group_id);
}
