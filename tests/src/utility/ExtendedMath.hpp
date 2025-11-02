#pragma once

#include <cstdint>
#include <limits>

namespace OpenVic::testing {
	/** @brief Structure to hold a 96-bit signed integer product.
	* The product of a signed 64-bit number and an unsigned 32-bit number
	* requires up to 96 bits (2^63 * 2^32 = 2^95).
	*
	* The result is stored as a magnitude across two 64-bit words, plus a sign flag.
	* The mathematical value is: (-1)^is_negative * (high * 2^64 + low)
	*/
	struct Int96Product {
		uint64_t low;           ///< The lower 64 bits of the product's magnitude.
		uint64_t high;          ///< The upper 32 bits (stored in a 64-bit word) of the product's magnitude.
		bool is_negative;       ///< True if the overall product is negative.
	};

	/** @brief Multiplies an int64_t by a uint32_t, returning the exact 96-bit result.
	* This function performs the multiplication using standard C++ 64-bit arithmetic,
	* preventing intermediate overflow and storing the full result in a 96-bit structure.
	*
	* @param a The signed 64-bit integer factor (int64_t).
	* @param b The unsigned 32-bit integer factor (uint32_t).
	* @return An Int96Product structure containing the full 96-bit signed result.
	*/
	[[nodiscard]] Int96Product portable_int64_mult_uint32_96bit(int64_t a, uint32_t b) {
		Int96Product result = {0, 0, false};

		// 1. Handle zero factors
		if (a == 0 || b == 0) {
			return result;
		}

		// 2. Determine Sign and Magnitude
		result.is_negative = (a < 0);
		
		// Get the magnitude of 'a' as an unsigned 64-bit number.
		// Note: If a == INT64_MIN, -a is undefined in signed arithmetic.
		// Using (uint64_t)a converts it to 2's complement representation, and 
		// applying the negative sign to the result of a * 1 (for INT64_MIN) yields the correct magnitude.
		uint64_t a_mag;
		if (a == std::numeric_limits<int64_t>::min()) {
			// Absolute value of INT64_MIN is 2^63, which requires a custom calculation 
			// to avoid signed overflow/UB when finding the magnitude.
			// INT64_MIN is 0x8000000000000000. Its magnitude is 0x8000000000000000.
			// The cast to uint64_t yields this magnitude directly.
			a_mag = (uint64_t)std::numeric_limits<int64_t>::min();
		} else {
			a_mag = (a < 0) ? (uint64_t)-a : (uint64_t)a;
		}
		
		uint64_t b_mag = b; // b is already unsigned and positive

		// 3. Perform 64x32 -> 96-bit multiplication using split-word technique
		
		// Masks and constants
		const uint64_t MASK_32 = 0xFFFFFFFFULL;

		// Split a_mag into 32-bit parts: a_mag = a_H * 2^32 + a_L
		uint64_t a_L = a_mag & MASK_32;
		uint64_t a_H = a_mag >> 32;

		// Partial product P_L = a_L * b_mag (32x32 -> 64 bits)
		uint64_t P_L = a_L * b_mag;

		// Partial product P_H = a_H * b_mag (32x32 -> 64 bits)
		uint64_t P_H = a_H * b_mag;

		// 4. Combine Products

		// The low word of the 96-bit result is simply P_L
		result.low = P_L;

		// The high word (P_H * 2^32) contribution needs to be added to the carry from P_L.
		// P_L_carry is the upper 32 bits of P_L, which carries into the high word.
		uint64_t P_L_carry = P_L >> 32;

		// The Middle Word (bits 32-95) is P_H + P_L_carry
		// This is the total coefficient for 2^32.
		uint64_t middle_word = P_H + P_L_carry;

		// The upper half of the 96-bit result (result.high, bits 64-95) 
		// is the upper 32 bits of the middle_word.
		// This is the correct coefficient for 2^64.
		result.high = middle_word >> 32; 
		
		// Note: Since a_H * b_mag is max (2^32-1) * (2^32-1) < 2^64, and P_L_carry < 2^32,
		// the sum result.high is safely stored in a uint64_t. The highest bit (2^95) 
		// of the product will be contained in the result.high word.

		return result;
	}

	/**
	* @brief Structure to hold the result of the division operation.
	*/
	struct Int96DivisionResult {
		int64_t quotient;         ///< The quotient (clamped to INT64_MAX/MIN if overflow occurs).
		int64_t remainder;        ///< The remainder, with the same sign as the dividend.
		bool quotient_overflow;   ///< True if the mathematical quotient exceeds int64_t limits.
	};

