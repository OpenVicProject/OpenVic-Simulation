
#include "ReliableUdpClient.hpp"

#include <cstring>

#include <reliable.h>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpClient.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/MemoryTracker.hpp"

using namespace OpenVic;

void ReliableUdpClient::_default_config_values(reliable_config_t& config) {
	if (config.fragment_above == 0) {
		config.fragment_above = 1024;
	}
	if (config.max_fragments == 0) {
		config.max_fragments = 3;
	}
	if (config.fragment_size == 0) {
		config.fragment_size = 1024;
	}
	if (config.ack_buffer_size == 0) {
		config.ack_buffer_size = 256;
	}
	if (config.sent_packets_buffer_size == 0) {
		config.sent_packets_buffer_size = 256;
	}
	if (config.received_packets_buffer_size == 0) {
		config.received_packets_buffer_size = 256;
	}
	if (config.fragment_reassembly_buffer_size == 0) {
		config.fragment_reassembly_buffer_size = 64;
	}
	if (config.rtt_smoothing_factor == 0) {
		config.rtt_smoothing_factor = 0.0025f;
	}
	if (config.rtt_history_size == 0) {
		config.rtt_history_size = 512;
	}
	if (config.packet_loss_smoothing_factor == 0) {
		config.packet_loss_smoothing_factor = 0.1f;
	}
	if (config.bandwidth_smoothing_factor == 0) {
		config.bandwidth_smoothing_factor = 0.1f;
	}
	if (config.packet_header_size == 0) {
		config.packet_header_size = 48;
	}
	if (config.max_packet_size == 0) {
		config.max_packet_size = max_packet_size() - config.packet_header_size;
	}
}

ReliableUdpClient::ReliableUdpClient(reliable_config_t config) {
	_default_config_values(config);

	config.context = this;
	config.transmit_packet_function = &_transmit_packet;
	config.process_packet_function = &_process_packet;

	_endpoint = endpoint_pointer_type {
		reliable_endpoint_create(&config, static_cast<double>(GameManager::get_elapsed_milliseconds()) / 1000.0)
	};
#ifdef DEBUG_ENABLED
	{
		utility::MemoryTracker tracker {};
		tracker.on_node_allocation(nullptr, sizeof(reliable_endpoint_dummy), 0);
	}
#endif

	OV_ERR_FAIL_COND(!_endpoint);
}

thread_local NetworkError transmit_error;

void ReliableUdpClient::_transmit_packet(void* context, uint64_t id, uint16_t sequence, uint8_t* packet_data, int packet_size) {
	ReliableUdpClient* self = static_cast<ReliableUdpClient*>(context);
	transmit_error = self->UdpClient::_set_packet({ packet_data, static_cast<size_t>(packet_size) });
}

NetworkError ReliableUdpClient::_set_packet(std::span<uint8_t> buffer) {
	OV_ERR_FAIL_COND_V(!_endpoint, NetworkError::UNAVAILABLE);

	reliable_endpoint_send_packet(_endpoint.get(), buffer.data(), buffer.size_bytes());
	// Pretty sure floating point for packet tracking shouldn't cause issues
	update(static_cast<double>(GameManager::get_elapsed_milliseconds()) / 1000.0);
	return transmit_error;
}

struct PeerProcessData {
	IpAddress& p_ip;
	NetworkSocket::port_type& p_port;
};

thread_local PeerProcessData* process_peer_data = nullptr;

int ReliableUdpClient::_process_packet(void* context, uint64_t id, uint16_t sequence, uint8_t* packet_data, int packet_size) {
	ReliableUdpClient* self = static_cast<ReliableUdpClient*>(context);
	std::span<uint8_t> packet { packet_data, static_cast<size_t>(packet_size) };

	if (self->UdpClient::store_packet(process_peer_data->p_ip, process_peer_data->p_port, packet) == NetworkError::OK) {
		return 1;
	}

	process_peer_data = nullptr;
	return 0;
}

