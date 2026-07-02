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

	vec read_payload(uint64_t size)
	{
		vec r;
		r.reserve((std::size_t)size);
		for (uint64_t i = 0; i < size; ++i)
			r.push_back(get_byte());
		return r;
	}

	std::string read_text(uint64_t size)
	{
		std::string r;
		r.reserve((std::size_t)size);
		for (uint64_t i = 0; i < size; ++i)
			r.push_back((char)get_byte());
		return r;
	}

	bool finished() const { return pos == data.size(); }

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

TEST_CASE("float shortest (canonical)")
{
	// Values that fit exactly in half precision -> 2 bytes (0xf9)
	{ cbor enc; enc.write_float_shortest(0.0);     enc.chk(decode_hex("f90000")); }
	{ cbor enc; enc.write_float_shortest(-0.0);    enc.chk(decode_hex("f98000")); }
	{ cbor enc; enc.write_float_shortest(1.0);     enc.chk(decode_hex("f93c00")); }
	{ cbor enc; enc.write_float_shortest(1.5);     enc.chk(decode_hex("f93e00")); }
	{ cbor enc; enc.write_float_shortest(-4.0);    enc.chk(decode_hex("f9c400")); }
	{ cbor enc; enc.write_float_shortest(65504.0); enc.chk(decode_hex("f97bff")); }
	{ cbor enc; enc.write_float_shortest(5.960464477539063e-8); enc.chk(decode_hex("f90001")); }
	{ cbor enc; enc.write_float_shortest(6.103515625e-5);       enc.chk(decode_hex("f90400")); }

	// Values that need single precision -> 5 bytes (0xfa)
	{ cbor enc; enc.write_float_shortest(100000.0);            enc.chk(decode_hex("fa47c35000")); }
	{ cbor enc; enc.write_float_shortest(3.4028234663852886e+38); enc.chk(decode_hex("fa7f7fffff")); }

	// Values that need full double precision -> 9 bytes (0xfb)
	{ cbor enc; enc.write_float_shortest(1.1);      enc.chk(decode_hex("fb3ff199999999999a")); }
	{ cbor enc; enc.write_float_shortest(1.0e+300); enc.chk(decode_hex("fb7e37e43c8800759c")); }
	{ cbor enc; enc.write_float_shortest(-4.1);     enc.chk(decode_hex("fbc010666666666666")); }

	// Infinities collapse to half precision, NaN uses the canonical half NaN
	{ cbor enc; enc.write_float_shortest(std::numeric_limits<double>::infinity());   enc.chk(decode_hex("f97c00")); }
	{ cbor enc; enc.write_float_shortest(-std::numeric_limits<double>::infinity());  enc.chk(decode_hex("f9fc00")); }
	{ cbor enc; enc.write_float_shortest(std::numeric_limits<double>::quiet_NaN()); enc.chk(decode_hex("f97e00")); }
}

