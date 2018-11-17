#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <cstring>
#include <string>
#include <iostream>

#include <vector>
#include <string>

#include <unistd.h>

#include "hidapi/hidapi.h"
#include "config.h"
#include "users/rhruiz/rhruiz_api.h"

hid_device* hid_open(unsigned short vendor_id, unsigned short product_id,
                     unsigned short interface_number) {
  hid_device* device = NULL;
  struct hid_device_info* deviceInfos;
  struct hid_device_info* currentDeviceInfo;
  struct hid_device_info* foundDeviceInfo = NULL;
  deviceInfos = hid_enumerate(vendor_id, product_id);
  currentDeviceInfo = deviceInfos;
  while (currentDeviceInfo) {
    if (currentDeviceInfo->interface_number == interface_number) {
      if (foundDeviceInfo) {
        // More than one matching device.
        // TODO: return error?
      } else {
        foundDeviceInfo = currentDeviceInfo;
      }
    }
    currentDeviceInfo = currentDeviceInfo->next;
  }

  if (foundDeviceInfo) {
    device = hid_open_path(foundDeviceInfo->path);
  }

  hid_free_enumeration(deviceInfos);

  return device;
}

std::vector<std::string> hid_get_device_paths(unsigned short vendor_id,
                                              unsigned short product_id,
                                              unsigned short interface_number) {
  std::vector<std::string> devicePaths;
  struct hid_device_info* deviceInfos;
  struct hid_device_info* currentDeviceInfo;
  struct hid_device_info* foundDeviceInfo = NULL;
  deviceInfos = hid_enumerate(vendor_id, product_id);
  currentDeviceInfo = deviceInfos;
  while (currentDeviceInfo) {
    if (currentDeviceInfo->interface_number == interface_number) {
      devicePaths.push_back(currentDeviceInfo->path);
    }
    currentDeviceInfo = currentDeviceInfo->next;
  }

  hid_free_enumeration(deviceInfos);

  return devicePaths;
}

bool send_message(hid_device* device, uint8_t id, void* outMsg = NULL,
                  uint8_t outMsgLength = 0, void* retMsg = NULL,
                  uint8_t retMsgLength = 0) {
  // assert( outMsgLength <= RAW_HID_BUFFER_SIZE );
  if (outMsgLength > RAW_HID_BUFFER_SIZE) {
    printf("Message size %d is bigger than maximum %d\n", outMsgLength,
           RAW_HID_BUFFER_SIZE);
    return false;
  }

  int res;
  uint8_t data[RAW_HID_BUFFER_SIZE + 1];
  memset(data, 0xFE, sizeof(data));
  data[0] = 0x00;  // NULL report ID. IMPORTANT!
  data[1] = id;

  if (outMsg && outMsgLength > 0) {
    memcpy(&data[2], outMsg, outMsgLength);
  }

  res = 0;
  res = hid_write(device, data, RAW_HID_BUFFER_SIZE + 1);
  if (res < 0) {
    printf("Unable to write()\n");
    printf("Error: %ls\n", hid_error(device));
    return false;
  }

  hid_set_nonblocking(device, 1);

  res = 0;
  // Timeout after 500ms
  for (int i = 0; i < 500; i++) {
    res = hid_read(device, data, RAW_HID_BUFFER_SIZE);
    if (res != 0) {
      break;
    }
// waiting
#ifdef WIN32
    Sleep(1);
#else
    usleep(1 * 1000);
#endif
  }

  if (res < 0) {
    printf("Unable to read()\n");
    printf("Error: %ls\n", hid_error(device));
    return false;
  }

  if (res > 0) {
    if (retMsg && retMsgLength > 0) {
      memcpy(retMsg, &data[1], retMsgLength);
    }
  }

  return true;
}

bool get_keyboard_value_uint32(hid_device* device, uint8_t value_id,
                               uint32_t* value) {
  uint8_t msg[5];
  msg[0] = value_id;
  msg[1] = 0xFF;
  msg[2] = 0xFF;
  msg[3] = 0xFF;
  msg[4] = 0xFF;
  if (send_message(device, id_get_keyboard_value, msg, sizeof(msg), msg,
                   sizeof(msg))) {
    *value = (msg[1] << 24) | (msg[2] << 16) | (msg[3] << 8) | msg[4];
    return true;
  }
  return false;
}

