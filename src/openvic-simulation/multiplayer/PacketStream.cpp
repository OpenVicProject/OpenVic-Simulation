
#include "PacketStream.hpp"

#include <bit>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "openvic-simulation/multiplayer/NetworkError.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Marshal.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

void PacketStream::set_big_endian(bool endian) {
	_is_big_endian = endian;
}

bool PacketStream::is_big_endian() const {
	return _is_big_endian;
}

void PacketStream::put_8(int8_t p_val) {
	set_data({ (uint8_t*)&p_val, 1 });
}

void PacketStream::put_u8(uint8_t p_val) {
	set_data({ (uint8_t*)&p_val, 1 });
}

void PacketStream::put_16(int16_t p_val) {
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	} else {
		if (_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	}
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT
	utility::encode(p_val, buf);
	set_data(buf);
}

void PacketStream::put_u16(uint16_t p_val) {
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	} else {
		if (_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	}
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT
	utility::encode(p_val, buf);
	set_data(buf);
}

void PacketStream::put_32(int32_t p_val) {
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	} else {
		if (_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	}
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT
	utility::encode(p_val, buf);
	set_data(buf);
}

void PacketStream::put_u32(uint32_t p_val) {
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	} else {
		if (_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	}
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT
	utility::encode(p_val, buf);
	set_data(buf);
}

void PacketStream::put_64(int64_t p_val) {
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	} else {
		if (_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	}
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT
	utility::encode(p_val, buf);
	set_data(buf);
}

void PacketStream::put_u64(uint64_t p_val) {
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	} else {
		if (_is_big_endian) {
			p_val = utility::byteswap(p_val);
		}
	}
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT
	utility::encode(p_val, buf);
	set_data(buf);
}

void PacketStream::put_half(float p_val) {
	std::array<uint8_t, sizeof(p_val) / 2> buf; // NOLINT

	utility::encode(utility::half_float { p_val }, buf);
	uint16_t* p16 = (uint16_t*)buf.data();
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			*p16 = utility::byteswap(*p16);
		}
	} else {
		if (_is_big_endian) {
			*p16 = utility::byteswap(*p16);
		}
	}

	set_data(buf);
}

void PacketStream::put_float(float p_val) {
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT

	utility::encode(p_val, buf);
	uint32_t* p32 = (uint32_t*)buf.data();
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			*p32 = utility::byteswap(*p32);
		}
	} else {
		if (_is_big_endian) {
			*p32 = utility::byteswap(*p32);
		}
	}

	set_data(buf);
}

void PacketStream::put_double(double p_val) {
	std::array<uint8_t, sizeof(p_val)> buf; // NOLINT

	utility::encode(p_val, buf);
	uint64_t* p64 = (uint64_t*)buf.data();
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			*p64 = utility::byteswap(*p64);
		}
	} else {
		if (_is_big_endian) {
			*p64 = utility::byteswap(*p64);
		}
	}

	set_data(buf);
}

void PacketStream::put_string(std::string_view p_string) {
	put_u32(p_string.length());
	set_data({ (uint8_t*)p_string.data(), p_string.length() });
}

uint8_t PacketStream::get_u8() {
	std::array<uint8_t, 1> buf {};
	get_data(buf);
	return buf[0];
}

int8_t PacketStream::get_8() {
	std::array<uint8_t, 1> buf {};
	get_data(buf);
	return static_cast<int8_t>(buf[0]);
}

uint16_t PacketStream::get_u16() {
	std::array<uint8_t, 2> buf; // NOLINT
	get_data(buf);

	uint16_t r = utility::decode<uint16_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return r;
}

int16_t PacketStream::get_16() {
	std::array<uint8_t, 2> buf; // NOLINT
	get_data(buf);

	uint16_t r = utility::decode<uint16_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return static_cast<int16_t>(r);
}

uint32_t PacketStream::get_u32() {
	std::array<uint8_t, 4> buf; // NOLINT
	get_data(buf);

	uint32_t r = utility::decode<uint32_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return r;
}

int32_t PacketStream::get_32() {
	std::array<uint8_t, 4> buf; // NOLINT
	get_data(buf);

	uint32_t r = utility::decode<uint32_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return static_cast<int32_t>(r);
}

uint64_t PacketStream::get_u64() {
	std::array<uint8_t, 8> buf; // NOLINT
	get_data(buf);

	uint64_t r = utility::decode<uint64_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return r;
}

