# cbor
A lightning fast stream oriented CBOR encoder/decoder with no memory usage.

Заголовочные файлы для чтения/записи в [CBOR](http://cbor.io/) формате в потоковом режиме.

## Упаковка в CBOR
Добавлям себе в проект `cbor_encoder.h`, наследуем `cbor_encoder` и определяем метод для записи одного байта:

    virtual void put_byte(uint8_t b);

Используем методы `cbor_encoder` (`write_bool()`, `write_int()`, `write_array()` и т.д.) для упаковки данных.
При записи бинарных данных или строк сначала пишется заголовок (`write_bytes_header()` или `write_string_header()`), а затем
сами данные (без участия класса `cbor_encoder`).

Либо используем готовый класс `cbor_encoder_ostream` из `cbor_encoder_ostream.h`, который расширяет `cbor_encoder` для работы 
со стандартными потоками вывода (std::ostream) и добавляет удобные методы для сохранения бинарных данных и строк.

## Распаковка из CBOR
Добавляем в проект `cbor_decoder.h`, наследуем `cbor_decoder` и определяем ему метод для чтения одного байта:

    virtual uint8_t get_byte();

Для получения следующей записи из потока CBOR используем метод `cbor_decoder::read()` (вызовет исключение, если запись некорректная), 
который возвращает объект типа `cbor_object`. Этот объект позволяет проверить свой тип (`is_bool()`, `is_string()` и т.д.) и получить
значение (`as_bool()`, `as_int()` и т.д). Если попытаться получить значение, несоответствующее типу объекта, то возникает 
исключение.

Либо используем готовый класс `cbor_decoder_istream` из `cbor_decoder_istream.h`, который расширяет `cbor_decoder` для работы 
со стандартными потоками ввода (std::istream) и добавляет удобные методы для чтения бинарных данных и строк.

## Пример

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

