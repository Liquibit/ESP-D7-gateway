#include "WiFi.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>

#define MAX_CHAR_SIZE 100

const char* ssid = "Dash7-gateway";

static char client_ssid_string[MAX_CHAR_SIZE];
int ssid_length;

static char client_password_string[MAX_CHAR_SIZE];
int password_length;

#define MAGIC_NUMBER 251

#define WIFI_TIMEOUT 20000
#define WIFI_DELAY_RETRY 500
WebServer server(80);
bool posted = false;

void handleRoot();
void handlePost();

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
  EEPROM.commit();
}

void init_credentials_eeprom()
{  
  int offset = 0;
  if(EEPROM.read(offset) != MAGIC_NUMBER) {
    Serial.println("first byte is not magic number, broadcasting for credentials");
    ssid_length = 0;
    password_length = 0;
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
}

void setup() 
{
  Serial.begin(115200);

  EEPROM.begin(300);
  
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

    return WiFi.status() == WL_CONNECTED;
  } else
    return true;
}

void loop() 
{
  if(check_connection()) {
    // connected! start parsing and forwarding!
  } else {
    server.handleClient();
  }

  // insert data here
}

const String first = "<form action=\"change\" method=\"POST\"><div><table width=\"100%\">";
const String postedString = "<tr><th colspan=\"3\" style=\"color: red\">Command sent to server!</th></tr>";
const String WiFiCredentialsString = "<tr><th colspan=\"3\">Wi-Fi SSID</th></tr><tr><th colspan=\"3\"><input type=\"text\" name=\"SSID\"></th></tr>"
                                     "<tr><th colspan=\"3\">Password</th></tr><tr><th colspan=\"3\"><input type=\"password\" name=\"password\"></th></tr>"
                                     "<tr></tr><tr><th colspan=\"3\"><input type=\"submit\" value=\"submit\"></th></tr></table></div></form>";

void handleRoot() {
  String htmlPage = first; 
  if(posted)
    htmlPage += postedString;
  htmlPage += WiFiCredentialsString;

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

  if(ssid_length > 0)
   write_credentials_to_eeprom();

  posted = true;
  handleRoot();
}
