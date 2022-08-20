#ifndef FILE_PARSER_H
#define FILE_PARSER_H
#include <Arduino.h>
#include "structures.h"

void file_parser_init(uint8_t max_size);
uint8_t parse_custom_files(custom_file_contents_t* custom_file_contents, publish_object_t* results);

#endif
