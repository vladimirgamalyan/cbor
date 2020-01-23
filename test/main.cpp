#include <fstream>
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
