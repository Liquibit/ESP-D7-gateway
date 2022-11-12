#include "serial_interface.h"
#include "CRC16.h"

#define DATARATE 115200

#define MODEM_HEADER_SIZE      7
#define MODEM_HEADER_SYNC_BYTE 0xC0
#define MODEM_HEADER_VERSION   0

#define SERIAL_MESSAGE_TYPE_ALP      1
#define SERIAL_MESSAGE_TYPE_REBOOTED 5

#define MAX_SERIAL_BUFFER_SIZE 256

static modem_rebooted_callback reboot_cb;

static uint8_t buffer[MAX_SERIAL_BUFFER_SIZE];
static uint8_t index_start = 0;
static uint8_t index_end = 0;

static bool header_parsed = false;
static uint8_t payload_length;
static uint8_t packet_type;
static uint16_t crc;

static uint8_t* output_buffer;

static CRC16 crc_tool;

static uint16_t get_serial_size();
static void memcpy_serial_overflow(uint8_t* dest, uint8_t length, uint8_t offset);

void serial_interface_init(modem_rebooted_callback reboot_callback, uint8_t* output_buffer_pointer) {
  DATABEGIN(DATARATE);
  reboot_cb = reboot_callback;
  output_buffer = output_buffer_pointer;

  crc_tool.setPolynome(0x1021);
  crc_tool.setStartXOR(0xFFFF);
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
        uint8_t counter = buffer[local_index++];
        packet_type = buffer[local_index++];
        payload_length = buffer[local_index++];
        crc = buffer[local_index++] << 8;
        crc += buffer[local_index++];
        header_parsed = true;
      } else {
        DPRINT("not header material ");
        DPRINTLN(buffer[index_start]);
        index_start++;
      }
    }
  } else {
    if(get_serial_size() >= payload_length) {
      uint8_t local_idx = index_start + MODEM_HEADER_SIZE;

      header_parsed = false;
      
      memcpy_serial_overflow(output_buffer, payload_length, MODEM_HEADER_SIZE);
      
      crc_tool.restart();
      crc_tool.add(output_buffer, payload_length);
      if(crc != crc_tool.getCRC()) {
        DPRINTLN("CRC did not match, skipping a byte");
        index_start++;
        return 0;
      }
      
      index_start += MODEM_HEADER_SIZE;
      switch(packet_type){
        case SERIAL_MESSAGE_TYPE_REBOOTED: //reboot
          if(reboot_cb)
            reboot_cb(buffer[index_start]);
          index_start += 1;
          break;
        case SERIAL_MESSAGE_TYPE_ALP:
        default:
          index_start += payload_length;
          return payload_length;
      }
    }
  }
  return 0;
}

void serial_send(uint8_t* data, uint8_t length, uint8_t type) {
  static uint8_t frame_counter = 0;
  uint8_t header[MODEM_HEADER_SIZE];

  crc_tool.restart();
  crc_tool.add(data, length);
  uint16_t calculated_crc = crc_tool.getCRC();

  header[0] = MODEM_HEADER_SYNC_BYTE;
  header[1] = MODEM_HEADER_VERSION;
  header[2] = frame_counter++;
  header[3] = type;
  header[4] = length;
  header[5] = calculated_crc >> 8;
  header[6] = calculated_crc & 0xFF;

  DATAWRITE(header, MODEM_HEADER_SIZE);
  DATAWRITE(data, length);
}

static void memcpy_serial_overflow(uint8_t* dest, uint8_t length, uint8_t offset)
{
  uint8_t local_index = index_start + offset;
  if(((local_index + length) & 0xFF) > local_index) {
    memcpy(dest, &buffer[local_index], length);
  } else {
    uint8_t end_length = MAX_SERIAL_BUFFER_SIZE - local_index;
    memcpy(dest, &buffer[local_index], end_length);
    memcpy(dest + end_length, buffer, length - end_length);
  }
}

static uint16_t get_serial_size()
{
  if(index_start > index_end)
    return (MAX_SERIAL_BUFFER_SIZE - index_start) + index_end;
  return index_end - index_start;
}
