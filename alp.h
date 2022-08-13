#ifndef ALP_H
#define ALP_H
#include "structures.h"

typedef void (*custom_file_parse_callback)(custom_file_contents_t* file_contents);

void alp_init(custom_file_parse_callback custom_file_parse_cb);

void alp_parse(uint8_t* buffer, uint8_t payload_length);

#endif
