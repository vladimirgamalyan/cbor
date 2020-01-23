# cbor
A lightning fast, header only, stream oriented [CBOR](http://cbor.io/) encoder/decoder with no memory usage.

[![Actions Status](https://github.com/vladimirgamalyan/cbor/workflows/ci/badge.svg)](https://github.com/vladimirgamalyan/cbor/actions)


## Encode CBOR
Inherit `cbor_encoder` class from `cbor_encoder.h`, and override `put_byte` method for write one byte anywhere you like:

    virtual void put_byte(uint8_t b);

Then use `cbor_encoder` methods (`write_bool()`, `write_int()`, `write_array()` etc.) to encode data. To write a byte array or a string to stream, first call `write_bytes_header()` or `write_string_header()`) and then write the data to output stream directly.

A helper class `cbor_encoder_ostream` from `cbor_encoder_ostream.h` extends `cbor_encoder` to work with standart `std::istream`.

## Decode CBOR
Inherit `cbor_decoder` from `cbor_decoder.h`, and override `get_byte` to read one byte from input source:

    virtual uint8_t get_byte();

Then use `cbor_decoder::read()` to read next records (`cbor_object`) from the input (it will raise an exception on bad records). The `cbor_object` has type check methods (`is_bool()`, `is_string()` etc.) and get value methods (`as_bool()`, `as_int()`). Methods for get values will raise an exception when real type of records is different.

A helper class `cbor_decoder_istream` from `cbor_decoder_istream.h` extends `cbor_decoder` to work with standart `std::ostream`.

## Example

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
