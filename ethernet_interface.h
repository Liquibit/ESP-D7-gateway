#ifndef ETHERNET_INTERFACE_H
#define ETHERNET_INTERFACE_H

#if defined(ARDUINO_ESP32_POE)

#include <Arduino.h>

void ethernet_init();

bool ethernet_interface_is_connected();

#endif

#endif
