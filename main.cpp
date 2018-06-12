#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "cbor_encoder.h"
#include "cbor_decoder.h"

class cbor_writer_sream : public cbor_writer
{
public:
	cbor_writer_sream(std::ostream& s) : s(s) {}
	virtual void put_bytes(const uint8_t* data, uint64_t size) override {
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
	cbor_encoder_ex(cbor_writer& writer) : cbor_encoder(writer) {}

	void write_cpp_string(const std::string& s) {
		write_string(s.c_str(), s.size());
	}
	void write_vector(const std::vector<uint8_t>& v) {
		write_bytes(&v[0], v.size());
	}
};

class cbor_decoder_ex : public cbor_decoder {
public:
	cbor_decoder_ex(cbor_reader_sream& reader) : cbor_decoder(reader), reader_stream(reader) {}

	std::string read_cpp_string() {
		uint64_t size = read_string();
		std::string s((size_t)size, ' ');
		reader_stream.get_bytes((uint8_t*)&s[0], size);
		return s;
	}
private:
	cbor_reader_sream& reader_stream;
};

int main() {

	{
		std::ofstream f("test.bin", std::fstream::binary);
		f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cbor_writer_sream writer(f);
		cbor_encoder_ex encoder(writer);
		encoder.write_cpp_string("Hello, World!");
		encoder.write_int(42);
	}

	{
		std::ifstream f("test.bin", std::fstream::binary);
		f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cbor_reader_sream reader(f);
		cbor_decoder_ex decode(reader);

		std::cout << decode.read_cpp_string() << std::endl;
		std::cout << decode.read_int() << std::endl;
	}

    return 0;
}