NetworkError ReliableUdpClient::store_packet(IpAddress p_ip, NetworkSocket::port_type p_port, std::span<uint8_t> p_buf) {
	OV_ERR_FAIL_COND_V(!_endpoint, NetworkError::UNAVAILABLE);

	PeerProcessData data = { p_ip, p_port };
	process_peer_data = &data;
	reliable_endpoint_receive_packet(_endpoint.get(), p_buf.data(), p_buf.size_bytes());
	if (process_peer_data == nullptr) {
		return NetworkError::OUT_OF_MEMORY;
	}
	process_peer_data = nullptr;
	return NetworkError::OK;
}

void ReliableUdpClient::update(double time) {
	reliable_endpoint_update(_endpoint.get(), time);
}

void ReliableUdpClient::reset() {
	reliable_endpoint_reset(_endpoint.get());
}

ReliableUdpClient::sequence_type ReliableUdpClient::get_current_sequence_value() const {
	return get_next_sequence_value() - 1;
}

ReliableUdpClient::sequence_type ReliableUdpClient::get_next_sequence_value() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_next_packet_sequence(_endpoint.get());
}

std::span<ReliableUdpClient::sequence_type> ReliableUdpClient::get_acknowledged_sequences() const {
	OV_ERR_FAIL_COND_V(!_endpoint, {});

	int32_t acks_count;
	uint16_t* acks_array = reliable_endpoint_get_acks(_endpoint.get(), &acks_count);
	OV_ERR_FAIL_COND_V(acks_count <= 0, {});

	return { acks_array, static_cast<size_t>(acks_count) };
}

void ReliableUdpClient::clear_acknowledged_sequences() {
	OV_ERR_FAIL_COND(!_endpoint);

	reliable_endpoint_clear_acks(_endpoint.get());
}

float ReliableUdpClient::get_round_trip_smooth_average() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_rtt(_endpoint.get());
}

float ReliableUdpClient::get_round_trip_average() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_rtt_avg(_endpoint.get());
}

float ReliableUdpClient::get_round_trip_minimum() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_rtt_min(_endpoint.get());
}

float ReliableUdpClient::get_round_trip_maximum() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_rtt_max(_endpoint.get());
}

float ReliableUdpClient::get_jitter_average() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_jitter_avg_vs_min_rtt(_endpoint.get());
}

float ReliableUdpClient::get_jitter_maximum() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_jitter_max_vs_min_rtt(_endpoint.get());
}

float ReliableUdpClient::get_jitter_against_average_round_trip() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_jitter_stddev_vs_avg_rtt(_endpoint.get());
}

float ReliableUdpClient::get_packet_loss() const {
	OV_ERR_FAIL_COND_V(!_endpoint, 0);

	return reliable_endpoint_packet_loss(_endpoint.get());
}

ReliableUdpClient::BandwidthStatistics ReliableUdpClient::get_bandwidth_statistics() const {
	OV_ERR_FAIL_COND_V(!_endpoint, {});

	BandwidthStatistics result; // NOLINT
	reliable_endpoint_bandwidth(
		_endpoint.get(), &result.sent_bandwidth_kbps, &result.received_bandwidth_kbps, &result.acked_bandwidth_kpbs
	);
	return result;
}

size_t ReliableUdpClient::get_packets_sent() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT];
}

size_t ReliableUdpClient::get_packets_received() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED];
}

size_t ReliableUdpClient::get_packets_acknowledged() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED];
}

size_t ReliableUdpClient::get_packets_stale() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_STALE];
}

size_t ReliableUdpClient::get_packets_invalid() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_INVALID];
}

size_t ReliableUdpClient::get_packets_too_large_to_send() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_SEND];
}

size_t ReliableUdpClient::get_packets_too_large_to_receive() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE];
}

size_t ReliableUdpClient::get_fragments_sent() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_SENT];
}

size_t ReliableUdpClient::get_fragments_received() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED];
}

size_t ReliableUdpClient::get_fragments_invalid() const {
	return reliable_endpoint_counters(_endpoint.get())[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID];
}