int64_t PacketStream::get_64() {
	std::array<uint8_t, 8> buf; // NOLINT
	get_data(buf);

	uint64_t r = utility::decode<uint64_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return static_cast<int64_t>(r);
}

float PacketStream::get_half() {
	std::array<uint8_t, 2> buf; // NOLINT
	get_data(buf);

	uint16_t r = utility::decode<uint16_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return utility::decode<utility::half_float>(buf).value;
}

float PacketStream::get_float() {
	std::array<uint8_t, 4> buf; // NOLINT
	get_data(buf);

	uint32_t r = utility::decode<uint32_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return utility::decode<float>(buf);
}

double PacketStream::get_double() {
	std::array<uint8_t, 8> buf; // NOLINT
	get_data(buf);

	uint64_t r = utility::decode<uint64_t>(buf);
	if constexpr (std::endian::native == std::endian::big) {
		if (!_is_big_endian) {
			r = utility::byteswap(r);
		}
	} else {
		if (_is_big_endian) {
			r = utility::byteswap(r);
		}
	}

	return utility::decode<double>(buf);
}

std::string PacketStream::get_string(int64_t bytes) {
	std::string buf;

	if (bytes < 0) {
		bytes = get_32();
	}
	OV_ERR_FAIL_COND_V(bytes < 0, buf);

	buf.resize(bytes);
	NetworkError err = get_data({ (uint8_t*)&buf[0], static_cast<size_t>(bytes) });
	OV_ERR_FAIL_COND_V(err != NetworkError::OK, [&] {
		buf.clear();
		return buf;
	}());

	return buf;
}

NetworkError BufferPacketStream::set_data(std::span<uint8_t> buffer) {
	if (buffer.empty()) {
		return NetworkError::OK;
	}

	if (_position + buffer.size() > _data.size()) {
		_data.resize(_position + buffer.size());
	}

	std::memcpy(&_data[_position], buffer.data(), buffer.size());

	_position += buffer.size();
	return NetworkError::OK;
}

NetworkError BufferPacketStream::get_data(std::span<uint8_t> r_buffer) {
	size_t received;
	get_data_no_blocking(r_buffer, received);
	if (received != r_buffer.size()) {
		return NetworkError::INVALID_PARAMETER;
	}

	return NetworkError::OK;
}

NetworkError BufferPacketStream::set_data_no_blocking(std::span<uint8_t> buffer, size_t& r_sent) {
	r_sent = buffer.size();
	return set_data(buffer);
}

NetworkError BufferPacketStream::get_data_no_blocking(std::span<uint8_t> r_buffer, size_t& r_received) {
	if (r_buffer.empty()) {
		r_received = 0;
		return NetworkError::OK;
	}

	if (_position + r_buffer.size() > _data.size()) {
		r_received = _data.size() - _position;
		if (r_received <= 0) {
			r_received = 0;
			return NetworkError::OK; // you got 0
		}
	} else {
		r_received = r_buffer.size();
	}

	std::memcpy(r_buffer.data(), &_data[_position], r_received);

	_position += r_received;
	// FIXME: return what? OK or ERR_*
	// return OK for now so we don't maybe return garbage
	return NetworkError::OK;
}

int64_t BufferPacketStream::available_bytes() const {
	return _data.size() - _position;
}

void BufferPacketStream::seek(size_t p_pos) {
	OV_ERR_FAIL_COND(p_pos > _data.size());
	_position = p_pos;
}

size_t BufferPacketStream::size() const {
	return _data.size();
}

size_t BufferPacketStream::position() const {
	return _position;
}

void BufferPacketStream::resize(size_t p_size) {
	_data.resize(p_size);
}

void BufferPacketStream::set_buffer_data(std::span<uint8_t> p_data) {
	if (p_data.size() > _data.size()) {
		_data.resize(p_data.size());
	}

	std::uninitialized_copy(p_data.begin(), p_data.end(), _data.begin());
}

std::span<const uint8_t> BufferPacketStream::get_buffer_data() const {
	return _data;
}

void BufferPacketStream::clear() {
	_data.clear();
	_position = 0;
}

BufferPacketStream BufferPacketStream::duplicate() const {
	BufferPacketStream result;
	result._data.resize(_data.size());
	std::uninitialized_copy(_data.begin(), _data.end(), result._data.begin());
	return result;
}
