#include "filesystem.h"
#include <EEPROM.h>

#define MAGIC_NUMBER 240

#define DEFAULT_MQTT_PORT 1883

void filesystem_init(int size) {
  EEPROM.begin(size);
}

void filesystem_read(persisted_data_t data) {
  int offset = 0;
  if(EEPROM.read(offset) != MAGIC_NUMBER) {
    DPRINTLN("first byte is not magic number, broadcasting for credentials");
    *data.wifi_ssid.length = 0;
    *data.wifi_password.length = 0;
    *data.mqtt_broker.length = 0;
    *data.mqtt_user.length = 0;
    *data.mqtt_password.length = 0;
    *data.mqtt_port = DEFAULT_MQTT_PORT;
    return;
  }
  offset += 1;

  *data.wifi_ssid.length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < *data.wifi_ssid.length; i++) {
    data.wifi_ssid.content[i] = EEPROM.read(offset);
    offset += 1;
  }

  *data.wifi_password.length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < *data.wifi_password.length; i++) {
    data.wifi_password.content[i] = EEPROM.read(offset);
    offset += 1;
  }

  *data.mqtt_broker.length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < *data.mqtt_broker.length; i++) {
    data.mqtt_broker.content[i] = EEPROM.read(offset);
    offset += 1;
  }

  *data.mqtt_user.length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < *data.mqtt_user.length; i++) {
    data.mqtt_user.content[i] = EEPROM.read(offset);
    offset += 1;
  }

  *data.mqtt_password.length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < *data.mqtt_password.length; i++) {
    data.mqtt_password.content[i] = EEPROM.read(offset);
    offset += 1;
  }

  uint8_t mqtt_port_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < mqtt_port_length; i++) {
    uint8_t* mqtt_port_ptr = (uint8_t*) data.mqtt_port;
    mqtt_port_ptr[i] = EEPROM.read(offset);
    offset += 1;
  }
}

void filesystem_write(persisted_data_t data) {
  int offset = 0;
  EEPROM.write(offset, MAGIC_NUMBER);
  offset += 1;
  EEPROM.write(offset, *data.wifi_ssid.length);
  offset += 1;
  for(int i = 0; i < *data.wifi_ssid.length; i++) {
    EEPROM.write(offset, data.wifi_ssid.content[i]);
    offset += 1;
  }
  EEPROM.write(offset, *data.wifi_password.length);
  offset += 1;
  for(int i = 0; i < *data.wifi_password.length; i++) {
    EEPROM.write(offset, data.wifi_password.content[i]);
    offset += 1;
  }
  EEPROM.write(offset, *data.mqtt_broker.length);
  offset += 1;
  for(int i = 0; i < *data.mqtt_broker.length; i++) {
    EEPROM.write(offset, data.mqtt_broker.content[i]);
    offset += 1;
  }
  EEPROM.write(offset, *data.mqtt_user.length);
  offset += 1;
  for(int i = 0; i < *data.mqtt_user.length; i++) {
    EEPROM.write(offset, data.mqtt_user.content[i]);
    offset += 1;
  }
  EEPROM.write(offset, *data.mqtt_password.length);
  offset += 1;
  for(int i = 0; i < *data.mqtt_password.length; i++) {
    EEPROM.write(offset, data.mqtt_password.content[i]);
    offset += 1;
  }
  uint8_t mqtt_port_length = 4;
  EEPROM.write(offset, mqtt_port_length);
  offset += 1;
  for(int i = 0; i < mqtt_port_length; i++) {
    uint8_t* port_buf = (uint8_t*) data.mqtt_port;
    EEPROM.write(offset, port_buf[i]);
    offset += 1;
  }
  EEPROM.commit();
}
