#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <limits>

class cbor_encoder {
public:
	void write_null() { put_byte(0xf6); }

	void write_undefined() { put_byte(0xf7); }

	void write_bool(bool value) { put_byte(value ? 0xf5 : 0xf4); }

	void write_break() { put_byte(0xff); }

	void write_uint(uint64_t value) { write_type_and_value(0, value); }

	void write_int(int64_t value) {
		if (value < 0)
			write_type_and_value(1, -(value + 1));
		else
			write_type_and_value(0, value);
	}

	void write_bytes_header(uint64_t size) { write_type_and_value(2, size); }

	void write_string_header(uint64_t size) { write_type_and_value(3, size); }

	void write_array(uint64_t size) { write_type_and_value(4, size); }

	void write_indefinite_array() { put_byte(0x9f); }

	void write_map(uint64_t size) { write_type_and_value(5, size); }

	void write_indefinite_map() { put_byte(0xbf); }

	void write_tag(uint64_t tag) { write_type_and_value(6, tag); }

	// Canonical (RFC 8949 preferred serialization): writes the value using the
	// shortest of the half/single/double encodings that preserves it exactly.
	// Any standard-conforming decoder reads the result back to the same value.
	void write_float_shortest(double value) {
		if (std::isnan(value)) {
			// The preferred encoding of NaN is the half-precision 0xf97e00.
			put_byte(0xf9);
			write_big_endian(0x7e00, 2);
			return;
		}

		float single = (float)value;
		if ((double)single != value) { // does not fit a single without changing value
			write_double(value);
			return;
		}

		uint16_t half = float_to_half(single);
		if ((double)half_to_float(half) == value)
			write_half_float(single);
		else
			write_float(single);
	}

	// Fixed width encoders (useful for interop tests or when a specific width is
	// required); the output is standard CBOR that any decoder accepts.
	void write_half_float(float value) {
		put_byte(0xf9);
		write_big_endian(float_to_half(value), 2);
	}

	void write_float(float value) {
		uint32_t bits;
		std::memcpy(&bits, &value, sizeof(bits));
		put_byte(0xfa);
		write_big_endian(bits, 4);
	}

	void write_double(double value) {
		uint64_t bits;
		std::memcpy(&bits, &value, sizeof(bits));
		put_byte(0xfb);
		write_big_endian(bits, 8);
	}

protected:
	virtual void put_byte(uint8_t b) = 0;

	void write_big_endian(uint64_t value, unsigned bytes) {
		for (unsigned i = bytes; i-- > 0;)
			put_byte((uint8_t)(value >> (i * 8u)));
	}

	static uint16_t float_to_half(float value) {
		uint32_t x;
		std::memcpy(&x, &value, sizeof(x));
		uint16_t sign = (uint16_t)((x >> 16u) & 0x8000u);
		uint32_t mantissa = x & 0x007fffffu;
		uint32_t biased_exp = (x >> 23u) & 0xffu;

		if (biased_exp == 0xffu) // Inf or NaN
			return (uint16_t)(sign | 0x7c00u | (mantissa ? 0x0200u : 0u));

		int32_t exp = (int32_t)biased_exp - 127 + 15;

		if (exp >= 31) // overflow -> Inf
			return (uint16_t)(sign | 0x7c00u);

		if (exp <= 0) { // subnormal or zero
			if (exp < -10)
				return sign;
			mantissa |= 0x00800000u;
			unsigned shift = (unsigned)(14 - exp);
			uint32_t half_mant = mantissa >> shift;
			uint32_t round_bit = 1u << (shift - 1);
			if ((mantissa & round_bit) && ((mantissa & (round_bit - 1)) || (half_mant & 1)))
				half_mant += 1;
			return (uint16_t)(sign | half_mant);
		}

		uint16_t result = (uint16_t)(sign | ((uint16_t)exp << 10u) | (mantissa >> 13u));
		uint32_t round_bit = 1u << 12;
		if ((mantissa & round_bit) && ((mantissa & (round_bit - 1)) || (result & 1)))
			result += 1; // carry into exponent is intentional and correct
		return result;
	}

	static float half_to_float(uint16_t half) {
		uint16_t exp = (half >> 10u) & 0x1fu;
		uint16_t mant = half & 0x3ffu;
		float value;
		if (exp == 0)
			value = std::ldexp((float)mant, -24);
		else if (exp != 31)
			value = std::ldexp((float)(mant + 1024), exp - 25);
		else
			value = mant == 0 ? std::numeric_limits<float>::infinity()
			              : std::numeric_limits<float>::quiet_NaN();
		return (half & 0x8000u) ? -value : value;
	}

	void write_type_and_value(uint8_t major_type, uint64_t value) {
		major_type <<= 5u;
		if (value < 24) {
			put_byte((uint8_t)(major_type | value));
		}
		else if (value < 256) {
			put_byte((uint8_t)(major_type | 24u));
			put_byte((uint8_t)value);
		}
		else if (value < 65536) {
			put_byte((uint8_t)(major_type | 25u));
			put_byte((uint8_t)(value >> 8u));
			put_byte((uint8_t)value);
		}
		else if (value < 4294967296ULL) {
			put_byte((uint8_t)(major_type | 26u));
			put_byte((uint8_t)(value >> 24u));
			put_byte((uint8_t)(value >> 16u));
			put_byte((uint8_t)(value >> 8u));
			put_byte((uint8_t)value);
		}
		else {
			put_byte((uint8_t)(major_type | 27u));
			put_byte((uint8_t)(value >> 56u));
			put_byte((uint8_t)(value >> 48u));
			put_byte((uint8_t)(value >> 40u));
			put_byte((uint8_t)(value >> 32u));
			put_byte((uint8_t)(value >> 24u));
			put_byte((uint8_t)(value >> 16u));
			put_byte((uint8_t)(value >> 8u));
			put_byte((uint8_t)(value));
		}
	}
};
