# Send HID raw data command line tool

This sends commands to be used by qmk firmware that handles raw HID data.

This is completely stolen from [Zeal60 command line tool](https://github.com/Wilba6582/zeal60).


## Setup

This asumes `qmk` is cloned next to this folder:

```
git clone git@github.com:rhruiz/hid_send.git
git clone git@github.com:rhruiz/qmk_firmware.git
```

### macOS setup

Install patched version of hidapi:

```shell
git clone https://github.com/signal11/hidapi.git
cd hidapi
git remote add dylanmckay https://github.com/dylanmckay/hidapi.git
git fetch dylanmckay
git checkout mac-hid-interface-support
./bootstrap
./configure
make
sudo make install
cd ../
```

## Building

Just run:

```
cd hid_send
make
```
