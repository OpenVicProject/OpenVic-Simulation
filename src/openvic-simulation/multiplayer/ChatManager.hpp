#pragma once

#include <chrono>
#include <cstdint>
#include <variant>

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
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
		struct PrivateMessageData {
			BaseMultiplayerManager::client_id_type to_client = BaseMultiplayerManager::HOST_ID;
			memory::string message;

			template<std::endian Endian>
			size_t encode(std::span<uint8_t> span) const {
				size_t offset = utility::encode<uint16_t>(to_client, span, utility::endian_tag<Endian>);
				return utility::encode(message, !span.empty() ? span.subspan(offset) : span, utility::endian_tag<Endian>) +
					offset;
			}

			template<std::endian Endian>
			static PrivateMessageData decode(std::span<uint8_t> span, size_t& r_decode_count) {
				BaseMultiplayerManager::client_id_type to_client = utility::decode<uint16_t, Endian>(span, r_decode_count);
				const size_t offset = r_decode_count;
				PrivateMessageData data {
					to_client, utility::decode<decltype(message), Endian>(span.subspan(offset), r_decode_count) //
				};
				r_decode_count += offset;
				return data;
			}
		};

		struct GroupMessageData {
			ChatGroup::index_t group_index;
			memory::string message;

			template<std::endian Endian>
			size_t encode(std::span<uint8_t> span) const {
				size_t offset = utility::encode<uint16_t>(group_index, span, utility::endian_tag<Endian>);
				return utility::encode(message, !span.empty() ? span.subspan(offset) : span, utility::endian_tag<Endian>) +
					offset;
			}

			template<std::endian Endian>
			static GroupMessageData decode(std::span<uint8_t> span, size_t& r_decode_count) {
				size_t group_index = utility::decode<uint16_t, Endian>(span, r_decode_count);
				const size_t offset = r_decode_count;
				GroupMessageData data {
					group_index, utility::decode<decltype(message), Endian>(span.subspan(offset), r_decode_count) //
				};
				r_decode_count += offset;
				return data;
			}
		};

		struct PublicMessageData {
			memory::string message;

			template<std::endian Endian>
			size_t encode(std::span<uint8_t> span) const {
				return utility::encode(message, span, utility::endian_tag<Endian>);
			}

			template<std::endian Endian>
			static PublicMessageData decode(std::span<uint8_t> span, size_t& r_decode_count) {
				return { utility::decode<decltype(message), Endian>(span, r_decode_count) };
			}
		};

		using MessageData = std::variant<std::monostate, PrivateMessageData, GroupMessageData, PublicMessageData>;


		ChatManager(GameManager* game_manager = nullptr);

		bool send_private_message(BaseMultiplayerManager::client_id_type to, memory::string&& message);
		bool send_public_message(memory::string&& message);
		bool send_group_message(ChatGroup const& group, memory::string&& message);
		bool send_group_message(ChatGroup::index_t group_id, memory::string&& message);

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
		GameManager* PROPERTY_PTR(game_manager);

		memory::deque<ChatGroup> groups;
		memory::vector<ChatMessageLog> messages;

		friend bool PacketTypes::send_chat_message_process_callback( //
			BaseMultiplayerManager* multiplayer_manager, PacketSpan packet
		);
		ChatMessageLog const& _log_message(ChatMessageLog&& log);

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
			offset += utility::encode(data, span.subspan(offset), utility::endian_tag<Endian>);
			offset += utility::encode(timestamp, span.subspan(offset), utility::endian_tag<Endian>);
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
