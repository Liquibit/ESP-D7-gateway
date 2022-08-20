#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H
#include "structures.h"

typedef void (*modem_rebooted_callback) (uint8_t);

void serial_interface_init(modem_rebooted_callback reboot_callback, uint8_t* output_buffer_pointer);
void serial_handle();
uint8_t serial_parse();

#endif
