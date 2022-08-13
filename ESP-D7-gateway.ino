#include "WiFi.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <Dns.h>

#include "WiFi_interface.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO

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
#define MAX_MQTT_LENGTH 20

#define DEFAULT_MQTT_PORT 1883

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

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
const IPAddress public_dns_ip(8, 8, 8, 8);
DNSClient dns_client;

static bool valid_mqtt_broker = false;
static bool mqtt_auth = false;

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

static uint32_t mqtt_port;

static unsigned char serial_buffer[MAX_SERIAL_BUFFER_SIZE];
static uint8_t serial_index_start = 0;
static uint8_t serial_index_end = 0;

static bool header_parsed;
static uint8_t payload_length;
static uint8_t packet_type;

static uint8_t current_uid[8];

static uint16_t last_voltage;
static bool last_state;
static char raw_file_string[MAX_CHAR_SIZE*2];
static char file_uid_string[MAX_CHAR_SIZE];
static char homeassistant_component[MAX_CHAR_SIZE];

#define MAGIC_NUMBER 241

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
  uint8_t mqtt_port_length = 4;
  EEPROM.write(offset, mqtt_port_length);
  offset += 1;
  for(int i = 0; i < mqtt_port_length; i++) {
    uint8_t* port_buf = (uint8_t*) &mqtt_port;
    EEPROM.write(offset, port_buf[i]);
    offset += 1;
  }
  EEPROM.commit();
}

void init_credentials_eeprom()
{
  int offset = 0;
  if(EEPROM.read(offset) != MAGIC_NUMBER) {
    DPRINTLN("first byte is not magic number, broadcasting for credentials");
    ssid_length = 0;
    password_length = 0;
    mqtt_broker_length = 0;
    mqtt_user_length = 0;
    mqtt_password_length = 0;
    mqtt_port = DEFAULT_MQTT_PORT;
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

  uint8_t mqtt_port_length = EEPROM.read(offset);
  offset += 1;
  for(int i = 0; i < mqtt_port_length; i++) {
    uint8_t* mqtt_port_ptr = (uint8_t*) &mqtt_port;
    mqtt_port_ptr[i] = EEPROM.read(offset);
    offset += 1;
  }

  valid_mqtt_broker = (mqtt_broker_length > 0);
}

void setup() 
{
  // the esp should report busy as long as it does not have an internet connection
//  pinMode(ESP_BUSY_PIN, OUTPUT);
//  digitalWrite(ESP_BUSY_PIN, HIGH);

  DBEGIN(115200);
  
  DATABEGIN(115200);

  EEPROM.begin(612);

  WiFi_init(ssid);

  server.begin();
  MDNS.begin("Dash7-gateway");

  init_credentials_eeprom();
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/change", HTTP_POST, handlePost);
  server.onNotFound(handleRoot);

  mqtt_client.setCallback(downlink);

  DPRINTLN("setup complete");
}

void set_mqtt_broker_address() {  
//  also allow public addresses
  if(valid_mqtt_broker) {
    IPAddress serverIp = MDNS.queryHost(mqtt_broker_string);
    if(serverIp.toString().equals("0.0.0.0")) {
      IPAddress public_ip;
      if(WiFi.hostByName(mqtt_broker_string, public_ip)) {
        mqtt_client.setServer(public_ip, mqtt_port);
      } else {
        DPRINTLN("No IP found");
        mqtt_client.setServer(mqtt_broker_string, mqtt_port);
      }
    } else
      mqtt_client.setServer(serverIp, mqtt_port);
  }
}

bool check_mqtt_connection() {
  if(!mqtt_client.connected()) {
    set_mqtt_broker_address();
    if(mqtt_client.connect("Dash7-Gateway", mqtt_user_string, mqtt_password_string)) {
      //reconnect on subscriptions
      DPRINTLN("connected to MQTT");
      return true;
    } else
      return false;
  } else
    return true;
}

void loop()
{
  if(WiFi_connect(client_ssid_string, ssid_length, client_password_string, password_length)) {
    if(valid_mqtt_broker && check_mqtt_connection()) {
//      digitalWrite(ESP_BUSY_PIN, LOW);
      while(DATAREADY()) {
        serial_buffer[serial_index_end] = DATAREAD();
        serial_index_end++;
      }
      serial_parse();
      mqtt_client.loop();
    }
  }
  server.handleClient();
}

void downlink(char* topic, byte* message, unsigned int length) {
  
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
        DPRINT("not header material ");
        DPRINTLN(serial_buffer[serial_index_start]);
        serial_index_start++;
      }
    }
  } else {
    if(get_serial_size() >= payload_length) {
      switch(packet_type){
        case 5: //reboot
          DPRINT("Modem rebooted with reason ");
          DPRINTLN(serial_buffer[serial_index_start]);
          serial_index_start += 1;
          break;
        default: //alp
//          digitalWrite(ESP_BUSY_PIN, HIGH);
          alp_parse();
          uint8_t empty_array[8] = {0, 0, 0, 0, 0, 0, 0, 0};
          if(memcmp(current_uid, empty_array, 8)) {
            create_and_send_json();
            memcpy(current_uid, empty_array, 8);
          }
//          digitalWrite(ESP_BUSY_PIN, LOW);
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
          DPRINT("current id ");
          DPRINT(current_uid[0], HEX);
          DPRINT(current_uid[1], HEX);
          DPRINT(current_uid[2], HEX);
          DPRINT(current_uid[3], HEX);
          DPRINT(current_uid[4], HEX);
          DPRINT(current_uid[5], HEX);
          DPRINT(current_uid[6], HEX);
          DPRINTLN(current_uid[7], HEX);
          
          payload_length -= 3 + length;
          break;
        }
      default: //not implemented alp
        DPRINTLN("unknown alp command, skipping message");
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
      last_voltage = button_file.battery_voltage;
      last_state = (bool) ((1 << button_file.button_id) & button_file.buttons_state);
      // indicate that last_state should be used
      raw_file_string[0] = 0;
      sprintf(file_uid_string, "_button%d", button_file.button_id);
      sprintf(homeassistant_component, "binary_sensor");
      DPRINT("button file: button id ");
      DPRINT(button_file.button_id);
      DPRINT(", buttons state ");
      DPRINTLN(button_file.buttons_state);
      break;
    case FILE_ID_PIR:;
      pir_file_t pir_file;
      memcpy_serial_overflow(pir_file.bytes, length);
      last_voltage = pir_file.battery_voltage;
      last_state = pir_file.state;
      // indicate that last_state should be used
      raw_file_string[0] = 0;
      sprintf(homeassistant_component, "binary_sensor");
      sprintf(file_uid_string, "_pir");
      break;
    default:;
      uint8_t raw_data[length];
      memcpy_serial_overflow(raw_data, length);
      sprintf(raw_file_string, "");
      for(int i = 0; i < length; i++) {
        sprintf(raw_file_string, "%s%02X", raw_file_string, raw_data[i]);
      }
      sprintf(file_uid_string, "_unknown");
      sprintf(homeassistant_component, "sensor");
      break;
  }
}

