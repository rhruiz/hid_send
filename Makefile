OSNAME:=$(shell uname -s)

CPPFLAGS=-I/usr/local/lib -Wno-write-strings -I../qmk_firmware

# Order of LDFLAGS matters! Remember, linking happens after compiling.
LDFLAGS=

ifeq ($(OSNAME),Darwin)
	CPPFLAGS += -lhidapi
else
	CPPFLAGS += -I/usr/local/include/hidapi
	LDFLAGS += -lhidapi-libusb
endif

.PHONY: all

all: hid_send.cpp config.h
	g++ $(CPPFLAGS) hid_send.cpp -o hid_send $(LDFLAGS)

clean:
	$(RM) main

