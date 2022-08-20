#include "serial_interface.h"

#define DATARATE 115200

#define MODEM_HEADER_SIZE 7
#define MODEM_HEADER_SYNC_BYTE 0xC0
#define MODEM_HEADER_VERSION 0

#define MAX_SERIAL_BUFFER_SIZE 256

static modem_rebooted_callback reboot_cb;

static uint8_t buffer[MAX_SERIAL_BUFFER_SIZE];
static uint8_t index_start = 0;
static uint8_t index_end = 0;

static bool header_parsed = false;
static uint8_t payload_length;
static uint8_t packet_type;

static uint8_t* output_buffer;

static uint16_t get_serial_size();
static void memcpy_serial_overflow(uint8_t* dest, uint8_t length);

void serial_interface_init(modem_rebooted_callback reboot_callback, uint8_t* output_buffer_pointer) {
  DATABEGIN(DATARATE);
  reboot_cb = reboot_callback;
  output_buffer = output_buffer_pointer;
}

void serial_handle() {
  while(DATAREADY()) {
    buffer[index_end] = DATAREAD();
    index_end++;
  }
}

uint8_t serial_parse() {
  if(!header_parsed) {
    if(get_serial_size() > MODEM_HEADER_SIZE) {
      // check sync byte and version, otherwise skip byte
      uint8_t local_index = index_start;
      if((buffer[local_index++] == MODEM_HEADER_SYNC_BYTE) && (buffer[local_index++] == MODEM_HEADER_VERSION)) {
        local_index++;
        packet_type = buffer[local_index++];
        payload_length = buffer[local_index++];
        header_parsed = true;
        index_start += MODEM_HEADER_SIZE;
      } else {
        DPRINT("not header material ");
        DPRINTLN(buffer[index_start]);
        index_start++;
      }
    }
  } else {
    if(get_serial_size() >= payload_length) {
      header_parsed = false;
      switch(packet_type){
        case 5: //reboot
          if(reboot_cb)
            reboot_cb(buffer[index_start]);
          index_start += 1;
          break;
        default: //alp
          memcpy_serial_overflow(output_buffer, payload_length);
          index_start += payload_length;
          return payload_length;
      }
    }
  }
  return 0;
}

static void memcpy_serial_overflow(uint8_t* dest, uint8_t length)
{
  if(((index_start + length) & 0xFF) > index_start) {
    memcpy(dest, &buffer[index_start], length);
  } else {
    uint8_t end_length = MAX_SERIAL_BUFFER_SIZE - index_start;
    memcpy(dest, &buffer[index_start], end_length);
    memcpy(dest + end_length, buffer, length - end_length);
  }
}

static uint16_t get_serial_size()
{
  if(index_start > index_end)
    return (MAX_SERIAL_BUFFER_SIZE - index_start) + index_end;
  return index_end - index_start;
}
