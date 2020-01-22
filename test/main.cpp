#include <iostream>
#include <fstream>
#include "../cbor_encoder_ostream.h"
#include "../cbor_decoder_istream.h"

int main(int argc, char *argv[]) {
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

		std::cout << "array size: " << decoder.read().as_array() << std::endl;
		std::cout << decoder.read_string()  << std::endl;
		std::cout << decoder.read().as_int() << std::endl;
	}
}

