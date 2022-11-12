#include "alp.h"

#define CUSTOM_FILE_EMPTY -1

#define ALP_OP_RETURN_FILE_DATA 0x20
#define ALP_OP_STATUS 0x22
#define ALP_OP_RESPONSE_TAG 0x23
#define ALP_OP_INDIRECT_FORWARD 0x33
#define ALP_OP_REQUEST_TAG 0x34

static uint8_t number_of_parsed_files = 0;

static uint8_t current_uid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t empty_uid[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static void print_uid();

static custom_file_contents_t* custom_files;
static uint8_t max_size;

void alp_init(custom_file_contents_t* custom_file_contents_buffer, uint8_t max_buffer_size) {
  custom_files = custom_file_contents_buffer;
  max_size = max_buffer_size;
}

static void reset_custom_files() {
  for(uint8_t i = 0; i < max_size; i++) {
    custom_files[i].file_id = CUSTOM_FILE_EMPTY;
  }
  number_of_parsed_files = 0;
}

static void save_custom_file(uint8_t* buffer, uint8_t file_id, uint8_t offset, uint8_t length) {
  for(uint8_t i = 0; i < max_size; i++) {
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

/**
 * @brief parse a length operand
 * @param buffer a buffer containing the length operand, starting at the first byte of this operand
 * @param length the variable where the length-result gets stored
 * @return the amount of bytes processed
 */
uint8_t alp_parse_length_operand(uint8_t* buffer, uint32_t* length) {
  uint8_t local_length;
  uint8_t length_of_field;
  uint8_t buffer_index = 0;
  
  local_length = buffer[buffer_index++];
  // the highest bits indicate the length of the length operator
  length_of_field = local_length >> 6 + 1;

  if(length_of_field == 1) {
    *length = (uint32_t)local_length;
    return length_of_field;
  }
  
  uint8_t field_index = length_of_field - 1;

  *length = (local_length & 0x3F) << ( 8 * field_index); // mask field length specificier bits and shift before adding other length bytes
  for(; field_index > 0; field_index--) {
    local_length = buffer[buffer_index++];
    *length += local_length << (8 * (field_index - 1));
  }
  return length_of_field;
}

uint8_t alp_parse(uint8_t* buffer, uint8_t payload_length) {
  uint8_t file_id;
  uint8_t offset;
  uint8_t length;
  uint8_t index = 0;
  uint8_t rssi = 0;

  //cleanup previous files
  reset_custom_files();
  memcpy(current_uid, empty_uid, 8);

  while(payload_length > 0) {
    switch (buffer[index++] & 0x3F) {
      case ALP_OP_RETURN_FILE_DATA:
        {
          file_id = buffer[index++];
          //TODO use alp_parse_length_operand instead
          offset = buffer[index++];
          length = buffer[index++];
          save_custom_file(&buffer[index], file_id, offset, length);
          index += length;
          payload_length -= 4 + length;
        }
        break;
      case ALP_OP_STATUS:
        {
          //field 1 is interface id
          index++;
          length = buffer[index++];
          index += 3;
          rssi = buffer[index++];
          uint8_t linkbudget = buffer[index++];
          // uid is at index 12
          index += 7;
          memcpy(current_uid, &buffer[index], 8);
          index += 8;
          print_uid();
          
          payload_length -= 3 + length;
          break;
        }
      case ALP_OP_RESPONSE_TAG:
        {
          uint8_t tag_id = buffer[index++];
          break;
        }
      default: //not implemented alp
        {
        DPRINTLN("unknown alp command, skipping message");
        index += payload_length - 1;
        payload_length = 0;
        }
        break;
    }
  }

  // after parsing, add uid and rssi to the files
  if(memcmp(current_uid, empty_uid, 8) && number_of_parsed_files) {
    for(uint8_t i = 0; i < number_of_parsed_files; i++) {
      memcpy(custom_files[i].uid, current_uid, 8);
      custom_files[i].rssi = rssi;
    }
  }

  return number_of_parsed_files;
}

/**
 * @brief append a length operand
 * @param alp_command a buffer containing the alp_command where the length operand has to be added
 * @param length the length that has to be appended
 * @return the amount of bytes added
 */
uint8_t alp_append_length_operand(uint8_t* alp_command, uint32_t length) {
  uint8_t size = 1;
  
  if(length < 64) {
    alp_command[0] = (uint8_t)length;
    return size;
  }

  size = 2;
  if(length > 0x3FFF)
    size = 3;
  if(length > 0x3FFFFF)
    size = 4;

  for(uint8_t index = 0; index < size; index++)
    alp_command[index] = (length >> (8 * (size - 1 - index))) & 0xFF;
  alp_command[0] += (size - 1) << 6;
  
  return size;
}

uint8_t alp_append_return_file_data(uint8_t* alp_command, uint8_t file_id, uint32_t offset, uint32_t length, uint8_t* data) {
  uint8_t index = 0;

  alp_command[index++] = ALP_OP_RETURN_FILE_DATA;
  alp_command[index++] = file_id;
  index += alp_append_length_operand(&alp_command[index], offset);
  index += alp_append_length_operand(&alp_command[index], length);
  memcpy(&alp_command[index], data, length);
  index += length;

  return index;
}

// overload of indirect forward not yet supported
uint8_t alp_append_indirect_forward(uint8_t* alp_command, uint8_t file_id) {
  uint8_t index = 0;
  
  alp_command[index++] = ALP_OP_INDIRECT_FORWARD;
  alp_command[index++] = file_id;

  return index;
}

uint8_t alp_append_tag_request(uint8_t* alp_command, uint8_t tag_id, bool always_answer) {
  uint8_t index = 0;

  alp_command[index++] = ALP_OP_REQUEST_TAG | (always_answer << 7);
  alp_command[index++] = tag_id;

  return index;
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
