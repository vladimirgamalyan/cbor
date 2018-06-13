#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include "cbor_encoder.h"
#include "cbor_decoder.h"
#include "cbor_decoder_istream.h"
#include "cbor_encoder_ostream.h"

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
		f.exceptions(std::fstream::failbit | std::fstream::badbit);
		cbor_encoder_ostream encoder(f);
		encoder.write_string("Hello, World!");
		encoder.write_int(42);
		encoder.write_bytes({ 0, 1, 2, 10 });
	}

	{
		std::ifstream f("test.bin", std::fstream::binary);
		f.exceptions(std::fstream::failbit | std::fstream::badbit);
		cbor_decoder_istream decoder(f);

		std::cout << decoder.read_string()  << std::endl;
		std::cout << decoder.read().as_int() << std::endl;
		std::cout << decoder.read_bytes()  << std::endl;
	}

    return 0;
}
