#pragma once
#include <cstdint>
#include <exception>

class cbor_exception : public std::exception {};

class cbor_reader {
public:
	virtual uint8_t get_byte() = 0;
};

class cbor_decoder {
public:
	cbor_decoder(cbor_reader& reader) : reader(reader) {

	}

	void read_null() {
		read_byte(0xf6);
	}

	void read_undefined() {
		read_byte(0xf7);
	}

	bool read_bool() {
		uint8_t type = reader.get_byte();
		if (type == 0xf4)
			return false;
		else if (type == 0xf5)
			return true;
		else
			throw cbor_exception();
	}

	void read_break() {
		read_byte(0xff);
	}

	uint64_t read_uint() {
		return read_object(0);
	}

	int64_t read_int() {
		uint8_t type;
		uint64_t r = read_any_object(type);
		if (type == 0)
			return r;
		if (type == 1)
			return -1 - r;
		throw cbor_exception();
	}

	uint64_t read_bytes_header() {
		return read_object(2);
	}

	uint64_t read_string_header() {
		return read_object(3);
	}

	uint64_t read_array() {
		return read_object(4);
	}

	void read_indefinite_array() {
		read_byte(0x9f);
	}

	uint64_t read_map() {
		return read_object(5);
	}

	void read_indefinite_map() {
		read_byte(0xbf);
	}

	uint64_t read_tag() {
		return read_object(6);
	}

protected:
	cbor_reader & reader;

	void read_byte(uint8_t b) {
		uint8_t r = reader.get_byte();
		if (r != b)
			throw cbor_exception();
	}

	uint64_t read_object(uint8_t required_type) {
		uint8_t type;
		uint64_t r = read_any_object(type);
		if (required_type != type)
			throw cbor_exception();
		return r;
	}

	uint64_t read_any_object(uint8_t& major_type) {
		uint8_t type = reader.get_byte();
		uint8_t additional_info = type & 31;
		major_type = type >> 5;

		if (additional_info < 24)
			return additional_info;
		if (additional_info == 24)
			return reader.get_byte();
		if (additional_info == 25)
		{
			uint64_t r = reader.get_byte();
			return (r << 8) + reader.get_byte();
		}
		if (additional_info == 26)
		{
			uint64_t r = reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			return r;
		}
		if (additional_info == 27)
		{
			uint64_t r = reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			r = (r << 8) + reader.get_byte();
			return r;
		}
		throw cbor_exception();
	}
};
