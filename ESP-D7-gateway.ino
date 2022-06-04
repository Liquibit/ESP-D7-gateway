#include "WiFi.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>

#define ESP_BUSY_PIN 13

#define MODEM_HEADER_SIZE 7
#define MODEM_HEADER_SYNC_BYTE 0xC0
#define MODEM_HEADER_VERSION 0

#define ALP_OP_RETURN_FILE_DATA 0x20
#define ALP_OP_STATUS 0x22

#define FILE_ID_BUTTON 51
#define FILE_ID_PIR 53

#define MAX_CHAR_SIZE 100
#define MAX_SERIAL_BUFFER_SIZE 256

const char* ssid = "Dash7-gateway";

static char client_ssid_string[MAX_CHAR_SIZE];
int ssid_length;

static char client_password_string[MAX_CHAR_SIZE];
int password_length;

static char mqtt_broker_string[MAX_CHAR_SIZE];
int mqtt_broker_length;

static char mqtt_user_string[MAX_CHAR_SIZE];
int mqtt_user_length;

static char mqtt_password_string[MAX_CHAR_SIZE];
int mqtt_password_length;

static unsigned char serial_buffer[MAX_SERIAL_BUFFER_SIZE];
static uint8_t serial_index_start = 0;
static uint8_t serial_index_end = 0;

static bool header_parsed;
static uint8_t payload_length;
static uint8_t packet_type;

static uint8_t current_uid[8];

#define MAGIC_NUMBER 249

#define WIFI_TIMEOUT 20000
#define WIFI_DELAY_RETRY 500
WebServer server(80);
bool posted = false;

void handleRoot();
void handlePost();

typedef struct
{
    union
    {
        uint8_t bytes[6];
        struct
        {
            uint8_t button_id;
            uint8_t mask;
            uint8_t elapsed_deciseconds;
            uint8_t buttons_state;
            uint16_t battery_voltage;
        } __attribute__((__packed__));
    };
} button_file_t;

typedef struct
{
    union
    {
        uint8_t bytes[3];
        struct
        {
            bool state;
            uint16_t battery_voltage;
        } __attribute__((__packed__));
    };
} pir_file_t;

void write_credentials_to_eeprom()
{
  int offset = 0;
  EEPROM.write(offset, MAGIC_NUMBER);
  offset += 1;
  EEPROM.write(offset, ssid_length);
  offset += 1;
  for(int i = 0; i < ssid_length; i++) {
    EEPROM.write(offset, client_ssid_string[i]);
    offset += 1;
  }
  EEPROM.write(offset, password_length);
  offset += 1;
  for(int i = 0; i < password_length; i++) {
    EEPROM.write(offset, client_password_string[i]);
    offset += 1;
  }
  EEPROM.write(offset, mqtt_broker_length);
  offset += 1;
  for(int i = 0; i < mqtt_broker_length; i++) {
    EEPROM.write(offset, mqtt_broker_string[i]);
    offset += 1;
  }
  EEPROM.write(offset, mqtt_user_length);
  offset += 1;
  for(int i = 0; i < mqtt_user_length; i++) {
    EEPROM.write(offset, mqtt_user_string[i]);
    offset += 1;
  }
  EEPROM.write(offset, mqtt_password_length);
  offset += 1;
  for(int i = 0; i < mqtt_password_length; i++) {
    EEPROM.write(offset, mqtt_password_string[i]);
    offset += 1;
  }
  EEPROM.commit();
}

void init_credentials_eeprom()
{  
  int offset = 0;
  if(EEPROM.read(offset) != MAGIC_NUMBER) {
    Serial.println("first byte is not magic number, broadcasting for credentials");
    ssid_length = 0;
    password_length = 0;
    mqtt_broker_length = 0;
    mqtt_user_length = 0;
    mqtt_password_length = 0;
    return;
  }
  offset += 1;

  ssid_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < ssid_length; i++) {
    client_ssid_string[i] = EEPROM.read(offset);
    offset += 1;
  }

  password_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < password_length; i++) {
    client_password_string[i] = EEPROM.read(offset);
    offset += 1;
  }

  mqtt_broker_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < mqtt_broker_length; i++) {
    mqtt_broker_string[i] = EEPROM.read(offset);
    offset += 1;
  }

  mqtt_user_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < mqtt_user_length; i++) {
    mqtt_user_string[i] = EEPROM.read(offset);
    offset += 1;
  }

  mqtt_password_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < mqtt_password_length; i++) {
    mqtt_password_string[i] = EEPROM.read(offset);
    offset += 1;
  }
}

