#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include <zpp_bits.h>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct PacketStream {
		virtual NetworkError set_data(std::span<const uint8_t> buffer) = 0;
		virtual NetworkError get_data(std::span<uint8_t> buffer_to_set) = 0;
		virtual NetworkError set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) = 0;
		virtual NetworkError get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) = 0;
		virtual NetworkError get_data_waiting(std::span<uint8_t> buffer_to_set) = 0;
		virtual int64_t available_bytes() const = 0;

	private:
		template<typename ByteType = uint8_t>
		static constexpr auto _data_in(auto&&... option) {
			using namespace zpp::bits;
			struct data_in {
				data_in(decltype(option)&&... option) : input(data, std::forward<decltype(option)>(option)...) {}

				memory::vector<ByteType> data;
				zpp::bits::in<decltype(data), decltype(option)...> input;
			};
			return data_in { std::forward<decltype(option)>(option)... };
		}

	public:
		template<typename... Opts>
		auto data_in(size_t bytes, Opts&&... option) -> std::optional<decltype(_data_in(std::forward<Opts>(option)...))> {
			decltype(_data_in()) _in = _data_in(std::forward<Opts>(option)...);
			_in.data.resize(bytes);
			if (in(_in.data).has_value()) {
				return _in;
			}
			return std::nullopt;
		}

		std::optional<zpp::bits::in<memory::vector<uint8_t>>> in(memory::vector<uint8_t>& buffer_store) {
			if (get_data(buffer_store) != NetworkError::OK) {
				return std::nullopt;
			}
			return zpp::bits::in(buffer_store);
		}
	};

	struct BufferPacketStream : PacketStream {
		NetworkError set_data(std::span<const uint8_t> buffer) override;
		NetworkError get_data(std::span<uint8_t> r_buffer) override;

		NetworkError set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) override;
		NetworkError get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) override;

		NetworkError get_data_waiting(std::span<uint8_t> buffer_to_set) override;

		virtual int64_t available_bytes() const override;

		void seek(size_t p_pos);
		size_t size() const;
		size_t position() const;
		void resize(size_t p_size);

		void set_buffer_data(std::span<uint8_t> p_data);
		std::span<const uint8_t> get_buffer_data() const;

		void clear();

		BufferPacketStream duplicate() const;

	private:
		memory::vector<uint8_t> _data;
		size_t _position;
	};
}
