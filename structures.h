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

typedef struct {
  int16_t file_id;
  uint8_t length;
  uint8_t offset;
  uint8_t buffer[255];
  uint8_t uid[8];
  uint8_t rssi;
} custom_file_contents_t;

typedef struct {
  char uid[17];
  char name[50];
  char object_id[50];
  char component[20];
  char category[20];
  char unit[5];
  char icon[20];
  char state_class[20];
  char device_class[20];
  char sw_version[4];
  char model[20];
  char state[20];
  bool default_shown;
} publish_object_t;

#endif