void setup() 
{
  // the esp should report busy as long as it does not have an internet connection
  pinMode(ESP_BUSY_PIN, OUTPUT);
  digitalWrite(ESP_BUSY_PIN, HIGH);

  Serial2.begin(115200);
  
  Serial.begin(115200);

  EEPROM.begin(512);
  
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.disconnect();

  WiFi.softAP(ssid);

  server.begin();
  MDNS.begin("gateway");

  init_credentials_eeprom();
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/change", HTTP_POST, handlePost);
  server.onNotFound(handleRoot);

  Serial.println("setup complete");
}

bool check_connection() {
  if(WiFi.status() != WL_CONNECTED) {
    if(ssid_length == 0) {
      return false;
    }
    int n = WiFi.scanNetworks();
    if(n < 1)
      return false;
    bool found = false;
    for(int i = 0; i < n; i++) {
      String found_ssid = WiFi.SSID(i);
      int len = found_ssid.length();
      char found_ssid_char[len];
      found_ssid.toCharArray(found_ssid_char, len + 1);
      if(!memcmp(found_ssid_char, client_ssid_string, len)) {
        WiFi.begin(found_ssid_char, client_password_string);
        found = true;
        break;
      }
    }
    if(!found)
      return false;

    int total_delay = 0;
    while((WiFi.status() != WL_CONNECTED) && (total_delay < WIFI_TIMEOUT)) {
      delay(WIFI_DELAY_RETRY);
      total_delay += WIFI_DELAY_RETRY;
    }
    Serial.println("connected to Wi-Fi");

    if(WiFi.status() == WL_CONNECTED) {
      WiFi.mode(WIFI_STA);
      return true;
    } else {
      return false;
    }
  } else
    return true;
}

void loop() 
{
  if(check_connection()) {
    digitalWrite(ESP_BUSY_PIN, LOW);
    while(Serial2.available()) {
      serial_buffer[serial_index_end] = Serial2.read();
      serial_index_end++;
    }
    serial_parse();
    // connected! start parsing and forwarding!
  }
  server.handleClient();
}

uint16_t get_serial_size()
{
  if(serial_index_start > serial_index_end)
    return (MAX_SERIAL_BUFFER_SIZE - serial_index_start) + serial_index_end;
  return serial_index_end - serial_index_start;
}

void serial_parse()
{
  if(!header_parsed) {
    if(get_serial_size() > MODEM_HEADER_SIZE) {
      // check sync byte and version, otherwise skip byte
      if((serial_buffer[serial_index_start++] == MODEM_HEADER_SYNC_BYTE) && (serial_buffer[serial_index_start++] == MODEM_HEADER_VERSION)) {
        serial_index_start++;
        packet_type = serial_buffer[serial_index_start++];
        payload_length = serial_buffer[serial_index_start++];
        header_parsed = true;
        serial_index_start += MODEM_HEADER_SIZE - 5;
      } else {
        Serial.print("not header material ");
        Serial.println(serial_buffer[serial_index_start]);
        serial_index_start++;
      }
    }
  } else {
    if(get_serial_size() >= payload_length) {
      switch(packet_type){
        case 5: //reboot
          Serial.print("Modem rebooted with reason ");
          Serial.println(serial_buffer[serial_index_start]);
          serial_index_start += 1;
          break;
        default: //alp
          digitalWrite(ESP_BUSY_PIN, HIGH);
          alp_parse();
          digitalWrite(ESP_BUSY_PIN, LOW);
          break;
      }
      header_parsed = false;
    }
  }
}

void memcpy_serial_overflow(uint8_t* dest, uint8_t length)
{
  if(((serial_index_start + length) & 0xFF) > serial_index_start) {
    memcpy(dest, &serial_buffer[serial_index_start], length);
  } else {
    uint8_t end_length = MAX_SERIAL_BUFFER_SIZE - serial_index_start;
    memcpy(dest, &serial_buffer[serial_index_start], end_length);
    memcpy(dest + end_length, serial_buffer, length - end_length);
  }
}

