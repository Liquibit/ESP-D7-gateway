#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <Arduino.h>

//#define DBEGIN(...) Serial.begin(__VA_ARGS__)
#define DPRINT(...) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) Serial.println(__VA_ARGS__)
#define DBEGIN(...)
//#define DPRINT(...)
//#define DPRINTLN(...)

//#define DATAPRINT(...) Serial2.print(__VA_ARGS__)
//#define DATAPRINTLN(...) Serial2.println(__VA_ARGS__)
//#define DATAREAD(...) Serial2.read(__VA_ARGS__)
//#define DATAREADY(...) Serial2.available()
//#define DATABEGIN(...) Serial2.begin(__VA_ARGS__)
#define DATAPRINT(...) Serial.print(__VA_ARGS__)
#define DATAPRINTLN(...) Serial.println(__VA_ARGS__)
#define DATAREAD(...) Serial.read(__VA_ARGS__)
#define DATAREADY(...) Serial.available()
#define DATABEGIN(...) Serial.begin(__VA_ARGS__)
//#define DATAPRINT(...)
//#define DATAPRINTLN(...)
//#define DATAREAD(...)
//#define DATAREADY(...)
//#define DATABEGIN(...)

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
} persisted_data_t;

#endif
