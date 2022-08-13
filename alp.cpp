#include "alp.h"

#define MAX_CUSTOM_FILES 2
#define CUSTOM_FILE_EMPTY -1

#define ALP_OP_RETURN_FILE_DATA 0x20
#define ALP_OP_STATUS 0x22

static custom_file_parse_callback custom_file_cb;

static custom_file_contents_t custom_files[MAX_CUSTOM_FILES]; //limit to 2 custom files for now;
static uint8_t number_of_parsed_files = 0;

static uint8_t current_uid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t empty_uid[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static void reset_custom_files();
static void print_uid();

void alp_init(custom_file_parse_callback custom_file_parse_cb) {
  custom_file_cb = custom_file_parse_cb;

  reset_custom_files();
}

static void reset_custom_files() {
  for(uint8_t i = 0; i < MAX_CUSTOM_FILES; i++) {
    custom_files[i].file_id = CUSTOM_FILE_EMPTY;
  }
  number_of_parsed_files = 0;
}

static void save_custom_file(uint8_t* buffer, uint8_t file_id, uint8_t offset, uint8_t length) {
  for(uint8_t i = 0; i < MAX_CUSTOM_FILES; i++) {
    if(custom_files[i].file_id == CUSTOM_FILE_EMPTY) {
      custom_files[i].file_id = file_id;
      custom_files[i].length  = length;
      custom_files[i].offset  = offset;
      memcpy(custom_files[i].buffer, buffer, length - offset);
      number_of_parsed_files += 1;
      return;
    }
  }
  DPRINT("ERROR - could not put custom file in buffer");
}

void alp_parse(uint8_t* buffer, uint8_t payload_length) {
  uint8_t file_id;
  uint8_t offset;
  uint8_t length;
  uint8_t index = 0;
  while(payload_length > 0) {
    switch (buffer[index++] & 0x3F) {
      case ALP_OP_RETURN_FILE_DATA:
        file_id = buffer[index++];
        offset = buffer[index++];
        length = buffer[index++];
        save_custom_file(&buffer[index], file_id, offset, length);
        index += length;
        payload_length -= 4 + length;
        break;
      case ALP_OP_STATUS:
        {
          //field 1 is interface id
          index++;
          length = buffer[index++];
          index += 4;
          uint8_t linkbudget = buffer[index++];
          // uid is at index 12
          index += 7;
          memcpy(current_uid, buffer, 8);
          index += 8;
          print_uid();
          
          payload_length -= 3 + length;
          break;
        }
      default: //not implemented alp
        DPRINTLN("unknown alp command, skipping message");
        index += payload_length - 1;
        payload_length = 0;
        break;
    }
  }

  // after parsing, send files with uid to be parsed
  if(memcmp(current_uid, empty_uid, 8) && number_of_parsed_files) {
    for(uint8_t i = 0; i < number_of_parsed_files; i++) {
      memcpy(custom_files[i].uid, current_uid, 8);
      if(custom_file_cb)
        custom_file_cb(&custom_files[i]);
    }
  }
  reset_custom_files();
  memcpy(current_uid, empty_uid, 8);
}

static void print_uid() {
  DPRINT("current id ");
  DPRINT(current_uid[0], HEX);
  DPRINT(current_uid[1], HEX);
  DPRINT(current_uid[2], HEX);
  DPRINT(current_uid[3], HEX);
  DPRINT(current_uid[4], HEX);
  DPRINT(current_uid[5], HEX);
  DPRINT(current_uid[6], HEX);
  DPRINTLN(current_uid[7], HEX);
}
