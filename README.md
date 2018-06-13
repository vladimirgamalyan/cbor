# cbor
A lightning fast stream oriented CBOR encoder/decoder with no memory usage.

������������ ����� ��� ������/������ � [CBOR](http://cbor.io/) ������� � ��������� ������.

## �������� � CBOR
�������� ���� � ������ `cbor_encoder.h`, ��������� cbor_encoder � ���������� ����� ��� ������ ������ �����:

    virtual void put_byte(uint8_t b) = 0;

���������� ������ `cbor_encoder` (`write_bool()`, `write_int()`, `write_array()` � �.�.) ��� �������� ������.
��� ������ �������� ������ ��� ����� ������� ������� ��������� (`write_bytes_header()` ��� `write_string_header()`), � �����
���� ������ (��� ������� ������ `cbor_encoder`).

���� ���������� ������� ����� `cbor_encoder_ostream` �� `cbor_encoder_ostream.h`, ������� ��������� cbor_encoder ��� ������ 
� �������� ������ (ostream) � ��������� ������� ������ ��� ���������� �������� ������ � �����.

## ���������� �� CBOR
��������� � ������ `cbor_decoder.h`, ��������� `cbor_decoder` � ���������� ��� ����� ��� ������ ������ �����:

    virtual uint8_t get_byte() = 0;

��� ��������� ��������� ������ �� ������ CBOR ���������� ����� `cbor_decoder::read()` ������� ���������� 
������ ���� `cbor_object`. ���� ������ ��������� ��������� ���� ��� (`is_bool()`, `is_string()` � �.�.) � ��������
�������� (`as_bool()`, `as_int()` � �.�). ���� ���������� �������� �������� �� ������� ������� ����, �� ��������� 
����������.

���� ���������� ������� ����� `cbor_decoder_istream` �� `cbor_decoder_istream.h`, ������� ��������� `cbor_decoder` ��� ������ 
� �������� ����� (istream) � ��������� ������� ������ ��� ������ �������� ������ � �����.

## ������

	{
		std::ofstream f("test.bin", std::fstream::binary);
		f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cbor_encoder_ostream encoder(f);
		encoder.write_array(2);
		encoder.write_string("Hello, World!");
		encoder.write_int(42);
	}

	{
		std::ifstream f("test.bin", std::fstream::binary);
		f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		cbor_decoder_istream decoder(f);

		std::cout << "array size: " << decoder.read_array()  << std::endl;
		std::cout << decoder.read_string()  << std::endl;
		std::cout << decoder.read().as_int() << std::endl;
	}

