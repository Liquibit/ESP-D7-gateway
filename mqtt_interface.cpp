#include "mqtt_interface.h"
#include "WiFi_interface.h"
#include <PubSubClient.h>
#include <ESPmDNS.h>

#include <WebServer.h>

#define MAX_MQTT_LENGTH 20

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

static bool configuration_changed = true;

void downlink(char* topic, byte* message, unsigned int length) {
  
}

static bool update_configuration(persisted_data_t persisted_data) {
    if(!configuration_changed)
      return true;
  
    if(*persisted_data.mqtt_broker.length <= 0)
        return false;

    if(!WiFi_interface_is_connected())
      return false;

    mqtt_client.setCallback(downlink);
    
    IPAddress server_ip;
    server_ip = MDNS.queryHost(persisted_data.mqtt_broker.content);
    if(server_ip.toString().equals("0.0.0.0")) {
        if(!WiFi_get_ip_by_name(persisted_data.mqtt_broker.content, server_ip)) {
            DPRINTLN("No valid IP found for mqtt broker");
            return false;
        }
    }
    mqtt_client.setServer(server_ip, *persisted_data.mqtt_port);
    configuration_changed = false;
    return true;
}


bool mqtt_interface_config_changed(persisted_data_t persisted_data) {
    configuration_changed = true;
    return update_configuration(persisted_data);
}

bool mqtt_interface_connect(char* client_name, persisted_data_t persisted_data) {
    // already connected
    if(mqtt_client.connected())
        return true;

    // configuration valid
    if(!update_configuration(persisted_data))
        return false;

    // connect successfull
    if(!mqtt_client.connect(client_name, persisted_data.mqtt_user.content, persisted_data.mqtt_password.content))
        return false;

    //reconnect on subscriptions
    DPRINTLN("connected to MQTT");
    return true;
}

void mqtt_interface_handle() {
    mqtt_client.loop();
}

static bool publish_in_parts(char* topic, char* to_publish, uint32_t length) {
    if(length < MAX_MQTT_LENGTH) {
        if(!mqtt_client.publish(topic, to_publish, true)) {
            DPRINTLN("publish of single frame failed, abort");
            return false;
        }
        return true;
    }

    if(!mqtt_client.beginPublish(topic, length, true)) {
        DPRINTLN("begin publish went wrong, abort");
        return false;
    }
    for(uint16_t i = 0; i < length; i += MAX_MQTT_LENGTH) {
        uint16_t remaining_length = length - i;
        mqtt_client.write((const uint8_t*)&to_publish[i], remaining_length < MAX_MQTT_LENGTH ? remaining_length : MAX_MQTT_LENGTH);
    }
    if(!mqtt_client.endPublish()) {
        DPRINTLN("end publish went wrong, abort");
        return false;
    }
    return true;
}

void mqtt_interface_publish(publish_object_t* objects, uint8_t amount) {
    static char state_topic[100];
    static char config_topic[100];

    static char model_string[40];
    static char sw_version_string[30];
    static char device_string[200];

    static char category_string[50];
    static char device_class_string[50];
    static char icon_string[40];
    static char state_class_string[50];
    static char unit_string[30];
    static char config_json[900];

    for(uint8_t index = 0; index < amount; index++) {
        sprintf(state_topic, "homeassistant/%s/%s/state", objects[index].component, objects[index].object_id);
        sprintf(config_topic, "homeassistant/%s/%s/config", objects[index].component, objects[index].object_id);


        if(objects[index].model[0] != 0)
            sprintf(model_string, ",\"mdl\":\"%s\"", objects[index].model);
        else
            sprintf(model_string, "");
        
        if(objects[index].sw_version[0] != 0)
            sprintf(sw_version_string, ",\"sw\":\"%s\"", objects[index].sw_version);
        else
            sprintf(sw_version_string, "");
        
        sprintf(device_string, "\"mf\":\"LiQuiBit\",\"name\":\"Push7_%s\",\"ids\":[\"%s\"]%s%s", objects[index].uid, objects[index].uid, model_string, sw_version_string);


        if(objects[index].category[0] != 0)
            sprintf(category_string, ",\"ent_cat\":\"%s\"", objects[index].category);
        else
            sprintf(category_string, "");

        if(objects[index].device_class[0] != 0)
            sprintf(device_class_string, ",\"dev_cla\":\"%s\"", objects[index].device_class);
        else
            sprintf(device_class_string, "");

        if(objects[index].icon[0] != 0)
            sprintf(icon_string, ",\"ic\":\"%s\"", objects[index].icon);
        else
            sprintf(icon_string, "");

        if(objects[index].state_class[0] != 0)
            sprintf(state_class_string, ",\"stat_cla\":\"%s\"", objects[index].state_class);
        else
            sprintf(state_class_string, "");

        if(objects[index].unit[0] != 0)
            sprintf(unit_string, ",\"unit_of_meas\":\"%s\"", objects[index].unit);
        else
            sprintf(unit_string, "");
        
        sprintf(config_json, "{\"dev\":{%s},\"name\":\"%s\",\"qos\":1,\"uniq_id\":\"%s\",\"obj_id\":\"%s\",\"enabled_by_default\":%s,\"stat_t\":\"%s\"%s%s%s%s%s}", 
    device_string, objects[index].name, objects[index].object_id, objects[index].object_id, objects[index].default_shown ? "true" : "false", 
    state_topic, category_string, device_class_string, icon_string, state_class_string, unit_string);

        DPRINTLN(config_json);
        DPRINTLN(objects[index].state);

        if(publish_in_parts(config_topic, config_json, strlen(config_json)))
            publish_in_parts(state_topic, objects[index].state, strlen(objects[index].state));
    }
}
