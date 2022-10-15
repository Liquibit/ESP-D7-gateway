#include "structures.h"
#include "WiFi_interface.h"
#include "d7_webserver.h"
#include "filesystem.h"
#include "serial_interface.h"
#include "alp.h"
#include "file_parser.h"
#include "mqtt_interface.h"
#include <esp_task_wdt.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#define ESP_BUSY_PIN 13

#define FILESYSTEM_SIZE 612

#define MAX_CREDENTIAL_SIZE 100
#define MAX_SERIAL_BUFFER_SIZE 256
#define MAX_CUSTOM_FILES 2
#define MAX_PUBLISH_OBJECTS 12

#define GATEWAY_STATUS_INTERVAL 60

// 60 seconds timeout WDT
#define WDT_TIMEOUT 60

const char* ssid = "Dash7-gateway";

static char mac_id_string[20];

static char client_ssid_string[MAX_CREDENTIAL_SIZE];
int ssid_length;

static char client_password_string[MAX_CREDENTIAL_SIZE];
int password_length;

static char mqtt_broker_string[MAX_CREDENTIAL_SIZE];
int mqtt_broker_length;

static char mqtt_user_string[MAX_CREDENTIAL_SIZE];
int mqtt_user_length;

static char mqtt_password_string[MAX_CREDENTIAL_SIZE];
int mqtt_password_length;

static char mqtt_client_string[30];

static uint32_t mqtt_port;

static unsigned long previous_trigger = 60000;

static uint8_t serial_output_buffer[MAX_SERIAL_BUFFER_SIZE];
static custom_file_contents_t custom_files[MAX_CUSTOM_FILES];
static publish_object_t results[MAX_PUBLISH_OBJECTS];

static persisted_data_t linked_data = (persisted_data_t){
  .wifi_ssid = { .length = &ssid_length, .content = client_ssid_string },
  .wifi_password = { .length = &password_length, .content = client_password_string },
  .mqtt_broker = { .length = &mqtt_broker_length, .content = mqtt_broker_string },
  .mqtt_user = { .length = &mqtt_user_length, .content = mqtt_user_string },
  .mqtt_password = { .length = &mqtt_password_length, .content = mqtt_password_string },
  .mqtt_port = &mqtt_port,
};

static gateway_status_file_t status_file;

static void connection_details_changed() {
  mqtt_interface_config_changed(linked_data);

  filesystem_write(linked_data);
}

static void modem_rebooted(uint8_t reason) {
  DPRINT("Modem rebooted with reason ");
  DPRINTLN(reason);
  status_file.dash7_modem_reboot_counter++;
}

void setup() 
{
  DBEGIN(115200);

  esp_task_wdt_init(WDT_TIMEOUT, true); // set timeout and indicate hardware reset on timeout
  esp_task_wdt_add(NULL); // add current thread to WDT watch

  uint64_t MAC = ESP.getEfuseMac();
  uint8_t* MAC_ptr = (uint8_t*) &MAC;
  sprintf(mac_id_string, "%02x%02x%02x%02x%02x%02x", MAC_ptr[5], MAC_ptr[4], MAC_ptr[3], MAC_ptr[2], MAC_ptr[1], MAC_ptr[0]);
  sprintf(mqtt_client_string, "Dash7-gateway-%s", mac_id_string);

  serial_interface_init(&modem_rebooted, serial_output_buffer);
  alp_init(custom_files, MAX_CUSTOM_FILES);
  file_parser_init(MAX_PUBLISH_OBJECTS);

  filesystem_init(FILESYSTEM_SIZE);
  filesystem_read(linked_data);

  WiFi_init(ssid);
  webserver_init(ssid, &connection_details_changed, linked_data);

  status_file.heartbeat_counter = 0;
  status_file.processed_messages = 0;
  status_file.dash7_modem_reboot_counter = 0;
  status_file.status_interval = GATEWAY_STATUS_INTERVAL;
  status_file.rebooted = true;

  esp_task_wdt_reset();

  DPRINTLN("setup complete");
}

static void parse_and_publish(custom_file_contents_t* custom_file_content) {
  uint8_t number_of_publish_results = parse_custom_files(custom_file_content, results);
  mqtt_interface_publish(results, number_of_publish_results);
}

static void gateway_status_triggered() {
  custom_files[0] = (custom_file_contents_t){
    .file_id = GATEWAY_STATUS_FILE_ID,
    .length = 5,
    .offset = 0,
    .chip_id = ESP.getEfuseMac()
  };
  memcpy(custom_files[0].buffer, status_file.bytes, 5);

  parse_and_publish(&custom_files[0]);

  status_file.rebooted = false;
  status_file.processed_messages = 0;
  status_file.heartbeat_counter = (status_file.heartbeat_counter == 0xFF ? 1 : status_file.heartbeat_counter + 1);
}

void loop()
{
  if(WiFi_connect(client_ssid_string, ssid_length, client_password_string, password_length)) {
    if(mqtt_interface_connect(mqtt_client_string, linked_data)) {
      serial_handle();
      uint8_t serial_payload_length = serial_parse();
      if(serial_payload_length) {
        uint8_t number_of_custom_files_parsed = alp_parse(serial_output_buffer, serial_payload_length);
        if(number_of_custom_files_parsed) {
            status_file.processed_messages += number_of_custom_files_parsed;
            for(uint8_t index_custom_file = 0; index_custom_file < number_of_custom_files_parsed; index_custom_file++)
                parse_and_publish(&custom_files[index_custom_file]);
        }
      }
      if(millis() - previous_trigger > (GATEWAY_STATUS_INTERVAL * 1000)) {
        previous_trigger = millis();
        gateway_status_triggered();        
      }
      mqtt_interface_handle();
    }
  }
  webserver_handle();
  esp_task_wdt_reset();
}
