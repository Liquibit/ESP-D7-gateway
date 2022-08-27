#ifndef WIFI_INTERFACE_H
#define WIFI_INTERFACE_H

#include <Arduino.h>

void WiFi_init(const char* access_point_ssid);

bool WiFi_connect(char* ssid, int ssid_length, char* password, int password_length);

bool WiFi_interface_is_connected();

bool WiFi_get_ip_by_name(char* host, IPAddress* resulting_ip);

#endif
