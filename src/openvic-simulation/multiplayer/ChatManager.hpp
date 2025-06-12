#pragma once

#include "openvic-simulation/multiplayer/BaseMultiplayerManager.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Deque.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	struct GameManager;
	struct ChatManager;

	struct ChatGroup {
		using index_t = size_t;

		ChatGroup(ChatGroup const&) = delete;
		ChatGroup& operator=(ChatGroup const&) = delete;
		ChatGroup(ChatGroup&&) = default;
		ChatGroup& operator=(ChatGroup&& lhs) = default;

		std::span<const BaseMultiplayerManager::client_id_type> get_clients() const {
			return clients;
		}

	private:
		friend struct ChatManager;
		ChatGroup(index_t index, memory::vector<BaseMultiplayerManager::client_id_type>&& clients);

		memory::vector<BaseMultiplayerManager::client_id_type> clients;
		index_t PROPERTY(index);
	};

	struct ChatManager {
		struct PrivateMessageData {
			BaseMultiplayerManager::client_id_type to_client;
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

		using MessageData = std::variant<PrivateMessageData, GroupMessageData, PublicMessageData>;

		ChatManager(GameManager* game_manager = nullptr);

		bool send_private_message(BaseMultiplayerManager::client_id_type to, memory::string&& message);
		bool send_public_message(memory::string&& message);
		bool send_group_message(ChatGroup const& group, memory::string&& message);
		bool send_group_message(ChatGroup::index_t group_id, memory::string&& message);

		void log_message(BaseMultiplayerManager::client_id_type from, MessageData&& message);

		ChatGroup const& create_group(memory::vector<BaseMultiplayerManager::client_id_type>&& clients);
		ChatGroup const& get_group(ChatGroup::index_t group_index) const;
		void delete_group(ChatGroup const& group);
		void delete_group(ChatGroup::index_t group_id);

	private:
		GameManager* PROPERTY_PTR(game_manager);

		OpenVic::utility::deque<ChatGroup> groups;
		deque_ordered_map<BaseMultiplayerManager::client_id_type, MessageData> messages;
	};
}
