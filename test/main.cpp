/*
	This test suite contains some parts of https://bitbucket.org/isode/cbor-lite/src/master/unit/codec.cpp
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
#include "json.hpp"

typedef std::vector<uint8_t> vec;

class cbor : public cbor_encoder_ostream
{
public:
	cbor() : cbor_encoder_ostream(oss) {}

	void chk(const vec& data)
	{
		REQUIRE(get_data() == data);
	}

	static void chk_uint(uint64_t n, const vec& data)
	{
		cbor enc;
		enc.write_uint(n);
		enc.chk(data);
	}

	static void chk_int(int64_t n, const vec& data)
	{
		cbor enc;
		enc.write_int(n);
		enc.chk(data);
	}

private:
	std::ostringstream oss;

	vec get_data() const
	{
		std::string str = oss.str();
		return { str.begin(), str.end() };
	}
};

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

TEST_CASE("unsigned integer")
{
	cbor::chk_uint(0u, { 0x00 });
	cbor::chk_uint(1u, { 0x01 });
	cbor::chk_uint(10u, { 0x0a });
	cbor::chk_uint(23u, { 0x17 });
	cbor::chk_uint(24u, { 0x18, 0x18 });
	cbor::chk_uint(25u, { 0x18, 0x19 });
	cbor::chk_uint(100u, { 0x18, 0x64 });
	cbor::chk_uint(1000u, { 0x19, 0x03, 0xe8 });
	cbor::chk_uint(1000000u, { 0x1a, 0x00, 0x0f, 0x42, 0x40 });
	cbor::chk_uint(1000000000000u, { 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 });
	cbor::chk_uint(18446744073709551615u, { 0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff });
}

TEST_CASE("integer")
{
	cbor::chk_int(0, { 0x00 });
	cbor::chk_int(1, { 0x01 });
	cbor::chk_int(10, { 0x0a });
	cbor::chk_int(23, { 0x17 });
	cbor::chk_int(24, { 0x18, 0x18 });
	cbor::chk_int(25, { 0x18, 0x19 });
	cbor::chk_int(100, { 0x18, 0x64 });
	cbor::chk_int(1000, { 0x19, 0x03, 0xe8 });
	cbor::chk_int(1000000, { 0x1a, 0x00, 0x0f, 0x42, 0x40 });
	cbor::chk_int(1000000000000, { 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 });
	cbor::chk_int(-1, { 0x20 });
	cbor::chk_int(-10, { 0x29 });
	cbor::chk_int(-100, { 0x38, 0x63 });
	cbor::chk_int(-1000, { 0x39, 0x03, 0xe7 });
}

bool validate_test_vecrtors()
{
	//TODO:
	return true;
}

TEST_CASE("test vectors")
{
	REQUIRE(validate_test_vecrtors());
}

TEST_CASE("boolean")
{
	cbor enc;

	SUBCASE("false")
	{
		enc.write_bool(false);
		enc.chk(vec{ 0xf4 });
	}

	SUBCASE("true")
	{
		enc.write_bool(true);
		enc.chk(vec{ 0xf5 });
	}

	SUBCASE("false and true")
	{
		enc.write_bool(false);
		enc.chk(vec{ 0xf4 });
		enc.write_bool(true);
		enc.chk(vec{ 0xf4, 0xf5 });
	}

	SUBCASE("true and false")
	{
		enc.write_bool(true);
		enc.chk(vec{ 0xf5 });
		enc.write_bool(false);
		enc.chk(vec{ 0xf5, 0xf4 });
	}
}
