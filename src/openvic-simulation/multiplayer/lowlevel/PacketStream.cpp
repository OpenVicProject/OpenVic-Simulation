
#include "PacketStream.hpp"

#include <cstdint>
#include <memory>
#include <span>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

NetworkError BufferPacketStream::set_data(std::span<const uint8_t> buffer) {
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

NetworkError BufferPacketStream::get_data(std::span<uint8_t> buffer_to_set) {
	size_t received;
	get_data_no_blocking(buffer_to_set, received);
	if (received != buffer_to_set.size()) {
		return NetworkError::INVALID_PARAMETER;
	}

	return NetworkError::OK;
}

NetworkError BufferPacketStream::set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) {
	r_sent = buffer.size();
	return set_data(buffer);
}

NetworkError BufferPacketStream::get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) {
	if (buffer_to_set.empty()) {
		r_received = 0;
		return NetworkError::OK;
	}

	if (_position + buffer_to_set.size() > _data.size()) {
		r_received = _data.size() - _position;
		if (r_received <= 0) {
			r_received = 0;
			return NetworkError::OK; // you got 0
		}
	} else {
		r_received = buffer_to_set.size();
	}

	std::memcpy(buffer_to_set.data(), &_data[_position], r_received);

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
