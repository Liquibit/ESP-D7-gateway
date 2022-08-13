#ifndef WIFI_INTERFACE_H
#define WIFI_INTERFACE_H
#include <Arduino.h>

void WiFi_init(char* access_point_ssid);

bool WiFi_connect(char* ssid, int ssid_length, char* password, int password_length);

#endif
