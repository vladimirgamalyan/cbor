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

	vec get_data() const
	{
		std::string str = oss.str();
		return { str.begin(), str.end() };
	}

private:
	std::ostringstream oss;
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

static vec decode_hex(const std::string& s)
{
	vec out;
	for (std::size_t i = 0; i + 1 < s.size(); i += 2)
		out.push_back((uint8_t)std::stoul(s.substr(i, 2), nullptr, 16));
	return out;
}

class cbor_mem_decoder : public cbor_decoder
{
public:
	explicit cbor_mem_decoder(vec data) : data(std::move(data)) {}
protected:
	uint8_t get_byte() override { return data.at(pos++); }
private:
	vec data;
	std::size_t pos = 0;
};

static double decode_double(const std::string& hex)
{
	cbor_mem_decoder dec(decode_hex(hex));
	cbor_object o = dec.read();
	REQUIRE(o.is_float());
	return o.as_double();
}

TEST_CASE("float decode")
{
	REQUIRE(decode_double("f90000") == 0.0);
	REQUIRE(decode_double("f98000") == 0.0); // -0.0
	REQUIRE(std::signbit(decode_double("f98000")));
	REQUIRE(decode_double("f93c00") == 1.0);
	REQUIRE(decode_double("f93e00") == 1.5);
	REQUIRE(decode_double("f97bff") == 65504.0);
	REQUIRE(decode_double("f90001") == doctest::Approx(5.960464477539063e-8));
	REQUIRE(decode_double("f90400") == doctest::Approx(6.103515625e-5));
	REQUIRE(decode_double("f9c400") == -4.0);
	REQUIRE(decode_double("fa47c35000") == 100000.0);
	REQUIRE(decode_double("fa7f7fffff") == doctest::Approx(3.4028234663852886e+38));
	REQUIRE(decode_double("fb3ff199999999999a") == doctest::Approx(1.1));
	REQUIRE(decode_double("fb7e37e43c8800759c") == doctest::Approx(1.0e+300));
	REQUIRE(decode_double("fbc010666666666666") == doctest::Approx(-4.1));
	REQUIRE(std::isinf(decode_double("f97c00")));
	REQUIRE(decode_double("f97c00") > 0);
	REQUIRE(std::isnan(decode_double("f97e00")));
	REQUIRE(std::isinf(decode_double("f9fc00")));
	REQUIRE(decode_double("f9fc00") < 0);
	REQUIRE(std::isinf(decode_double("fa7f800000")));
	REQUIRE(std::isnan(decode_double("fa7fc00000")));
	REQUIRE(std::isinf(decode_double("fb7ff0000000000000")));
	REQUIRE(std::isnan(decode_double("fb7ff8000000000000")));
}

TEST_CASE("float encode")
{
	{ cbor enc; enc.write_double(1.1);                    enc.chk(decode_hex("fb3ff199999999999a")); }
	{ cbor enc; enc.write_double(1.0e+300);               enc.chk(decode_hex("fb7e37e43c8800759c")); }
	{ cbor enc; enc.write_double(-4.1);                   enc.chk(decode_hex("fbc010666666666666")); }
	{ cbor enc; enc.write_float(100000.0f);               enc.chk(decode_hex("fa47c35000")); }
	{ cbor enc; enc.write_float(3.4028234663852886e+38f); enc.chk(decode_hex("fa7f7fffff")); }
	{ cbor enc; enc.write_half_float(0.0f);               enc.chk(decode_hex("f90000")); }
	{ cbor enc; enc.write_half_float(1.0f);               enc.chk(decode_hex("f93c00")); }
	{ cbor enc; enc.write_half_float(1.5f);               enc.chk(decode_hex("f93e00")); }
	{ cbor enc; enc.write_half_float(65504.0f);           enc.chk(decode_hex("f97bff")); }
	{ cbor enc; enc.write_half_float(-4.0f);              enc.chk(decode_hex("f9c400")); }
	{ cbor enc; enc.write_half_float(6.103515625e-5f);    enc.chk(decode_hex("f90400")); }  // smallest normal
	{ cbor enc; enc.write_half_float(5.960464477539063e-8f); enc.chk(decode_hex("f90001")); }  // smallest subnormal
}

TEST_CASE("float roundtrip")
{
	const double values[] = { 0.0, -0.0, 1.0, -1.0, 3.14159265358979,
		1.0e300, -2.5e-200, 123456789.0 };
	for (double v : values)
	{
		cbor enc; enc.write_double(v);
		cbor_mem_decoder dec(enc.get_data());
		cbor_object o = dec.read();
		REQUIRE(o.is_double_float());
		REQUIRE(o.as_double() == doctest::Approx(v));
	}
}

bool validate_test_vectors()
{
	//TODO:
	return true;
}

TEST_CASE("test vectors")
{
	REQUIRE(validate_test_vectors());
}

TEST_CASE("boolean")
{
	cbor enc;

	SUBCASE("false")
	{
		enc.write_bool(false);
		enc.chk({ 0xf4 });
	}

	SUBCASE("true")
	{
		enc.write_bool(true);
		enc.chk({ 0xf5 });
	}

	SUBCASE("false and true")
	{
		enc.write_bool(false);
		enc.chk({ 0xf4 });
		enc.write_bool(true);
		enc.chk({ 0xf4, 0xf5 });
	}

	SUBCASE("true and false")
	{
		enc.write_bool(true);
		enc.chk({ 0xf5 });
		enc.write_bool(false);
		enc.chk({ 0xf5, 0xf4 });
	}
}
