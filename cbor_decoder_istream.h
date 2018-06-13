#pragma once
#include <istream>
#include "cbor_decoder.h"

class cbor_decoder_istream : public cbor_decoder
{
public:
	cbor_decoder_istream(std::istream& s) : s(s) {}

	std::string read_string_body(const cbor_object& o) {
		uint64_t size = o.as_string_header();
		std::string r((size_t)size, ' ');
		s.read(&r[0], (std::streamsize)size);
		return r;
	}

	std::string read_string() {
		return read_string_body(read());
	}

	std::vector<uint8_t> read_bytes_body(const cbor_object& o) {
		uint64_t size = o.as_bytes_header();
		std::vector<uint8_t> r((size_t)size);
		s.read((char*)&r[0], (std::streamsize)size);
		return r;
	}

	std::vector<uint8_t> read_bytes() {
		return read_bytes_body(read());
	}

private:
	std::istream& s;
	virtual uint8_t get_byte() override {
		return (uint8_t)s.get();
	}
};
