#include "openvic-simulation/core/container/RingBuffer.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("RingBuffer", "[RingBuffer]") {
	RingBuffer<uint16_t> buffer;

	CHECK(buffer.size() == 0);
	CHECK(buffer.capacity() == 1);
	buffer.push_back(1);
	CHECK(buffer.size() == 1);
	CHECK(buffer[0] == 1);

	buffer.reserve(8);

	CHECK(buffer.size() == 1);
	CHECK(buffer.capacity() == 8);
	buffer.push_back(2);
	buffer.push_back(3);
	buffer.push_back(4);
	buffer.push_back(5);
	buffer.push_back(6);
	buffer.push_back(7);
	buffer.push_back(8);
	CHECK(buffer.size() == 8);
	CHECK(buffer[0] == 1);
	CHECK(buffer[1] == 2);
	CHECK(buffer[2] == 3);
	CHECK(buffer[3] == 4);
	CHECK(buffer[4] == 5);
	CHECK(buffer[5] == 6);
	CHECK(buffer[6] == 7);
	CHECK(buffer[7] == 8);
	buffer.push_back(9);
	CHECK(buffer[0] == 2);
	CHECK(buffer[1] == 3);
	CHECK(buffer[2] == 4);
	CHECK(buffer[3] == 5);
	CHECK(buffer[4] == 6);
	CHECK(buffer[5] == 7);
	CHECK(buffer[6] == 8);
	CHECK(buffer[7] == 9);

	buffer.append_range(std::to_array<uint16_t>({ 10, 11, 12, 13, 14, 15 }));
	CHECK(buffer.size() == 8);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 8);
	CHECK(buffer[1] == 9);
	CHECK(buffer[2] == 10);
	CHECK(buffer[3] == 11);
	CHECK(buffer[4] == 12);
	CHECK(buffer[5] == 13);
	CHECK(buffer[6] == 14);
	CHECK(buffer[7] == 15);

	buffer.clear();
	CHECK(buffer.size() == 0);
	buffer.append_range(std::to_array<uint16_t>({ 16, 17, 18, 19, 20, 21 }));
	CHECK(buffer.size() == 6);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 16);
	CHECK(buffer[1] == 17);
	CHECK(buffer[2] == 18);
	CHECK(buffer[3] == 19);
	CHECK(buffer[4] == 20);
	CHECK(buffer[5] == 21);

	buffer.append_range(std::to_array<uint16_t>({ 22, 23, 24, 25, 26, 27 }), 6);
	CHECK(buffer.size() == 8);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 20);
	CHECK(buffer[1] == 21);
	CHECK(buffer[2] == 22);
	CHECK(buffer[3] == 23);
	CHECK(buffer[4] == 24);
	CHECK(buffer[5] == 25);
	CHECK(buffer[6] == 26);
	CHECK(buffer[7] == 27);

	buffer.push_back(28);
	CHECK(buffer.size() == 8);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 21);
	CHECK(buffer[1] == 22);
	CHECK(buffer[2] == 23);
	CHECK(buffer[3] == 24);
	CHECK(buffer[4] == 25);
	CHECK(buffer[5] == 26);
	CHECK(buffer[6] == 27);
	CHECK(buffer[7] == 28);

	buffer.erase(buffer.begin(), 3);
	CHECK(buffer.size() == 5);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 24);
	CHECK(buffer[1] == 25);
	CHECK(buffer[2] == 26);
	CHECK(buffer[3] == 27);
	CHECK(buffer[4] == 28);

	buffer.pop_back();
	CHECK(buffer.size() == 4);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 24);
	CHECK(buffer[1] == 25);
	CHECK(buffer[2] == 26);
	CHECK(buffer[3] == 27);

	buffer.shrink_to_fit();
	CHECK(buffer.size() == 4);
	CHECK(buffer.capacity() == 4);
	CHECK(buffer[0] == 24);
	CHECK(buffer[1] == 25);
	CHECK(buffer[2] == 26);
	CHECK(buffer[3] == 27);
	buffer.push_back(28);
	CHECK(buffer.size() == 4);
	CHECK(buffer.capacity() == 4);
	CHECK(buffer[0] == 25);
	CHECK(buffer[1] == 26);
	CHECK(buffer[2] == 27);
	CHECK(buffer[3] == 28);

	buffer.reserve(8);
	CHECK(buffer.size() == 4);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 25);
	CHECK(buffer[1] == 26);
	CHECK(buffer[2] == 27);
	CHECK(buffer[3] == 28);
	buffer.append_range(std::to_array<uint16_t>({ 29, 30, 31, 32 }));
	CHECK(buffer.size() == 8);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 25);
	CHECK(buffer[1] == 26);
	CHECK(buffer[2] == 27);
	CHECK(buffer[3] == 28);
	CHECK(buffer[4] == 29);
	CHECK(buffer[5] == 30);
	CHECK(buffer[6] == 31);
	CHECK(buffer[7] == 32);

	buffer.erase(buffer.end() - 1, buffer.begin() + 3);
	CHECK(buffer.size() == 4);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 28);
	CHECK(buffer[1] == 29);
	CHECK(buffer[2] == 30);
	CHECK(buffer[3] == 31);

	buffer.append_range(std::to_array<uint16_t>({ 32, 33, 34, 35 }));
	CHECK(buffer.size() == 8);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 28);
	CHECK(buffer[1] == 29);
	CHECK(buffer[2] == 30);
	CHECK(buffer[3] == 31);
	CHECK(buffer[4] == 32);
	CHECK(buffer[5] == 33);
	CHECK(buffer[6] == 34);
	CHECK(buffer[7] == 35);

	buffer.erase(buffer.end() - 1, buffer.end() + 3);
	CHECK(buffer.size() == 8);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 28);
	CHECK(buffer[1] == 29);
	CHECK(buffer[2] == 30);
	CHECK(buffer[3] == 31);
	CHECK(buffer[4] == 32);
	CHECK(buffer[5] == 33);
	CHECK(buffer[6] == 34);
	CHECK(buffer[7] == 35);

	std::vector new_buffer = buffer.read_buffer(4);
	CHECK(!new_buffer.empty());
	CHECK(new_buffer.size() == 4);
	CHECK(new_buffer[0] == 28);
	CHECK(new_buffer[1] == 29);
	CHECK(new_buffer[2] == 30);
	CHECK(new_buffer[3] == 31);

	CHECK(buffer.size() == 4);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 32);
	CHECK(buffer[1] == 33);
	CHECK(buffer[2] == 34);
	CHECK(buffer[3] == 35);

	uint16_t read = buffer.read();
	CHECK(read == 32);

	CHECK(buffer.size() == 3);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 33);
	CHECK(buffer[1] == 34);
	CHECK(buffer[2] == 35);

	buffer.resize(6, 5);
	CHECK(buffer.size() == 6);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 33);
	CHECK(buffer[1] == 34);
	CHECK(buffer[2] == 35);
	CHECK(buffer[3] == 5);
	CHECK(buffer[4] == 5);
	CHECK(buffer[5] == 5);

	buffer.resize(1);
	CHECK(buffer.size() == 1);
	CHECK(buffer.capacity() == 8);
	CHECK(buffer[0] == 33);

	buffer.resize(10, 3);
	CHECK(buffer.size() == 10);
	CHECK(buffer.capacity() == 10);
	CHECK(buffer[0] == 33);
	CHECK(buffer[1] == 3);
	CHECK(buffer[2] == 3);
	CHECK(buffer[3] == 3);
	CHECK(buffer[4] == 3);
	CHECK(buffer[5] == 3);
	CHECK(buffer[6] == 3);
	CHECK(buffer[7] == 3);
	CHECK(buffer[8] == 3);
	CHECK(buffer[9] == 3);

	buffer.push_back(10);
	CHECK(buffer.size() == 10);
	CHECK(buffer.capacity() == 10);
	CHECK(buffer[0] == 3);
	CHECK(buffer[1] == 3);
	CHECK(buffer[2] == 3);
	CHECK(buffer[3] == 3);
	CHECK(buffer[4] == 3);
	CHECK(buffer[5] == 3);
	CHECK(buffer[6] == 3);
	CHECK(buffer[7] == 3);
	CHECK(buffer[8] == 3);
	CHECK(buffer[9] == 10);
}