void alp_parse()
{
  uint8_t file_id;
  uint8_t offset;
  uint8_t length;
  while(payload_length > 0) {
    switch (serial_buffer[serial_index_start++] & 0x3F) {
      case ALP_OP_RETURN_FILE_DATA:
        file_id = serial_buffer[serial_index_start++];
        offset = serial_buffer[serial_index_start++];
        length = serial_buffer[serial_index_start++];
        parse_custom_files(file_id, offset, length);
        serial_index_start += length;
        payload_length -= 4 + length;
        break;
      case ALP_OP_STATUS:
        {
          //field 1 is interface id
          serial_index_start++;
          length = serial_buffer[serial_index_start++];
          serial_index_start += 4;
          uint8_t linkbudget = serial_buffer[serial_index_start++];
          // uid is at index 12
          serial_index_start += 7;
          memcpy_serial_overflow(current_uid, 8);
          serial_index_start += 8;
          Serial.print("current id ");
          Serial.print(current_uid[0], HEX);
          Serial.print(current_uid[1], HEX);
          Serial.print(current_uid[2], HEX);
          Serial.print(current_uid[3], HEX);
          Serial.print(current_uid[4], HEX);
          Serial.print(current_uid[5], HEX);
          Serial.print(current_uid[6], HEX);
          Serial.println(current_uid[7], HEX);
          
          payload_length -= 3 + length;
          break;
        }
      default: //not implemented alp
        Serial.println("unknown alp command, skipping message");
        serial_index_start += payload_length - 1; // alp command is already parsed
        payload_length = 0;
        break;
    }
  }  
}

void parse_custom_files(uint8_t file_id, uint8_t offset, uint8_t length)
{
  switch(file_id) {
    case FILE_ID_BUTTON:;
      button_file_t button_file;
      memcpy_serial_overflow(button_file.bytes, length);
      Serial.print("button file: button id ");
      Serial.print(button_file.button_id);
      Serial.print(", buttons state ");
      Serial.println(button_file.buttons_state);
      break;
    case FILE_ID_PIR:;
      pir_file_t pir_file;
      memcpy_serial_overflow(pir_file.bytes, length);
      break;
  }
}

const String first = "<form action=\"change\" method=\"POST\"><div><table width=\"100%\">";
const String postedString = "<tr><th colspan=\"3\" style=\"color: red\">Command sent to server!</th></tr>";
const String WiFiCredentialsString = "<tr><th colspan=\"3\">Wi-Fi SSID</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"SSID\"></th></tr>"
                                     "<tr><th colspan=\"3\">Password</th></tr><tr><th colspan=\"3\"><input type=\"password\" name=\"password\"></th></tr>"
                                     "<tr></tr><tr><th colspan=\"3\"><input type=\"submit\" value=\"submit Wi-Fi credentials\"></th></tr></table></div></form><tr></tr>";
const String mqttBrokerString = "<form action=\"change\" method=\"POST\"><div><table width=\"100%\"><tr><th colspan=\"3\">MQTT Broker</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"broker\" value=\"";
const String mqttUserString = "\"</th></tr><tr><th colspan=\"3\">MQTT User</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"user\" value=\"";
const String mqttPasswordString = "\"</th></tr><tr><th colspan=\"3\">MQTT Password</th></tr><tr><th colspan=\"3\"><input type=\"password\" name=\"mqttPassword\" value=\"";
const String mqttSubmitString = "\"</th></tr><tr><th colspan=\"3\"><input type=\"submit\" value=\"submit MQTT Details\"></th></tr></table></div></form>";

void handleRoot() {
  String htmlPage = first; 
  if(posted)
    htmlPage += postedString;
  htmlPage += WiFiCredentialsString;
  htmlPage += mqttBrokerString;
  htmlPage += mqtt_broker_string;
  htmlPage += mqttUserString;
  htmlPage += mqtt_user_string;
  htmlPage += mqttPasswordString;
  htmlPage += mqtt_password_string;
  htmlPage += mqttSubmitString;

  server.send(200, "text/html", htmlPage);

  posted = false;
}

void handlePost() {
  if(server.hasArg("SSID")) {
    String ssid = server.arg("SSID");
    ssid_length = ssid.length();
    ssid.toCharArray(client_ssid_string,ssid_length+1); 
  }
  if(server.hasArg("password")) {
    String pw = server.arg("password");
    password_length = pw.length();
    pw.toCharArray(client_password_string,password_length+1); 
  }
  if(server.hasArg("broker")) {
    String broker = server.arg("broker");
    mqtt_broker_length = broker.length();
    broker.toCharArray(mqtt_broker_string,mqtt_broker_length+1); 
  }
  if(server.hasArg("user")) {
    String user = server.arg("user");
    mqtt_user_length = user.length();
    user.toCharArray(mqtt_user_string,mqtt_user_length+1); 
  }
  if(server.hasArg("mqttPassword")) {
    String pw = server.arg("mqttPassword");
    mqtt_password_length = pw.length();
    pw.toCharArray(mqtt_password_string,mqtt_password_length+1); 
  }

  write_credentials_to_eeprom();

  posted = true;
  handleRoot();
}
