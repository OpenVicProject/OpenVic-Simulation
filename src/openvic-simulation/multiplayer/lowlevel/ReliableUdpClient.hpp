#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <reliable.h>

#include "openvic-simulation/multiplayer/lowlevel/Constants.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpClient.hpp"

namespace OpenVic {
	struct ReliableUdpClient : UdpClient {
		ReliableUdpClient(reliable_config_t config = { .name = "client" });

		using sequence_type = reliable_udp_sequence_type;

		NetworkError store_packet(IpAddress p_ip, NetworkSocket::port_type p_port, std::span<uint8_t> p_buf) override;

		void update(double time);
		void reset();

		sequence_type get_current_sequence_value() const;
		sequence_type get_next_sequence_value() const;

		std::span<sequence_type> get_acknowledged_sequences() const;
		void clear_acknowledged_sequences();

		float get_round_trip_smooth_average() const;
		float get_round_trip_average() const;
		float get_round_trip_minimum() const;
		float get_round_trip_maximum() const;

		float get_jitter_average() const;
		float get_jitter_maximum() const;
		float get_jitter_against_average_round_trip() const;

		float get_packet_loss() const;

		struct BandwidthStatistics {
			float sent_bandwidth_kbps;
			float received_bandwidth_kbps;
			float acked_bandwidth_kpbs;
		};
		BandwidthStatistics get_bandwidth_statistics() const;

		size_t get_packets_sent() const;
		size_t get_packets_received() const;
		size_t get_packets_acknowledged() const;
		size_t get_packets_stale() const;
		size_t get_packets_invalid() const;
		size_t get_packets_too_large_to_send() const;
		size_t get_packets_too_large_to_receive() const;
		size_t get_fragments_sent() const;
		size_t get_fragments_received() const;
		size_t get_fragments_invalid() const;

	protected:
		NetworkError _set_packet(std::span<uint8_t> buffer) override;

	private:
		static void _transmit_packet(void* context, uint64_t id, uint16_t sequence, uint8_t* packet_data, int packet_size);
		static int _process_packet(void* context, uint64_t ud, uint16_t sequence, uint8_t* packet_data, int packet_size);

		void _default_config_values(reliable_config_t& config);

		// Ensures we track an accurate allocation/deallocation size for reliable_endpoint_t
		struct reliable_endpoint_dummy {
			void* allocator_context;
			void* (*allocate_function)(void*, size_t);
			void (*free_function)(void*, void*);
			struct reliable_config_t config;
			double time;
			float rtt;
			float rtt_min;
			float rtt_max;
			float rtt_avg;
			float jitter_avg_vs_min_rtt;
			float jitter_max_vs_min_rtt;
			float jitter_stddev_vs_avg_rtt;
			float packet_loss;
			float sent_bandwidth_kbps;
			float received_bandwidth_kbps;
			float acked_bandwidth_kbps;
			int num_acks;
			uint16_t* acks;
			uint16_t sequence;
			float* rtt_history_buffer;
			struct reliable_sequence_buffer_t* sent_packets;
			struct reliable_sequence_buffer_t* received_packets;
			struct reliable_sequence_buffer_t* fragment_reassembly;
			uint64_t counters[RELIABLE_ENDPOINT_NUM_COUNTERS];
		};

		struct endpoint_deleter_t {
			using value_type = reliable_endpoint_t;

			void operator()(value_type* ptr) {
				reliable_endpoint_destroy(ptr);
#ifdef DEBUG_ENABLED
				{
					utility::MemoryTracker tracker {};
					tracker.on_node_deallocation(nullptr, sizeof(reliable_endpoint_dummy), 0);
				}
#endif
			}
		};
		using endpoint_pointer_type = std::unique_ptr<endpoint_deleter_t::value_type, endpoint_deleter_t>;

		endpoint_pointer_type _endpoint;
	};
}
