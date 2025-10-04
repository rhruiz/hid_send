# Send HID raw data command line tool

This sends commands to be used by qmk t to this folder:

```
git clone git@github.com:rhruiz/hid_send.git
git clone git@github.com:rhruiz/qmk-userspace.git qmk_userspace
```

### macOS setup

```shell
brew instal hidapi # brew qmk/qmk/qmk already installed this
```


## Building

Just run:

```
cd hid_send
make
```

## Setting vendor and product id

Default values are taken from the constants in hid_send.c and they can be
overriden using VID and PID environment variables just be sure to use hex
numbers with 0x format like `0xFEED`.

These variables may also be set when running `make` and the binary will be
generated to values by default.