hid_device* hid_open_least_uptime(unsigned short vendor_id,
                                  unsigned short product_id,
                                  unsigned short interface_number) {
  std::vector<std::string> devicePaths =
      hid_get_device_paths(vendor_id, product_id, interface_number);

  // early abort
  if (devicePaths.size() == 0) {
    return NULL;
  }

  // no need to check ticks
  if (devicePaths.size() == 1) {
    return hid_open_path(devicePaths[0].c_str());
  }

  std::string bestDevicePath;
  uint32_t bestDeviceTick = 0;

  for (int i = 0; i < (int)devicePaths.size(); i++) {
    hid_device* device = hid_open_path(devicePaths[i].c_str());

    uint32_t thisDeviceTick = 0;
    if (!get_keyboard_value_uint32(device, id_uptime, &thisDeviceTick)) {
      std::cerr << "*** Error: Error getting uptime" << std::endl;
      hid_close(device);
      continue;
    }

    if (bestDevicePath.empty() || thisDeviceTick < bestDeviceTick) {
      bestDevicePath = devicePaths[i];
      bestDeviceTick = thisDeviceTick;
    }

    hid_close(device);
  }

  if (!bestDevicePath.empty()) {
    return hid_open_path(bestDevicePath.c_str());
  }

  return NULL;
}

void hid_test(void) {
  struct hid_device_info *devs, *cur_dev;

  devs = hid_enumerate(0x0, 0x0);
  cur_dev = devs;
  while (cur_dev) {
    printf(
        "Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
        cur_dev->vendor_id, cur_dev->product_id, cur_dev->path,
        cur_dev->serial_number);
    printf("\n");
    printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
    printf("  Product:      %ls\n", cur_dev->product_string);
    printf("  Release:      %hx\n", cur_dev->release_number);
    printf("  Interface:    %d\n", cur_dev->interface_number);
    printf("\n");
    cur_dev = cur_dev->next;
  }
  hid_free_enumeration(devs);
}

bool send_no_arg_message(hid_device* device, uint8_t id) {
  uint8_t msg[2];
  return send_message(device, id, msg, sizeof(msg), msg, sizeof(msg));
}

bool set_rgblight_to(hid_device* device, uint16_t hue, uint8_t sat) {
  uint8_t msg[3];

  msg[0] = (uint8_t)((hue & 0xFF00) >> 8);
  msg[1] = (uint8_t)((hue & 0x00FF));
  msg[2] = sat;

  return send_message(device, id_rgblight_color, msg, sizeof(msg), msg,
                      sizeof(msg));
}

int main(int argc, char** argv) {
  if (hid_init()) {
    std::cerr << "*** Error: hidapi initialization failed" << std::endl;
    return -1;
  }

  if (argc <= 1) {
    // No args, do nothing
    return 0;
  }

  // First arg is the command
  std::string command = argv[1];

  if (command == "hidtest") {
    std::cout << "Running hidtest" << std::endl;
    hid_test();
    return 0;
  }

  hid_device* device =
      hid_open_least_uptime(DEVICE_VID, DEVICE_PID, DEVICE_INTERFACE_NUMBER);

  if (!device) {
    std::cerr << "*** Error: Device not found" << std::endl;
    return -1;
  }

  bool res = false;

  if (command == "rgblight_color") {
    if (argc < 4) {
      std::cerr << "Usage " << argv[0] << " rgblight_color hue sat"
                << std::endl;
    } else {
      uint16_t hue;
      uint8_t sat;

      if (sscanf(argv[2], "%" SCNu16 "", &hue) == 1 &&
          sscanf(argv[3], "%" SCNu8 "", &sat) == 1) {
        if (hue >= 0 && hue <= 360 && sat >= 0 && sat <= 255) {
          res = set_rgblight_to(device, hue, sat);
        } else {
          std::cerr << "Invalid hue / sat" << std::endl;
        }
      } else {
        std::cerr << "Invalid hue / sat" << std::endl;
      }
    }
  }

  if (command == "bootloader") {
    res = send_no_arg_message(device, id_bootloader_jump);
  }

  if (command == "rgblight_reset") {
    res = send_no_arg_message(device, id_rgblight_reset);
  }

  if (command == "backlight_toggle") {
    res = send_no_arg_message(device, id_backlight_toggle);
  }

  if (command == "rgblight_toggle") {
    res = send_message(device, id_rgblight_toggle);
  }

  if (res != true) {
    return -1;
  }
}
