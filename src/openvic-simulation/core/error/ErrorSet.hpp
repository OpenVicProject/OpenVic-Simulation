#pragma once

#include <bitset>
#include <cstddef>

#include "openvic-simulation/core/error/Error.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	// Set of all reported errors, if empty operates as if set to Error::OK
	struct ErrorSet : private std::bitset<static_cast<std::size_t>(Error::MAX) - 1> {
		ErrorSet() = default;
		ErrorSet(ErrorSet const&) = default;
		ErrorSet(ErrorSet&&) = default;
		ErrorSet& operator=(ErrorSet const&) = default;
		ErrorSet& operator=(ErrorSet&&) = default;

		ErrorSet(Error error) {
			if (error != Error::OK) {
				set(static_cast<std::size_t>(error) - 1, true);
			}
		}

		ErrorSet& operator=(Error error) {
			reset();
			if (error != Error::OK) {
				set(static_cast<std::size_t>(error) - 1, true);
			}
			return *this;
		}

		using bitset::all;
		using bitset::any;
		using bitset::count;
		using bitset::none;
		using bitset::size;

		bool operator[](Error error) const {
			if (error == Error::OK) {
				return none();
			}

			return bitset::operator[](static_cast<std::size_t>(error) - 1);
		}

		bool operator[](ErrorSet const& errors) const {
			if (errors.none()) {
				return none();
			}

			return (*this & errors) == errors;
		}

		bool operator==(ErrorSet const& rhs) const {
			return bitset::operator==(rhs);
		}

		bool operator==(Error rhs) const {
			return operator[](rhs);
		}

		bool test(Error error) const {
			return operator[](error);
		}

		bool test(ErrorSet const& errors) const {
			return operator[](errors);
		}

		ErrorSet& operator|=(ErrorSet const& rhs) {
			bitset::operator|=(rhs);
			return *this;
		}

		ErrorSet operator|=(Error rhs) {
			if (rhs != Error::OK) {
				set(static_cast<std::size_t>(rhs) - 1, true);
			}
			return *this;
		}

		friend inline ErrorSet operator|(ErrorSet const& lhs, ErrorSet const& rhs) {
			ErrorSet result { lhs };
			result |= rhs;
			return result;
		}

		friend inline ErrorSet operator|(ErrorSet const& lhs, Error rhs) {
			ErrorSet result { lhs };
			result |= rhs;
			return result;
		}

		friend inline ErrorSet operator|(Error lhs, ErrorSet const& rhs) {
			ErrorSet result { rhs };
			result |= lhs;
			return result;
		}

		memory::string to_bitset_string(memory::string::value_type zero = '0', memory::string::value_type one = '1') const {
			return bitset::to_string<memory::string::value_type, memory::string::traits_type, memory::string::allocator_type>(
				zero, one
			);
		}

		memory::string to_string() const {
			memory::string result;
			if (none()) {
				result.append(as_string(Error::OK));
				return result;
			}

			{
				std::size_t str_size = 0;
				for (std::size_t i = 1; i <= size(); i++) {
					if (!(*this).bitset::operator[](i - 1)) {
						continue;
					}

					if (str_size > 0) {
						str_size += 2; // for ", "
					}

					str_size += as_string(static_cast<Error>(i)).size();
				}

				result.reserve(str_size);
			}

			for (std::size_t i = 1; i <= size(); i++) {
				if (!(*this).bitset::operator[](i - 1)) {
					continue;
				}

				if (result.size() > 0) {
					result.append(", ");
				}

				result.append(as_string(static_cast<Error>(i)));
			}
			return result;
		}

	private:
		using bitset::bitset;
	};
}
