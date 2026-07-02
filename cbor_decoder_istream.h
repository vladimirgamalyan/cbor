#pragma once

#include <istream>
#include <string>
#include <vector>
#include "cbor_decoder.h"

class cbor_decoder_istream : public cbor_decoder {
public:
	explicit cbor_decoder_istream(std::istream& s) : s(s) {}

	std::string read_string_body(const cbor_object& o) {
		const uint64_t size = o.as_string_header();
		check_size(size);
		std::string r((size_t)size, '\0');
		if (size)
			read_payload(&r[0], size);
		return r;
	}

	std::string read_string() { return read_string_body(read()); }

	std::vector<uint8_t> read_bytes_body(const cbor_object& o) {
		const uint64_t size = o.as_bytes_header();
		check_size(size);
		std::vector<uint8_t> r((size_t)size);
		if (size)
			read_payload((char*)r.data(), size);
		return r;
	}

	std::vector<uint8_t> read_bytes() { return read_bytes_body(read()); }

private:
	std::istream& s;

	static void check_size(uint64_t size) {
		if ((uint64_t)(size_t)size != size) // does not fit size_t (32 bit platforms)
			throw cbor_decoder_exception("CBOR: length does not fit size_t");
	}

	void read_payload(char* dst, uint64_t size) {
		s.read(dst, (std::streamsize)size);
		if (s.gcount() != (std::streamsize)size)
			throw cbor_decoder_exception("CBOR: unexpected end of input");
	}

	uint8_t get_byte() override {
		const int c = s.get();
		if (c == std::char_traits<char>::eof())
			throw cbor_decoder_exception("CBOR: unexpected end of input");
		return (uint8_t)c;
	}
};