static void create_and_send_json()
{  
  static char device_uid[20];
  static char device_string[MAX_CHAR_SIZE*2];
  static char unique_id[32];
  static char state_topic[MAX_CHAR_SIZE];
  static char state_string[MAX_CHAR_SIZE];
  static char config_topic[MAX_CHAR_SIZE];
  static char config_json[800];
  sprintf(device_uid, "%02X%02X%02X%02X%02X%02X%02X%02X", current_uid[0], current_uid[1], current_uid[2], current_uid[3], current_uid[4], current_uid[5], current_uid[6], current_uid[7]);
  sprintf(device_string, "\"manufacturer\":\"LiQuiBit\",\"name\":\"Push7_%s\",\"identifiers\":[\"%s\"],\"model\":\"Push7\"", device_uid, device_uid);
  sprintf(unique_id, "%s%s", device_uid, file_uid_string);
  sprintf(state_topic, "homeassistant/%s/%s/state", homeassistant_component, unique_id);
  sprintf(config_topic, "homeassistant/%s/%s/config", homeassistant_component, unique_id);
  sprintf(config_json, "{\"device\":{%s},\"name\":\"%s\",\"qos\":1,\"unique_id\":\"%s\",\"state_topic\":\"%s\"}", 
    device_string, unique_id, unique_id, state_topic);
  sprintf(state_string, "%s", last_state ? "ON" : "OFF");

  if(!mqtt_client.beginPublish(config_topic, strlen(config_json), true)) {
    DPRINTLN("begin publish went wrong, abort");
    return;
  }
  for(uint16_t i = 0; i < strlen(config_json); i += MAX_MQTT_LENGTH) {
    uint16_t remaining_length = strlen(config_json) - i;
    mqtt_client.write((const uint8_t*)&config_json[i], remaining_length < MAX_MQTT_LENGTH ? remaining_length : MAX_MQTT_LENGTH);
  }
  if(!mqtt_client.endPublish()) {
    DPRINTLN("end publish went wrong, abort");
    return;
  }
  // raw file should be sent as string and returns directly, it has no idea of contents so can't send voltage
  if(raw_file_string[0]) {
    if(!mqtt_client.beginPublish(raw_file_string, strlen(raw_file_string), true)) {
      DPRINTLN("begin publish went wrong, abort");
      return;
    }
    for(uint16_t i = 0; i < strlen(raw_file_string); i += MAX_MQTT_LENGTH) {
      uint16_t remaining_length = strlen(raw_file_string) - i;
      mqtt_client.write((const uint8_t*)&raw_file_string[i], remaining_length < MAX_MQTT_LENGTH ? remaining_length : MAX_MQTT_LENGTH);
    }
    if(!mqtt_client.endPublish()) {
      DPRINTLN("end publish went wrong, abort");
      return;
    }
    DPRINTLN("publish of raw succeeded");
    return;
  }
  
  if(!mqtt_client.publish(state_topic, state_string, true)) {
    DPRINTLN("publish of state binary sensor failed");
    return;
  }
  DPRINTLN("config and state of file sent");

  /*
  DPRINT("config topic: ");
  DPRINTLN(config_topic);
  DPRINTLN(config_json);
  DPRINT("state : ");
  DPRINTLN(state_topic);
  if(raw_file_string[0])
    DPRINTLN(raw_file_string);
  else
    DPRINTLN(state_string);
  */


  sprintf(unique_id, "%s_voltage", device_uid);
  sprintf(state_topic, "homeassistant/%s/%s/state", "sensor", unique_id);
  sprintf(config_topic, "homeassistant/%s/%s/config", "sensor", unique_id);
  sprintf(config_json, "{\"device\":{%s},\"name\":\"voltage\",\"qos\":1,\"unique_id\":\"%s\",\"entity_category\":\"diagnostic\",\"state_topic\":\"%s\",\"state_class\":\"measurement\",\"unit_of_measurement\":\"mV\",\"icon\":\"mdi:sine-wave\"}", 
    device_string, unique_id, state_topic);
  sprintf(state_string, "%d", last_voltage);

  if(!mqtt_client.beginPublish(config_topic, strlen(config_json), true)) {
    DPRINTLN("begin publish went wrong, abort");
    return;
  }
  for(uint16_t i = 0; i < strlen(config_json); i += MAX_MQTT_LENGTH) {
    uint16_t remaining_length = strlen(config_json) - i;
    mqtt_client.write((const uint8_t*)&config_json[i], remaining_length < MAX_MQTT_LENGTH ? remaining_length : MAX_MQTT_LENGTH);
  }
  if(!mqtt_client.endPublish()) {
    DPRINTLN("end publish went wrong, abort");
    return;
  }
  if(!mqtt_client.publish(state_topic, state_string, true)) {
    DPRINTLN("publish of state voltage failed");
    return;
  }
  DPRINTLN("config and state of voltage sent");
  

/*  
  DPRINT("config topic 2 : ");
  DPRINTLN(config_topic);
  DPRINTLN(config_json);
  DPRINT("state : ");
  DPRINTLN(state_topic);
  DPRINTLN(state_string);
  */
}

