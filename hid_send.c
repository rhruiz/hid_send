#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <hidapi/hidapi.h>

// qmk includes
#include <users/rhruiz/raw_hid/api.h>

#include "config.h"

typedef struct device_path_s device_path_t;
struct device_path_s {
  char *path;
  SLIST_ENTRY(device_path_s) entries;
};

typedef SLIST_HEAD(device_path_list, device_path_s) device_path_list_t;

device_path_list_t* hid_get_device_paths(unsigned short vendor_id,
                                   unsigned short product_id,
                                   unsigned short interface_number) {

  device_path_t *new_device_path;
  device_path_list_t *device_paths = malloc(sizeof(device_path_list_t));

  SLIST_INIT(device_paths);

  struct hid_device_info* device_infos;
  struct hid_device_info* current_device_info;
  struct hid_device_info* found_ddevice_info = NULL;
  device_infos = hid_enumerate(vendor_id, product_id);
  current_device_info = device_infos;

  while (current_device_info) {
    if (current_device_info->interface_number == interface_number) {

      char *device_path = malloc(strlen(current_device_info->path));
      strcpy(device_path, current_device_info->path);

      new_device_path = malloc(sizeof(device_path_t));
      new_device_path->path = device_path;
      SLIST_INSERT_HEAD(device_paths, new_device_path, entries);
    }

    current_device_info = current_device_info->next;
  }

  hid_free_enumeration(device_infos);

  return device_paths;
}

bool send_message(hid_device* device, uint8_t id, void* out_msg,
                  uint8_t out_msg_length, void* ret_msg,
                  uint8_t ret_msg_length) {
  if (out_msg_length > RAW_HID_BUFFER_SIZE) {
    printf("Message size %d is bigger than maximum %d\n", out_msg_length,
           RAW_HID_BUFFER_SIZE);
    return false;
  }

  int res;
  uint8_t data[RAW_HID_BUFFER_SIZE + 1];
  memset(data, 0xFE, sizeof(data));
  data[0] = 0x00;  // NULL report ID. IMPORTANT!
  data[1] = id;

  if (out_msg && out_msg_length > 0) {
    memcpy(&data[2], out_msg, out_msg_length);
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
    if (ret_msg && ret_msg_length > 0) {
      memcpy(ret_msg, &data[1], ret_msg_length);
    }
  }

  return true;
}

bool send_no_args_message(hid_device *device, uint8_t id) {
  uint8_t msg[0];
  return send_message(device, id, msg, 0, msg, 0);
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

  device_path_list_t *paths = hid_get_device_paths(vendor_id, product_id, interface_number);

  // early abort
  if (SLIST_EMPTY(paths)) {
    return NULL;
  }

  device_path_t *device_path = SLIST_FIRST(paths);

  // no need to check ticks
  if (SLIST_NEXT(device_path, entries) == NULL) {
    return hid_open_path(device_path->path);
  }

  char *best_device_path = NULL;
  uint32_t best_device_tick = 0;

  SLIST_FOREACH(device_path, paths, entries) {
    hid_device* device = hid_open_path(device_path->path);

    uint32_t this_device_tick = 0;

    if (!get_keyboard_value_uint32(device, id_uptime, &this_device_tick)) {
      fprintf(stderr, "*** Error: Error getting uptime\n");
      hid_close(device);
      continue;
    }

    if (best_device_path == NULL || this_device_tick < best_device_tick) {
      best_device_path = device_path->path;
      best_device_tick = this_device_tick;
    }

    free(device_path);
    hid_close(device);
  }

  free(paths);

  if (best_device_path != NULL) {
    return hid_open_path(best_device_path);
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

bool oled_write(hid_device* device, char *msg) {
  printf("sending %s\n", msg);
  return send_message(device, id_oled_write, msg, strlen(msg), msg,
                      strlen(msg));
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
    fprintf(stderr, "*** Error: hidapi initialization failed\n");
    return -1;
  }

  if (argc <= 1) {
    // No args, do nothing
    return 0;
  }

  // First arg is the command
  char *command = argv[1];

  if (strcmp(command, "hidtest") == 0) {
    hid_test();
    return 0;
  }

  bool verbose = getenv("VERBOSE") != NULL;
  unsigned short device_vid = DEVICE_VID;
  unsigned short device_pid = DEVICE_PID;
  char *v;

  if ((v = getenv("VID")) != NULL) {
    if (verbose) {
      fprintf(stderr, "reading vid %s\n", v);
    }

    sscanf(v, "%hX", &device_vid);
  }

  if ((v = getenv("PID")) != NULL) {
    if (verbose) {
      fprintf(stderr, "reading pid %s\n", v);
    }

    sscanf(v, "%hX", &device_pid);
  }

  hid_device* device =
      hid_open_least_uptime(device_vid, device_pid, DEVICE_INTERFACE_NUMBER);

  if (!device) {
    fprintf(stderr, "*** Error: Device not found\n");
    return -1;
  }

  bool res = false;

  if (strcmp(command, "oled_write") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Usage %s oled_write str\n", argv[0]);
    } else {
      char *str = argv[2];
      res = oled_write(device, str);
    }
  }

  if (strcmp(command, "rgblight_color") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Usage %s rgblight_color hue sat\n", argv[0]);
    } else {
      uint16_t hue;
      uint8_t sat;

      if (sscanf(argv[2], "%" SCNu16 "", &hue) == 1 &&
          sscanf(argv[3], "%" SCNu8 "", &sat) == 1) {
        if (hue >= 0 && hue <= 360 && sat >= 0 && sat <= 255) {
          res = set_rgblight_to(device, hue, sat);
        } else {
          fprintf(stderr, "Invalid hue / sat\n");
        }
      } else {
        fprintf(stderr, "Invalid hue / sat\n");
      }
    }
  }

  if (strcmp(command, "bootloader") == 0) {
    res = send_no_args_message(device, id_bootloader_jump);
  }

  if (strcmp(command, "rgblight_reset") == 0) {
    res = send_no_args_message(device, id_rgblight_reset);
  }

  if (strcmp(command, "oled_clear") == 0) {
    res = send_no_args_message(device, id_oled_clear);
  }

  if (strcmp(command, "backlight_toggle") == 0) {
    res = send_no_args_message(device, id_backlight_toggle);
  }

  if (strcmp(command, "rgblight_toggle") == 0) {
    res = send_no_args_message(device, id_rgblight_toggle);
  }

  if (res != true) {
    return -1;
  }
}
