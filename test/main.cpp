#include <fstream>
#include <sstream>
#include "../cbor_encoder_ostream.h"
#include "../cbor_decoder_istream.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("simple file read/write")
{
	{
		std::ofstream f("test.bin", std::fstream::binary);
		f.exceptions(std::fstream::failbit | std::fstream::badbit);
		cbor_encoder_ostream encoder(f);

		encoder.write_array(2);
		encoder.write_string("Hello, World!");
		encoder.write_int(42);
	}
	
	{
		std::ifstream f("test.bin", std::fstream::binary);
		f.exceptions(std::fstream::failbit | std::fstream::badbit);
		cbor_decoder_istream decoder(f);

		uint64_t l = decoder.read().as_array();
		REQUIRE(l == 2);
		std::string s = decoder.read_string();
		REQUIRE(s == "Hello, World!");
		int64_t  i = decoder.read().as_int();
		REQUIRE(i == 42);
	}
}

size_t packed_integer_len(int64_t n)
{
	std::ostringstream oss;
	cbor_encoder_ostream encoder(oss);
	encoder.write_int(n);
	oss.seekp(0, std::ios::end);
	return oss.tellg();
}

TEST_CASE("number length")
{
	REQUIRE(packed_integer_len(0) == 1);
	REQUIRE(packed_integer_len(1) == 1);
	REQUIRE(packed_integer_len(23) == 1);
	REQUIRE(packed_integer_len(24) == 2);
	REQUIRE(packed_integer_len(255) == 2);
	REQUIRE(packed_integer_len(256) == 3);
	REQUIRE(packed_integer_len(65535) == 3);
	REQUIRE(packed_integer_len(65536) == 4);
	REQUIRE(packed_integer_len(4294967295) == 4);
	REQUIRE(packed_integer_len(4294967296) == 5);
}
