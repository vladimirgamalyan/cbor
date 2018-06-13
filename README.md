# cbor
A lightning fast stream oriented CBOR encoder/decoder with no memory usage.

«аголовочные файлы дл€ чтени€/записи в [CBOR](http://cbor.io/) формате в потоковом режиме.

## ”паковка в CBOR
ƒобавл€м себе в проект `cbor_encoder.h`, наследуем cbor_encoder и определ€ем метод дл€ записи одного байта:

    virtual void put_byte(uint8_t b) = 0;

»спользуем методы `cbor_encoder` (`write_bool()`, `write_int()`, `write_array()` и т.д.) дл€ упаковки данных.
ѕри записи бинарных данных или строк сначала пишетс€ заголовок (`write_bytes_header()` или `write_string_header()`), а затем
сами данные (без участи€ класса `cbor_encoder`).

Ћибо используем готовый класс `cbor_encoder_ostream` из `cbor_encoder_ostream.h`, который расшир€ет cbor_encoder дл€ работы 
с потоками вывода (ostream) и добавл€ет удобные методы дл€ сохранени€ бинарных данных и строк.

## –аспаковка из CBOR
ƒобавл€ем в проект `cbor_decoder.h`, наследуем `cbor_decoder` и определ€ем ему метод дл€ чтени€ одного байта:

    virtual uint8_t get_byte() = 0;

ƒл€ получени€ следующей записи из потока CBOR используем метод `cbor_decoder::read()` который возвращает 
объект типа `cbor_object`. Ётот объект позвол€ет проверить свой тип (`is_bool()`, `is_string()` и т.д.) и получить
значение (`as_bool()`, `as_int()` и т.д). ≈сли попытатьс€ получить значение из объекта другого типа, то возникает 
исключение.

Ћибо используем готовый класс `cbor_decoder_istream` из `cbor_decoder_istream.h`, который расшир€ет `cbor_decoder` дл€ работы 
с потоками ввода (istream) и добавл€ет удобные методы дл€ чтени€ бинарных данных и строк.

## ѕример

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

