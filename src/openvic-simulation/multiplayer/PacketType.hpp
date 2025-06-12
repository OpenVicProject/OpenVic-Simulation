#pragma once

#include <any>
#include <array>
#include <cstdint>
#include <type_traits>

#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"

namespace OpenVic {
	struct BaseMultiplayerManager;

#define PACKET_LIST \
	X(retransmit_packet, true) \
	X(broadcast_packet, true) \
	X(send_raw_packet, true) \
	X(update_host_session, false) \
	X(execute_game_action, false) \
	X(send_chat_message, true) \
	X(send_battleplan, true)

	struct PacketType {
		using argument_type = std::any const&;

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
#define X(NAME, CAN_CLIENT_SEND) \
	bool NAME##_send_callback( \
		BaseMultiplayerManager const* multiplayer_manager, PacketType::argument_type argument, PacketBuilder<>& builder \
	); \
	bool NAME##_process_callback(BaseMultiplayerManager* multiplayer_manager, PacketSpan packet); \
	static constexpr PacketType NAME = { __COUNTER__, CAN_CLIENT_SEND, &NAME##_send_callback, &NAME##_process_callback };
		PACKET_LIST
#undef X

		static constexpr std::array _packet_types = std::to_array<PacketType const*>({
#define X(NAME, _) &NAME,
			PACKET_LIST
#undef X
		});
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
