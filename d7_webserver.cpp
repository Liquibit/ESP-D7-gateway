#include "d7_webserver.h"

#include <WebServer.h>
#include <ESPmDNS.h>

WebServer server(80);

static bool posted = false;
static webserver_update_callback update_callback = NULL;

static persisted_data_t cached_data;

void handleRoot();
void handlePost();

void webserver_init(const char* mdns_hostname, webserver_update_callback callback, persisted_data_t data) {
  update_callback = callback;
  cached_data = data;
  
  server.begin();
  MDNS.begin(mdns_hostname);
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/change", HTTP_POST, handlePost);
  server.onNotFound(handleRoot);
}

void webserver_handle() {
  server.handleClient();
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

void handleRoot() {
  String htmlPage = first; 
  if(posted)
    htmlPage += postedString;
  htmlPage += WiFiCredentialsString;
  htmlPage += mqttBrokerString;
  htmlPage += cached_data.mqtt_broker.content;
  htmlPage += mqttUserString;
  // we could fill in user and password up front so users can make easy changes. This will, however, send them in plaintext and thus expose them to the network
  //htmlPage += mqtt_user_string;
  htmlPage += mqttPasswordString;
  //htmlPage += mqtt_password_string;
  htmlPage += mqttPortString;
  htmlPage += *cached_data.mqtt_port;
  htmlPage += mqttSubmitString;

  server.send(200, "text/html", htmlPage);

  posted = false;
}

void handlePost() {
  DPRINTLN("handle post");
  if(server.hasArg("SSID")) {
    String ssid = server.arg("SSID");
    if(ssid.length()) {
      DPRINTLN("gotten ssid, parsing");
      *cached_data.wifi_ssid.length = ssid.length();
      DPRINTLN(*cached_data.wifi_ssid.length);
      ssid.toCharArray(cached_data.wifi_ssid.content, ssid.length()+1); 
      DPRINTLN("got em");
    }
  }
  DPRINTLN("gotten ssid");
  if(server.hasArg("password")) {
    String pw = server.arg("password");
    if(pw.length()) {
      *cached_data.wifi_password.length = pw.length();
      pw.toCharArray(cached_data.wifi_password.content, pw.length()+1); 
    }
  }
  if(server.hasArg("broker")) {
    String broker = server.arg("broker");
    if(broker.length()){
      *cached_data.mqtt_broker.length = broker.length();
      broker.toCharArray(cached_data.mqtt_broker.content, broker.length()+1); 
    }
  }
  if(server.hasArg("user")) {
    String user = server.arg("user");
    if(user.length()){
      *cached_data.mqtt_user.length = user.length();
      user.toCharArray(cached_data.mqtt_user.content, user.length()+1); 
    }
  }
  if(server.hasArg("mqttPassword")) {
    String pw = server.arg("mqttPassword");
    *cached_data.mqtt_password.length = pw.length();
    pw.toCharArray(cached_data.mqtt_password.content, pw.length()+1); 
  }
  if(server.hasArg("mqttPort")) {
    *cached_data.mqtt_port = strtoul(server.arg("mqttPort").c_str(), NULL, 10);
  }

  if(update_callback)
    update_callback();

  posted = true;
  handleRoot();
}