const String first = "<head>" \
    "<style type=\"text/css\">" \
      "#container {" \
        "margin: 0;" \
        "position: absolute;" \
        "top: 25%;" \
        "left: 50%;" \
        "transform: translate(-50%, -25%);" \
        "width:400px;" \
        "height:600px;" \
      "}" \
      "h1{" \
        "text-align: center;" \
        "padding:10px;" \
      "}" \
      "label, input{" \
        "display:block;" \
        "width:100%;" \
      "}" \
      "input {" \
        "margin-bottom: 2em;" \
      "}" \
    "</style>" \
  "</head>" \
  "<body>" \
    "<div id=\"container\">" \
    "<h1>IoWay</h1>" \
    "<form action=\"change\" method=\"POST\">";
const String postedString = "<p style=\"color: red;\">Command sent to server!</p>";
const String WiFiCredentialsString = "<div>" \
        "<label for=\"SSID\">Wi-Fi SSID</label>" \
        "<input type=\"text\" name=\"SSID\" />" \
      "</div>" \
      "<div>" \
        "<label for=\"password\">Password</label>" \
        "<input type=\"password\" name=\"password\" />" \
      "</div>";
const String mqttBrokerString = "<div>" \
        "<label for=\"broker\">MQTT Broker</label>" \
        "<input type=\"text\" name=\"broker\" value=\"";
