CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra

HEADERS = ../cbor_encoder.h ../cbor_decoder.h ../cbor_encoder_ostream.h ../cbor_decoder_istream.h

test: main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp -o test

clean:
	rm -f test test.bin

.PHONY: clean
