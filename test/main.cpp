/*
	This test suite contains some parts of https://bitbucket.org/isode/cbor-lite/src/master/unit/codec.cpp.
	cbor-lite is a MIT licensed software and requires to include next copyright notice:

	Copyright 2018-2019 Isode Limited.
	All rights reserved.

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/

#include <fstream>
#include <sstream>
#include "../cbor_encoder_ostream.h"
#include "../cbor_decoder_istream.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace doctest
{
	template <typename uint8_t>
	struct StringMaker<std::vector<uint8_t> >
	{
		static String convert(const std::vector<uint8_t>& in)
		{
			std::ostringstream oss;
			oss << "[" << std::setfill('0') << std::hex;
			for (std::size_t i = 0; i < in.size(); ++i)
			{
				if (i)
					oss << ", ";
				oss << "0x" << std::setw(2) << static_cast<int>(in[i]);
			}
			oss << "]";
			return oss.str().c_str();
		}
	};
}

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
	return static_cast<size_t>(oss.tellp());
}

TEST_CASE("integer length")
{
	REQUIRE(packed_integer_len(0) == 1);
	REQUIRE(packed_integer_len(1) == 1);
	REQUIRE(packed_integer_len(23) == 1);
	REQUIRE(packed_integer_len(24) == 2);
	REQUIRE(packed_integer_len(255) == 2);
	REQUIRE(packed_integer_len(256) == 3);
	REQUIRE(packed_integer_len(65535) == 3);
	REQUIRE(packed_integer_len(65536) == 5);
	REQUIRE(packed_integer_len(4294967295) == 5);
	REQUIRE(packed_integer_len(4294967296) == 9);
}

std::vector<uint8_t> packed_unsigned_integer(uint64_t n)
{
	std::ostringstream oss;
	cbor_encoder_ostream encoder(oss);
	encoder.write_uint(n);
	std::string str = oss.str();
	std::vector<uint8_t> result(str.begin(), str.end());
	return result;
}

std::vector<uint8_t> packed_integer(int64_t n)
{
	std::ostringstream oss;
	cbor_encoder_ostream encoder(oss);
	encoder.write_int(n);
	std::string str = oss.str();
	std::vector<uint8_t> result(str.begin(), str.end());
	return result;
}

TEST_CASE("unsigned integer")
{
	REQUIRE(packed_unsigned_integer(0u) == std::vector<uint8_t>{ 0x00 });
	REQUIRE(packed_unsigned_integer(1u) == std::vector<uint8_t>{ 0x01 });
	REQUIRE(packed_unsigned_integer(10u) == std::vector<uint8_t>{ 0x0a });
	REQUIRE(packed_unsigned_integer(23u) == std::vector<uint8_t>{ 0x17 });
	REQUIRE(packed_unsigned_integer(24u) == std::vector<uint8_t>{ 0x18, 0x18 });
	REQUIRE(packed_unsigned_integer(25u) == std::vector<uint8_t>{ 0x18, 0x19 });
	REQUIRE(packed_unsigned_integer(100u) == std::vector<uint8_t>{ 0x18, 0x64 });
	REQUIRE(packed_unsigned_integer(1000u) == std::vector<uint8_t>{ 0x19, 0x03, 0xe8 });
	REQUIRE(packed_unsigned_integer(1000000u) == std::vector<uint8_t>{ 0x1a, 0x00, 0x0f, 0x42, 0x40 });
	REQUIRE(packed_unsigned_integer(1000000000000u) == std::vector<uint8_t>{ 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 });
	REQUIRE(packed_unsigned_integer(18446744073709551615u) == std::vector<uint8_t>{ 0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff });
}

TEST_CASE("integer")
{
	REQUIRE(packed_integer(0) == std::vector<uint8_t>{ 0x00 });
	REQUIRE(packed_integer(1) == std::vector<uint8_t>{ 0x01 });
	REQUIRE(packed_integer(10) == std::vector<uint8_t>{ 0x0a });
	REQUIRE(packed_integer(23) == std::vector<uint8_t>{ 0x17 });
	REQUIRE(packed_integer(24) == std::vector<uint8_t>{ 0x18, 0x18 });
	REQUIRE(packed_integer(25) == std::vector<uint8_t>{ 0x18, 0x19 });
	REQUIRE(packed_integer(100) == std::vector<uint8_t>{ 0x18, 0x64 });
	REQUIRE(packed_integer(1000) == std::vector<uint8_t>{ 0x19, 0x03, 0xe8 });
	REQUIRE(packed_integer(1000000) == std::vector<uint8_t>{ 0x1a, 0x00, 0x0f, 0x42, 0x40 });
	REQUIRE(packed_integer(1000000000000) == std::vector<uint8_t>{ 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 });
	REQUIRE(packed_integer(-1) == std::vector<uint8_t>{ 0x20 });
	REQUIRE(packed_integer(-10) == std::vector<uint8_t>{ 0x29 });
	REQUIRE(packed_integer(-100) == std::vector<uint8_t>{ 0x38, 0x63 });
	REQUIRE(packed_integer(-1000) == std::vector<uint8_t>{ 0x39, 0x03, 0xe7 });
}