	/**
	* @brief Divides a 96-bit signed integer (Int96Product) by an int64_t, checking for quotient overflow.
	* * This is implemented using a portable iterative long division algorithm.
	*
	* @param dividend The 96-bit signed number.
	* @param divisor The signed 64-bit divisor.
	* @return An IntDivisionResult containing the quotient, remainder, and overflow status.
	*/
	Int96DivisionResult portable_int96_div_int64(const Int96Product& dividend, int64_t divisor) {
		Int96DivisionResult result = {0, 0, false};

		// 1. Handle Divisor Zero
		if (divisor == 0) {
			result.quotient_overflow = true;
			result.quotient = std::numeric_limits<int64_t>::max(); // Return clamped value for safety
			result.remainder = 0;
			return result;
		}

		// 2. Handle Dividend Zero
		if (dividend.low == 0 && dividend.high == 0) {
			return result;
		}

		// 3. Determine Signs and Magnitudes
		const bool result_negative = (dividend.is_negative != (divisor < 0));
		
		uint64_t d_mag;
		if (divisor == std::numeric_limits<int64_t>::min()) {
			// INT64_MIN magnitude is 2^63 (0x8000000000000000ULL)
			d_mag = 1ULL << 63; 
		} else {
			// For other negative numbers, -divisor is safe. For positive, divisor is fine.
			d_mag = (divisor < 0) ? (uint64_t)-divisor : (uint64_t)divisor;
		}

		// 4. Perform 96-bit / 64-bit Division on Magnitudes (Iterative Long Division)
		
		uint64_t quotient_mag = 0;
		uint64_t remainder_mag = 0; // Remainder is 64-bit and accumulates dividend bits

		// Iterate through all 96 bits of the dividend magnitude (from bit 95 down to 0)
		for (int i = 95; i >= 0; --i) {
			remainder_mag <<= 1;

			// Bring in the next dividend bit
			if (i >= 64) {
				// Bit is in dividend.high at position i-64 (Max 32 bits)
				if ((dividend.high >> (i - 64)) & 1) {
					remainder_mag |= 1;
				}
			} else {
				// Bit is in dividend.low at position i (64 bits)
				if ((dividend.low >> i) & 1) {
					remainder_mag |= 1;
				}
			}

			// Try to subtract divisor magnitude from the current remainder
			if (remainder_mag >= d_mag) {
				remainder_mag -= d_mag;
				
				if (i >= 64) {
					// If a quotient bit is set at or above index 64, the mathematical quotient
					// is >= 2^64, which is an overflow for int64_t.
					result.quotient_overflow = true;
					// We must continue the loop to calculate the correct final remainder.
				} else {
					// Store the quotient bit $0 \le i \le 63$.
					quotient_mag |= (1ULL << i);
				}
			}
		}
		
		// 5. Final Overflow Check and Sign Application
		const uint64_t INT64_MAX_MAG = ~(1ULL << 63);
		// The magnitude of INT64_MIN is 2^63, which is 0x8000000000000000ULL
		const uint64_t INT64_MIN_MAG = 1ULL << 63;

		if (result_negative) {
			// If overflow was detected earlier OR if the 64-bit quotient magnitude exceeds 2^63 (INT64_MIN magnitude)
			if (result.quotient_overflow || (quotient_mag > INT64_MIN_MAG)) {
				result.quotient_overflow = true;
				result.quotient = std::numeric_limits<int64_t>::min();
			} else {
				// Check for the exact INT64_MIN case (magnitude 2^63)
				if (quotient_mag == INT64_MIN_MAG) {
					result.quotient = std::numeric_limits<int64_t>::min();
				} else {
					result.quotient = -(int64_t)quotient_mag;
				}
			}
		} else { // Positive result
			if (result.quotient_overflow || (quotient_mag > INT64_MAX_MAG)) {
				result.quotient_overflow = true;
				result.quotient = std::numeric_limits<int64_t>::max();
			} else {
				result.quotient = (int64_t)quotient_mag;
			}
		}
		
		// Apply sign to remainder (must have the same sign as the dividend).
		if (dividend.is_negative && remainder_mag != 0) {
			result.remainder = -(int64_t)remainder_mag;
		} else {
			result.remainder = (int64_t)remainder_mag;
		}
		
		return result;
	}
}