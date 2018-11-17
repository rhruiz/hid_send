OSNAME:=$(shell uname -s)

CFLAGS=-I/usr/local/lib -Wno-write-strings -I../qmk_firmware -lc

# Order of LDFLAGS matters! Remember, linking happens after compiling.
LDFLAGS=

ifeq ($(OSNAME),Darwin)
	CFLAGS += -lhidapi
else
	CFLAGS += -I/usr/local/include/hidapi
	LDFLAGS += -lhidapi-libusb
endif

.PHONY: all

all: hid_send.c config.h
	gcc $(CFLAGS) hid_send.c -o hid_send $(LDFLAGS)

clean:
	$(RM) hid_send

