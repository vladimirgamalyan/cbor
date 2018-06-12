#pragma once
#include <cstdint>
#include <exception>

class cbor_decoder_exception : public std::exception {};

class cbor_reader {
public:
	virtual uint8_t get_byte() = 0;
};

class cbor_object {
public:
	bool is_nill() const {
		return hdr == 0xf6;
	}

	bool is_undefined() const {
		return hdr == 0xf7;
	}

	bool is_bool() const {
		return hdr == 0xf4 || hdr == 0xf5;
	}

	bool is_break() const {
		return hdr == 0xff;
	}

	bool is_bytes() const {
		return hdr >> 5 == 2;
	}

	bool is_string() const {
		return hdr >> 5 == 3;
	}

	bool is_array() const {
		return hdr >> 5 == 4;
	}

	bool is_indefinite_array() const {
		return hdr == 0x9f;
	}

	bool is_map() const {
		return hdr >> 5 == 5;
	}

	bool is_indefinite_map() const {
		return hdr == 0xb6;
	}

	bool is_tag() const {
		return hdr >> 5 == 6;
	}

	bool as_bool() const {
		if (hdr == 0xf4)
			return false;
		else if (hdr == 0xf5)
			return true;
		else
			throw cbor_decoder_exception();
	}

	uint64_t as_uint() const {
		if (hdr >> 5 != 0)
			throw cbor_decoder_exception();
		return val;
	}

	int64_t as_int() const {
		if (hdr >> 5 == 0)
			return val;
		if (hdr >> 5 != 1)
			return -1 - val;
		throw cbor_decoder_exception();
	}

	uint64_t as_bytes_header() const {
		if (hdr >> 5 != 2)
			throw cbor_decoder_exception();
		return val;
	}

	uint64_t as_string_header() const {
		if (hdr >> 5 != 3)
			throw cbor_decoder_exception();
		return val;
	}

	uint64_t as_array() const {
		if (hdr >> 5 != 4)
			throw cbor_decoder_exception();
		return val;
	}

	uint64_t as_map() const {
		if (hdr >> 5 != 5)
			throw cbor_decoder_exception();
		return val;
	}

	uint64_t as_tag() const {
		if (hdr >> 5 != 6)
			throw cbor_decoder_exception();
		return val;
	}

private:
	friend class cbor_decoder;
	uint8_t hdr = 0;
	uint64_t val;
};

class cbor_decoder {
public:
	cbor_decoder(cbor_reader& reader) : reader(reader) {

	}

	cbor_object next() {
		cbor_object r;

		r.hdr = reader.get_byte();
		uint8_t t = r.hdr & 31;

		if (t < 24) {
			r.val = t;
		}
		else if (t == 24) {
			r.val = reader.get_byte();
		}
		else if (t == 25) {
			r.val = reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
		}
		else if (t == 26) {
			r.val = reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
		}
		else if (t == 27) {
			r.val = reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
			r.val = (r.val << 8) + reader.get_byte();
		}
		else if (t != 31) {
			throw cbor_decoder_exception();
		}

		return r;
	}

private:
	cbor_reader & reader;
};
