#pragma once

#include <array>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "openvic-simulation/misc/GameAction.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"

namespace OpenVic {
	struct BaseMultiplayerManager;

	using PacketChatGroupModifyData = std::pair<size_t, std::span<uint64_t>>;

	// F_TYPE(packet_name, can_client_send, packet_data_type)
	// F(packet_name, can_client_send)
#define PACKET_LIST(F, F_TYPE) \
	F_TYPE(retransmit_packet, true, RetransmitData const*) \
	F_TYPE(broadcast_packet, true, BroadcastData const*) \
	F_TYPE(send_raw_packet, true, std::span<uint8_t>) \
	F_TYPE(update_host_session, false, HostSession const*) \
	F_TYPE(execute_game_action, false, game_action_t const*) \
	F_TYPE(send_chat_message, true, ChatMessageLog const*) \
	F_TYPE(add_chat_group, true, std::span<uint64_t>) \
	F_TYPE(modify_chat_group, true, PacketChatGroupModifyData) \
	F_TYPE(delete_chat_group, true, size_t) \
	F(send_battleplan, true) \
	F_TYPE(notify_player_left, false, uint64_t) \
	F_TYPE(set_ready_status, true, bool)

	struct RetransmitData;
	struct BroadcastData;
	struct HostSession;
	struct ChatMessageLog;

	struct PacketType {
#define DEFINE_TYPE(_, _2, DATA_TYPE) DATA_TYPE,
#define IGNORE(_, _2) std::monostate,

		using argument_type = std::variant<PACKET_LIST(IGNORE, DEFINE_TYPE) std::monostate>;

#undef DEFINE_TYPE
#undef IGNORE

		using packet_type_send_callback_t = std::add_pointer_t<bool( //
			BaseMultiplayerManager const* multiplayer_manager, argument_type argument, PacketBuilder<>& builder
		)>;
		using packet_type_process_callback_t =
			std::add_pointer_t<bool(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet)>;

		uint16_t packet_id;
		bool can_client_send;
		packet_type_send_callback_t create_packet;
		packet_type_process_callback_t process_packet;

		static constexpr bool is_valid_type(decltype(packet_id) id);
		static constexpr PacketType const& get_type_by_id(decltype(packet_id) id);

		constexpr operator decltype(packet_id)() const {
			return packet_id;
		}
	};

	namespace PacketTypes {
#define DEFINE_PACKET_TYPE(NAME, CAN_CLIENT_SEND, ...) \
	bool NAME##_send_callback( \
		BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder \
	); \
	bool NAME##_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet); \
	static constexpr PacketType NAME = { __COUNTER__, CAN_CLIENT_SEND, &NAME##_send_callback, &NAME##_process_callback };

		PACKET_LIST(DEFINE_PACKET_TYPE, DEFINE_PACKET_TYPE)

#undef DEFINE_PACKET_TYPE

#define DEFINE_PACKET_TYPE_IN_ARRAY(NAME, _, ...) &NAME,

		static constexpr std::array _packet_types = std::to_array<PacketType const*>({
			PACKET_LIST(DEFINE_PACKET_TYPE_IN_ARRAY, DEFINE_PACKET_TYPE_IN_ARRAY) //
		});

#undef DEFINE_PACKET_TYPE_IN_ARRAY
	};

	constexpr bool PacketType::is_valid_type(decltype(PacketType::packet_id) id) {
		return id < PacketTypes::_packet_types.size();
	}

	constexpr PacketType const& PacketType::get_type_by_id(decltype(PacketType::packet_id) id) {
		return *PacketTypes::_packet_types[id];
	}

#undef PACKET_LIST

	struct RetransmitData {
		uint64_t client_id;
		decltype(PacketType::packet_id) packet_id;
		PacketType::argument_type argument;
	};

	struct BroadcastData {
		decltype(PacketType::packet_id) packet_id;
		PacketType::argument_type argument;
	};
}
