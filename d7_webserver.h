#ifndef D7_WEBSERVER_H
#define D7_WEBSERVER_H
#include <Arduino.h>

typedef void (*webserver_update_callback)();

typedef struct {
  int* length;
  char* content;
} char_length_t;

typedef struct {
  char_length_t wifi_ssid;
  char_length_t wifi_password;
  char_length_t mqtt_broker;
  char_length_t mqtt_user;
  char_length_t mqtt_password;
  uint32_t* mqtt_port;
} webserver_data_t;

void webserver_init(const char* mdns_hostname, webserver_update_callback callback, webserver_data_t data);

void webserver_handle();

#endif
