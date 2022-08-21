#ifndef ALP_H
#define ALP_H
#include "structures.h"

void alp_init(custom_file_contents_t* custom_file_contents_buffer, uint8_t max_buffer_size);

uint8_t alp_parse(uint8_t* buffer, uint8_t payload_length);

#endif