const String mqttUserString = "\" />" \
      "</div>" \
      "<div>" \
        "<label for=\"user\">MQTT User</label>" \
        "<input type=\"text\" name=\"user\" value=\"";
const String mqttPasswordString = "\"/>" \
      "</div>" \
      "<div>" \
        "<label for=\"mqttPassword\">MQTT Password</label>" \
        "<input type=\"password\" name=\"mqttPassword\" value=\"";
const String mqttPortString = "\"/>" \
      "</div>" \
      "<div>" \
        "<label for=\"mqttPort\">MQTT Port</label>" \
        "<input type=\"number\" name=\"mqttPort\" value=\"";
const String mqttSubmitString = "\" />" \
      "</div>" \
      "<div>" \
        "<input type=\"submit\" value=\"submit\" />" \
      "</div>" \
    "</form></div>" \
  "</body>";


// const String first = "<form action=\"change\" method=\"POST\"><div><table style=\"width: 100%;\">";
// const String postedString = "<tr><th colspan=\"3\" style=\"color: red\">Command sent to server!</th></tr>";
// const String WiFiCredentialsString = "<tr><th colspan=\"3\">Wi-Fi SSID</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"SSID\"></th></tr>"
//                                      "<tr><th colspan=\"3\">Password</th></tr><tr><th colspan=\"3\"><input type=\"password\" name=\"password\"></th></tr><tr></tr>";
// const String mqttBrokerString = "<tr><th colspan=\"3\">MQTT Broker</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"broker\" value=\"";
// const String mqttUserString = "\"</th></tr><tr><th colspan=\"3\">MQTT User</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"user\" value=\"";
// const String mqttPasswordString = "\"</th></tr><tr><th colspan=\"3\">MQTT Password</th></tr><tr><th colspan=\"3\"><input type=\"password\" name=\"mqttPassword\" value=\"";
// const String mqttPortString = "\"</th></tr><tr><th colspan=\"3\">MQTT Port</th></tr><tr><th colspan=\"3\"><input type=\"number\" name=\"mqttPort\" value=\"";
// const String mqttSubmitString = "\"</th></tr><tr><th colspan=\"3\"><input type=\"submit\" value=\"submit\"></th></tr></table></div></form>";

void handleRoot() {
  String htmlPage = first; 
  if(posted)
    htmlPage += postedString;
  htmlPage += WiFiCredentialsString;
  htmlPage += mqttBrokerString;
  htmlPage += mqtt_broker_string;
  htmlPage += mqttUserString;
  // we could fill in user and password up front so users can make easy changes. This will, however, send them in plaintext and thus expose them to the network
  //htmlPage += mqtt_user_string;
  htmlPage += mqttPasswordString;
  //htmlPage += mqtt_password_string;
  htmlPage += mqttPortString;
  htmlPage += mqtt_port;
  htmlPage += mqttSubmitString;

  server.send(200, "text/html", htmlPage);

  posted = false;
}

void handlePost() {
  if(server.hasArg("SSID")) {
    String ssid = server.arg("SSID");
    if(ssid.length()) {
      ssid_length = ssid.length();
      ssid.toCharArray(client_ssid_string,ssid_length+1); 
    }
  }
  if(server.hasArg("password")) {
    String pw = server.arg("password");
    if(pw.length()) {
      password_length = pw.length();
      pw.toCharArray(client_password_string,password_length+1); 
    }
  }
  if(server.hasArg("broker")) {
    String broker = server.arg("broker");
    if(broker.length()){
      mqtt_broker_length = broker.length();
      broker.toCharArray(mqtt_broker_string,mqtt_broker_length+1); 
    }
  }
  if(server.hasArg("user")) {
    String user = server.arg("user");
    if(user.length()){
      mqtt_user_length = user.length();
      user.toCharArray(mqtt_user_string,mqtt_user_length+1); 
    }
  }
  if(server.hasArg("mqttPassword")) {
    String pw = server.arg("mqttPassword");
    mqtt_password_length = pw.length();
    pw.toCharArray(mqtt_password_string,mqtt_password_length+1); 
  }
  if(server.hasArg("mqttPort")) {
    mqtt_port = strtoul(server.arg("mqttPort").c_str(), NULL, 10);
  }
  
  
  valid_mqtt_broker = (mqtt_broker_length > 0);
  set_mqtt_broker_address();

  write_credentials_to_eeprom();

  posted = true;
  handleRoot();
}
