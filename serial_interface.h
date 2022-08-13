#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H
#include "structures.h"

typedef void (*modem_rebooted_callback)(uint8_t reason);
typedef void (*alp_handle_callback)(uint8_t* buffer, uint8_t length);

void serial_interface_init(modem_rebooted_callback reboot_callback, alp_handle_callback alp_callback);
void serial_handle();
void serial_parse();

#endif
