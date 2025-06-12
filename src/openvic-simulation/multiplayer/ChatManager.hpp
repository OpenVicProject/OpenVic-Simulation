#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	struct GameManager;
	struct ChatManager;
	struct ChatMessageLog;

	struct ChatGroup {
		using index_t = size_t;

		ChatGroup(ChatGroup const&) = delete;
		ChatGroup& operator=(ChatGroup const&) = delete;
		ChatGroup(ChatGroup&&) = default;
		ChatGroup& operator=(ChatGroup&& lhs) = default;

		std::span<const BaseMultiplayerManager::client_id_type> get_clients() const {
			return clients;
		}

		operator index_t() const {
			return index;
		}

	private:
		friend struct ChatManager;
		ChatGroup(index_t index, memory::vector<BaseMultiplayerManager::client_id_type>&& clients);

		memory::vector<BaseMultiplayerManager::client_id_type> clients;
		index_t PROPERTY(index);
	};

	struct ChatManager {
		enum class MessageType : uint8_t { NONE, PRIVATE, GROUP, PUBLIC };

		struct MessageData {
			MessageType type = MessageType::NONE;
			memory::string message;
			uint64_t from_index = BaseMultiplayerManager::HOST_ID;

			template<std::endian Endian>
			size_t encode(std::span<uint8_t> span) const {
				size_t offset = utility::encode(type, span, utility::endian_tag<Endian>);
				if (type != MessageType::PUBLIC) {
					offset += utility::encode<
						uint16_t>(from_index, !span.empty() ? span.subspan(offset) : span, utility::endian_tag<Endian>);
				}

				return utility::encode(message, !span.empty() ? span.subspan(offset) : span, utility::endian_tag<Endian>) +
					offset;
			}

			template<std::endian Endian>
			static MessageData decode(std::span<uint8_t> span, size_t& r_decode_count) {
				MessageType type = utility::decode<MessageType, Endian>(span, r_decode_count);
				size_t offset = r_decode_count;

				uint64_t from_index = BaseMultiplayerManager::HOST_ID;
				if (type != MessageType::PUBLIC) {
					from_index = utility::decode<uint16_t>(span.subspan(offset), r_decode_count);
					offset += r_decode_count;
				}

				MessageData data {
					type, utility::decode<decltype(message), Endian>(span.subspan(offset), r_decode_count), from_index //
				};
				r_decode_count += offset;
				return data;
			}
		};

		ChatManager(ClientManager* client_manager = nullptr);

		bool send_private_message(BaseMultiplayerManager::client_id_type to, memory::string&& message);
		bool send_private_message(BaseMultiplayerManager::client_id_type to, std::string_view message);
		bool send_public_message(memory::string&& message);
		bool send_public_message(std::string_view message);
		bool send_group_message(ChatGroup const& group, memory::string&& message);
		bool send_group_message(ChatGroup::index_t group_id, memory::string&& message);
		bool send_group_message(ChatGroup const& group, std::string_view message);
		bool send_group_message(ChatGroup::index_t group_id, std::string_view message);

		signal_property<ChatManager, ChatMessageLog const&> message_logged;

		ChatMessageLog const& log_message(
			BaseMultiplayerManager::client_id_type from, MessageData&& message,
			std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now()
		);
		memory::vector<ChatMessageLog> const& get_message_logs() const;

		signal_property<ChatManager, ChatGroup const&> group_created;
		// group, old clients
		signal_property<ChatManager, ChatGroup const&, memory::vector<BaseMultiplayerManager::client_id_type>&> group_modified;
		signal_property<ChatManager, ChatGroup::index_t> group_deleted;

		void create_group(memory::vector<BaseMultiplayerManager::client_id_type>&& clients);
		void set_group(ChatGroup::index_t group, memory::vector<BaseMultiplayerManager::client_id_type>&& clients);
		ChatGroup const& get_group(ChatGroup::index_t group_index) const;
		void delete_group(ChatGroup::index_t group_id);

	private:
		ClientManager* PROPERTY_PTR(client_manager);

		memory::deque<ChatGroup> groups;
		memory::vector<ChatMessageLog> messages;

		friend bool PacketTypes::send_chat_message_process_callback( //
			BaseMultiplayerManager* multiplayer_manager, PacketSpan packet
		);
		ChatMessageLog const* _log_message(ChatMessageLog&& log);

		friend bool PacketTypes::add_chat_group_process_callback( //
			BaseMultiplayerManager* multiplayer_manager, PacketSpan packet
		);
		void _create_group(memory::vector<BaseMultiplayerManager::client_id_type>&& clients);

		friend bool PacketTypes::modify_chat_group_process_callback( //
			BaseMultiplayerManager* multiplayer_manager, PacketSpan packet
		);
		void _set_group(ChatGroup::index_t group, memory::vector<BaseMultiplayerManager::client_id_type>&& clients);

		friend bool PacketTypes::delete_chat_group_process_callback( //
			BaseMultiplayerManager* multiplayer_manager, PacketSpan packet
		);
		void _delete_group(ChatGroup::index_t group_id);
	};

	struct ChatMessageLog {
		BaseMultiplayerManager::client_id_type from_id = BaseMultiplayerManager::HOST_ID;
		ChatManager::MessageData data;
		int64_t timestamp = 0;

		ChatMessageLog() = default;
		ChatMessageLog(BaseMultiplayerManager::client_id_type from, ChatManager::MessageData data, int64_t timestamp)
			: from_id { from }, data { data }, timestamp { timestamp } {}

		template<std::endian Endian>
		size_t encode(std::span<uint8_t> span) const {
			size_t offset = utility::encode<uint16_t>(from_id, span, utility::endian_tag<Endian>);
			offset += utility::encode(data, span.empty() ? span : span.subspan(offset), utility::endian_tag<Endian>);
			offset += utility::encode(timestamp, span.empty() ? span : span.subspan(offset), utility::endian_tag<Endian>);
			return offset;
		}

		template<std::endian Endian>
		static ChatMessageLog decode(std::span<uint8_t> span, size_t& r_decode_count) {
			size_t offset = 0;

			uint16_t from_id = utility::decode<decltype(from_id), Endian>(span, r_decode_count);
			offset += r_decode_count;
			decltype(data) data = utility::decode<decltype(data), Endian>(span.subspan(offset), r_decode_count);
			offset += r_decode_count;
			decltype(timestamp) timestamp = utility::decode<decltype(timestamp), Endian>(span.subspan(offset), r_decode_count);
			r_decode_count = offset;

			return { std::move(from_id), std::move(data), std::move(timestamp) };
		}
	};
}
