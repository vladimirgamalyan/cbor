#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include "cbor_encoder.h"
#include "cbor_decoder.h"

class cbor_writer_sream : public cbor_writer
{
public:
	cbor_writer_sream(std::ostream& s) : s(s) {}
	virtual void put_byte(uint8_t b) override {
		s.put((char)b);
	}
 	void put_bytes(const uint8_t* data, uint64_t size) {
 		s.write((const char*)data, (std::streamsize)size);
 	}
private:
	std::ostream& s;
};

class cbor_reader_sream : public cbor_reader
{
public:
	cbor_reader_sream(std::istream& s) : s(s) {}
	virtual uint8_t get_byte() override {
		return (uint8_t)s.get();
	}
	void get_bytes(uint8_t* buffer, uint64_t size) {
		s.read((char*)buffer, (std::streamsize)size);
	}
private:
	std::istream& s;
};

class cbor_encoder_ex : public cbor_encoder {
public:
	cbor_encoder_ex(cbor_writer_sream& writer) : cbor_encoder(writer), writer_stream(writer){}

	void write_string(const std::string& s) {
		write_string_header(s.size());
		writer_stream.put_bytes((const uint8_t*)s.c_str(), s.size());
	}
	void write_bytes(const std::vector<uint8_t>& v) {
		write_bytes_header(v.size());
		writer_stream.put_bytes(&v[0], v.size());
	}
private:
	cbor_writer_sream& writer_stream;
};

class cbor_decoder_ex : public cbor_decoder {
public:
	cbor_decoder_ex(cbor_reader_sream& reader) : cbor_decoder(reader), reader_stream(reader) {}

	std::string read_string() {
		uint64_t size = read_string_header();
		std::string s((size_t)size, ' ');
		reader_stream.get_bytes((uint8_t*)&s[0], size);
		return s;
	}

	std::vector<uint8_t> read_bytes() {
		uint64_t size = read_bytes_header();
		std::vector<uint8_t> v((size_t)size);
		reader_stream.get_bytes((uint8_t*)&v[0], size);
		return v;
	}
private:
	cbor_reader_sream& reader_stream;
};

class ios_flag_saver {
public:
	explicit ios_flag_saver(std::ostream& _ios) :
		ios(_ios),
		f(_ios.flags()) {
	}
	~ios_flag_saver() {
		ios.flags(f);
	}

	ios_flag_saver(ios_flag_saver const&) = delete;
	ios_flag_saver(ios_flag_saver&&) = delete;
	ios_flag_saver& operator=(ios_flag_saver const&) = delete;
	ios_flag_saver& operator=(ios_flag_saver &&) = delete;

private:
	std::ostream& ios;
	std::ios::fmtflags f;
};

std::ostream& operator<<(std::ostream& os, const std::vector<uint8_t>& v)
{
	ios_flag_saver iosfs(std::cout);
	os << "[" << std::hex << std::setfill('0');
	for (size_t i = 0; i < v.size(); ++i) {
		os << "0x" << std::setw(2) << (int)v[i];
		if (i != v.size() - 1)
			os << ", ";
	}
	os << "]";
	return os;
}

int main() {

	{
		std::ofstream f("test.bin", std::fstream::binary);
		f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cbor_writer_sream writer(f);
		cbor_encoder_ex encoder(writer);
		encoder.write_string("Hello, World!");
		encoder.write_int(42);
		encoder.write_bytes({ 0, 1, 2, 10 });
	}

	{
		std::ifstream f("test.bin", std::fstream::binary);
		f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cbor_reader_sream reader(f);
		cbor_decoder_ex decode(reader);

		std::cout << decode.read_string() << std::endl;
		std::cout << decode.read_int() << std::endl;
		std::cout << decode.read_bytes() << std::endl;
	}

    return 0;
}