TEST_CASE("float shortest roundtrip preserves value")
{
	const double values[] = { 0.0, -0.0, 1.0, -1.0, 1.5, 65504.0, 100000.0,
		3.14159265358979, 1.1, 1.0e300, -2.5e-200, 123456789.0,
		5.960464477539063e-8 };
	for (double v : values)
	{
		cbor enc; enc.write_float_shortest(v);
		cbor_mem_decoder dec(enc.get_data());
		cbor_object o = dec.read();
		REQUIRE(o.is_float());
		REQUIRE(o.as_double() == v);
	}
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

TEST_CASE("as_int range checking")
{
	// INT64_MAX / INT64_MIN decode fine
	{
		cbor_mem_decoder dec(decode_hex("1b7fffffffffffffff"));
		REQUIRE(dec.read().as_int() == std::numeric_limits<int64_t>::max());
	}
	{
		cbor_mem_decoder dec(decode_hex("3b7fffffffffffffff"));
		REQUIRE(dec.read().as_int() == std::numeric_limits<int64_t>::min());
	}
	// 2^64-1 and -2^64 do not fit int64_t and must throw (previously they
	// silently returned -1 and 0)
	{
		cbor_mem_decoder dec(decode_hex("1bffffffffffffffff"));
		cbor_object o = dec.read();
		REQUIRE(o.as_uint() == 18446744073709551615u);
		REQUIRE_THROWS_AS(o.as_int(), cbor_decoder_exception);
	}
	{
		cbor_mem_decoder dec(decode_hex("3bffffffffffffffff"));
		cbor_object o = dec.read();
		REQUIRE(o.raw_value() == 18446744073709551615u);
		REQUIRE_THROWS_AS(o.as_int(), cbor_decoder_exception);
	}
}

TEST_CASE("malformed headers are rejected")
{
	// additional information 31 is not allowed for major types 0, 1 and 6
	for (const char* hex : { "1f", "3f", "df" })
	{
		cbor_mem_decoder dec(decode_hex(hex));
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
	// additional information 28..30 is reserved
	for (const char* hex : { "1c", "1d", "1e", "5d", "fc", "fe" })
	{
		cbor_mem_decoder dec(decode_hex(hex));
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
	// two-byte simple values below 32 are malformed (RFC 8949 section 3.3)
	{
		cbor_mem_decoder dec(decode_hex("f810"));
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
}

TEST_CASE("indefinite-length headers")
{
	// still valid and recognizable...
	{
		cbor_mem_decoder dec(decode_hex("5f7f9fbfff"));
		REQUIRE(dec.read().is_indefinite_bytes());
		REQUIRE(dec.read().is_indefinite_string());
		REQUIRE(dec.read().is_indefinite_array());
		REQUIRE(dec.read().is_indefinite_map());
		REQUIRE(dec.read().is_break());
	}
	// ...but they carry no length, so definite-length accessors must throw
	// (previously an indefinite string was silently read as an empty one)
	{
		cbor_mem_decoder dec(decode_hex("5f7f9fbf"));
		REQUIRE_THROWS_AS(dec.read().as_bytes_header(), cbor_decoder_exception);
		REQUIRE_THROWS_AS(dec.read().as_string_header(), cbor_decoder_exception);
		REQUIRE_THROWS_AS(dec.read().as_array(), cbor_decoder_exception);
		REQUIRE_THROWS_AS(dec.read().as_map(), cbor_decoder_exception);
	}
}

TEST_CASE("simple values")
{
	{ cbor enc; enc.write_simple(0);   enc.chk(decode_hex("e0")); }
	{ cbor enc; enc.write_simple(16);  enc.chk(decode_hex("f0")); }
	{ cbor enc; enc.write_simple(23);  enc.chk(decode_hex("f7")); } // undefined
	{ cbor enc; enc.write_simple(32);  enc.chk(decode_hex("f820")); }
	{ cbor enc; enc.write_simple(255); enc.chk(decode_hex("f8ff")); }
	{ cbor enc; REQUIRE_THROWS_AS(enc.write_simple(24), cbor_encoder_exception); }
	{ cbor enc; REQUIRE_THROWS_AS(enc.write_simple(31), cbor_encoder_exception); }

	{
		cbor_mem_decoder dec(decode_hex("f0f4f6f8ff"));
		cbor_object o = dec.read();
		REQUIRE(o.is_simple());
		REQUIRE(o.as_simple() == 16);
		o = dec.read(); // false is a simple value too
		REQUIRE(o.is_simple());
		REQUIRE(o.as_simple() == 20);
		o = dec.read(); // null
		REQUIRE(o.as_simple() == 22);
		o = dec.read();
		REQUIRE(o.as_simple() == 255);
	}
	{
		cbor_mem_decoder dec(decode_hex("01"));
		cbor_object o = dec.read();
		REQUIRE(!o.is_simple());
		REQUIRE_THROWS_AS(o.as_simple(), cbor_decoder_exception);
	}
}

TEST_CASE("indefinite-length string/bytes encoders")
{
	cbor enc;
	enc.write_indefinite_string();
	enc.write_string("strea");
	enc.write_string("ming");
	enc.write_break();
	enc.chk(decode_hex("7f657374726561646d696e67ff"));

	cbor enc2;
	enc2.write_indefinite_bytes();
	enc2.write_break();
	enc2.chk(decode_hex("5fff"));
}

TEST_CASE("istream decoder error handling")
{
	// empty string and empty byte string are fine
	{
		std::istringstream iss(std::string("\x60\x40", 2), std::ios::binary);
		cbor_decoder_istream dec(iss);
		REQUIRE(dec.read_string() == "");
		REQUIRE(dec.read_bytes() == vec());
	}
	// end of input instead of a header
	{
		std::istringstream iss(std::string(), std::ios::binary);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
	// truncated multi-byte header
	{
		std::istringstream iss(std::string("\x19\x01", 2), std::ios::binary);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
	// truncated string payload
	{
		std::istringstream iss(std::string("\x65\x61\x62", 3), std::ios::binary);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read_string(), cbor_decoder_exception);
	}
	// indefinite-length string can not be read as a definite one
	{
		std::istringstream iss(std::string("\x7f\x61\x61\xff", 4), std::ios::binary);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read_string(), cbor_decoder_exception);
	}
	// truncated input yields cbor_decoder_exception even when the caller has
	// enabled stream exceptions (as the README example does)
	{
		std::istringstream iss(std::string(), std::ios::binary);
		iss.exceptions(std::istream::failbit | std::istream::badbit);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
	{
		std::istringstream iss(std::string("\x19\x01", 2), std::ios::binary);
		iss.exceptions(std::istream::failbit | std::istream::badbit);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
	}
	{
		std::istringstream iss(std::string("\x65\x61\x62", 3), std::ios::binary);
		iss.exceptions(std::istream::failbit | std::istream::badbit);
		cbor_decoder_istream dec(iss);
		REQUIRE_THROWS_AS(dec.read_string(), cbor_decoder_exception);
	}
	// a stream with exceptions enabled still reads fine
	{
		std::istringstream iss(std::string("\x63\x61\x62\x63", 4), std::ios::binary);
		iss.exceptions(std::istream::failbit | std::istream::badbit);
		cbor_decoder_istream dec(iss);
		REQUIRE(dec.read_string() == "abc");
	}
}

TEST_CASE("float shortest keeps large finite values as doubles")
{
	// larger than FLT_MAX but finite: must be encoded as a double, not Inf
	{
		cbor enc; enc.write_float_shortest(3.5e38);
		cbor_mem_decoder dec(enc.get_data());
		cbor_object o = dec.read();
		REQUIRE(o.is_double_float());
		REQUIRE(o.as_double() == 3.5e38);
	}
	{
		cbor enc; enc.write_float_shortest(-3.5e38);
		cbor_mem_decoder dec(enc.get_data());
		REQUIRE(dec.read().as_double() == -3.5e38);
	}
}

// ---------------------------------------------------------------------------
// RFC 8949 Appendix A test vectors (appendix_a.json)
// ---------------------------------------------------------------------------

// Walk one data item (whose header `o` is already read), consuming any nested
// items and string payloads; throws on malformed input.
static void walk_item(cbor_mem_decoder& dec, const cbor_object& o)
{
	if (o.is_indefinite_bytes() || o.is_indefinite_string())
	{
		for (;;)
		{
			cbor_object chunk = dec.read();
			if (chunk.is_break())
				return;
			// chunks must be definite-length strings of the same major type
			REQUIRE(chunk.major_type() == o.major_type());
			dec.read_payload(chunk.major_type() == 2 ? chunk.as_bytes_header() : chunk.as_string_header());
		}
	}
	if (o.is_indefinite_array())
	{
		for (;;)
		{
			cbor_object e = dec.read();
			if (e.is_break())
				return;
			walk_item(dec, e);
		}
	}
	if (o.is_indefinite_map())
	{
		for (;;)
		{
			cbor_object k = dec.read();
			if (k.is_break())
				return;
			walk_item(dec, k);
			walk_item(dec, dec.read());
		}
	}
	switch (o.major_type())
	{
	case 2: dec.read_payload(o.as_bytes_header()); return;
	case 3: dec.read_payload(o.as_string_header()); return;
	case 4: for (uint64_t i = 0, n = o.as_array(); i < n; ++i) walk_item(dec, dec.read()); return;
	case 5: for (uint64_t i = 0, n = o.as_map(); i < n; ++i) { walk_item(dec, dec.read()); walk_item(dec, dec.read()); } return;
	case 6: walk_item(dec, dec.read()); return;
	default: return; // 0, 1, 7 have no content
	}
}

// Compare one decoded data item against the "decoded" JSON value.
static bool json_matches(cbor_mem_decoder& dec, const nlohmann::json& j);

static bool json_matches_item(cbor_mem_decoder& dec, const cbor_object& o, const nlohmann::json& j)
{
	if (j.is_null())
		return o.is_null();
	if (j.is_boolean())
		return o.is_bool() && o.as_bool() == j.get<bool>();
	if (j.is_number_unsigned())
		return o.major_type() == 0 && o.as_uint() == j.get<uint64_t>();
	if (j.is_number_integer())
		return (o.major_type() == 0 || o.major_type() == 1) && o.as_int() == j.get<int64_t>();
	if (j.is_number_float())
	{
		const double d = j.get<double>();
		if (o.is_float())
			return o.as_double() == d;
		// integers out of the int64/uint64 range are parsed from JSON as
		// doubles; compare CBOR major types 0/1 in the double domain
		if (o.major_type() == 0)
			return (double)o.raw_value() == d;
		if (o.major_type() == 1)
			return -1.0 - (double)o.raw_value() == d;
		return false;
	}
	if (j.is_string())
	{
		if (o.is_indefinite_string())
		{
			std::string s;
			for (;;)
			{
				cbor_object chunk = dec.read();
				if (chunk.is_break())
					break;
				s += dec.read_text(chunk.as_string_header());
			}
			return s == j.get<std::string>();
		}
		return o.is_string() && dec.read_text(o.as_string_header()) == j.get<std::string>();
	}
	if (j.is_array())
	{
		if (o.is_indefinite_array())
		{
			std::size_t i = 0;
			for (;;)
			{
				cbor_object e = dec.read();
				if (e.is_break())
					break;
				if (i >= j.size() || !json_matches_item(dec, e, j[i]))
					return false;
				++i;
			}
			return i == j.size();
		}
		if (!o.is_array() || o.as_array() != j.size())
			return false;
		for (const auto& e : j)
			if (!json_matches(dec, e))
				return false;
		return true;
	}
	if (j.is_object())
	{
		if (o.is_indefinite_map())
		{
			std::size_t n = 0;
			for (;;)
			{
				cbor_object k = dec.read();
				if (k.is_break())
					break;
				auto it = j.find(dec.read_text(k.as_string_header()));
				if (it == j.end() || !json_matches(dec, *it))
					return false;
				++n;
			}
			return n == j.size();
		}
		if (!o.is_map() || o.as_map() != j.size())
			return false;
		for (std::size_t i = 0; i < j.size(); ++i)
		{
			cbor_object k = dec.read();
			auto it = j.find(dec.read_text(k.as_string_header()));
			if (it == j.end() || !json_matches(dec, *it))
				return false;
		}
		return true;
	}
	return false;
}

static bool json_matches(cbor_mem_decoder& dec, const nlohmann::json& j)
{
	return json_matches_item(dec, dec.read(), j);
}

// Re-encode the "decoded" JSON value with the preferred serialization.
static void encode_json(cbor& enc, const nlohmann::json& j)
{
	if (j.is_null())
		enc.write_null();
	else if (j.is_boolean())
		enc.write_bool(j.get<bool>());
	else if (j.is_number_unsigned())
		enc.write_uint(j.get<uint64_t>());
	else if (j.is_number_integer())
		enc.write_int(j.get<int64_t>());
	else if (j.is_number_float())
		enc.write_float_shortest(j.get<double>());
	else if (j.is_string())
		enc.write_string(j.get<std::string>());
	else if (j.is_array())
	{
		enc.write_array(j.size());
		for (const auto& e : j)
			encode_json(enc, e);
	}
	else
	{
		enc.write_map(j.size());
		for (auto it = j.begin(); it != j.end(); ++it)
		{
			enc.write_string(it.key());
			encode_json(enc, it.value());
		}
	}
}

TEST_CASE("RFC 8949 Appendix A test vectors")
{
	std::ifstream f("appendix_a.json");
	REQUIRE(f);
	nlohmann::json vectors = nlohmann::json::parse(f);
	REQUIRE(vectors.size() == 82);

	std::size_t compared = 0, reencoded = 0;
	for (const auto& entry : vectors)
	{
		const vec bytes = decode_hex(entry["hex"].get<std::string>());
		INFO("hex: " << entry["hex"].get<std::string>());
		const uint8_t major = bytes.at(0) >> 5;

		// simple(24), i.e. 0xf818, comes from the RFC 7049 era vector set;
		// RFC 8949 section 3.3 made two-byte simple values below 32 not
		// well-formed, so the decoder must reject it now
		if (entry["hex"] == "f818")
		{
			cbor_mem_decoder dec(bytes);
			REQUIRE_THROWS_AS(dec.read(), cbor_decoder_exception);
			continue;
		}

		// every vector must decode structurally, consuming all bytes
		{
			cbor_mem_decoder dec(bytes);
			walk_item(dec, dec.read());
			REQUIRE(dec.finished());
		}

		if (!entry.contains("decoded"))
			continue;
		const nlohmann::json& decoded = entry["decoded"];

		// tagged vectors (bignums) can not be compared as plain JSON values
		if (major == 6)
			continue;

		// compare the decoded value
		{
			cbor_mem_decoder dec(bytes);
			REQUIRE(json_matches(dec, decoded));
			REQUIRE(dec.finished());
			++compared;
		}

		// re-encoding with the preferred serialization must reproduce the
		// exact bytes; skip 2^64 boundary integers that JSON parses as doubles
		if (entry["roundtrip"].get<bool>() && !(decoded.is_number_float() && major != 7))
		{
			cbor enc;
			encode_json(enc, decoded);
			REQUIRE(enc.get_data() == bytes);
			++reencoded;
		}
	}
	REQUIRE(compared == 57);
	REQUIRE(reencoded == 46);
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
