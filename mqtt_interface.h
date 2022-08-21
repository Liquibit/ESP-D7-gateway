#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H
#include "structures.h"

bool mqtt_interface_config_changed(persisted_data_t persisted_data);

bool mqtt_interface_connect(char* client_name, persisted_data_t persisted_data);

void mqtt_interface_handle();

void mqtt_interface_publish(publish_object_t* objects, uint8_t amount);

#endif
