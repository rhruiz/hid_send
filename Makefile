OSNAME:=$(shell uname -s)

# Fix -I if yoyr qmk user space is in a different location
CFLAGS=-Wno-write-strings -I../qmk-userspace -lc

# Order of LDFLAGS matters! Remember, linking happens after compiling.
LDFLAGS=

ifeq ($(OSNAME),Darwin)
	BREW_PREFIX:=$(shell brew --prefix)
	CFLAGS += -I$(BREW_PREFIX)/include -I$(BREW_PREFIX)/lib -I/usr/local/lib -DOS_DARWIN
	LDFLAGS += -L$(BREW_PREFIX)/lib -lhidapi
else
	CFLAGS += -I/usr/local/include/hidapi
	LDFLAGS += -lhidapi-libusb
endif

ifdef VID
	CFLAGS += -DDEVICE_VID=$(VID)
endif

ifdef PID
	CFLAGS += -DDEVICE_PID=$(PID)
endif

.PHONY: all

all: hid_send.c config.h
	gcc $(CFLAGS) hid_send.c -o hid_send $(LDFLAGS)

clean:
	$(RM) hid_send

