#pragma once
#include <cstdint>

class cbor_writer {
public:
	virtual void put_bytes(const uint8_t *data, uint64_t size) = 0;
};

class cbor_encoder {
public:
	cbor_encoder(cbor_writer& writer) : writer(writer) {

	}

	void write_null() {
		put_byte(0xf6);
	}

	void write_undefined() {
		put_byte(0xf7);
	}

	void write_bool(bool value) {
		if (value)
			put_byte(0xf5);
		else
			put_byte(0xf4);
	}

	void write_break() {
		put_byte(0xff);
	}

	void write_uint(const uint64_t value) {
		write_type_and_value(0, value);
	}

	void write_int(const int64_t value) {
		if (value < 0)
			write_type_and_value(1, -(value + 1));
		else
			write_type_and_value(0, value);
	}

	void write_bytes(const uint8_t* data, const uint64_t size) {
		write_type_and_value(2, size);
		writer.put_bytes(data, size);
	}

	void write_bytes_header(const uint8_t* data, const uint64_t size) {
		write_type_and_value(2, size);
	}

	void write_string(const char* data, const uint64_t size) {
		write_type_and_value(3, size);
		writer.put_bytes((const uint8_t*)data, size);
	}

	void write_string_header(const char* data, const uint64_t size) {
		write_type_and_value(3, size);
	}
	
	void write_array(const uint64_t size) {
		write_type_and_value(4, size);
	}

	void write_map(const uint64_t size) {
		write_type_and_value(5, size);
	}

	void write_tag(const uint64_t tag) {
		write_type_and_value(6, tag);
	}

private:
	cbor_writer& writer;

	void put_byte(uint64_t b) {
		uint8_t i = (uint8_t)i;
		writer.put_bytes(&i, 1);
	}

	void write_type_and_value(uint8_t major_type, uint64_t value) {
		major_type <<= 5;
		if (value < 24) {
			put_byte(major_type | value);
		} else if (value < 256) {
			put_byte(major_type | 24);
			put_byte(value);
		} else if (value < 65536) {
			put_byte(major_type | 25);
			put_byte(value >> 8);
			put_byte(value);
		}
		else if (value < 4294967296ULL) {
			put_byte(major_type | 26);
			put_byte(value >> 24);
			put_byte(value >> 16);
			put_byte(value >> 8);
			put_byte(value);
		}
		else {
			put_byte(major_type | 27);
			put_byte(value >> 56);
			put_byte(value >> 48);
			put_byte(value >> 40);
			put_byte(value >> 32);
			put_byte(value >> 24);
			put_byte(value >> 16);
			put_byte(value >> 8);
			put_byte(value);
		}
	}
};
