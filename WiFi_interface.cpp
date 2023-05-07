#include "WiFi_interface.h"
#include <ETH.h>
#include <Dns.h>
#include "structures.h"

#define WIFI_TIMEOUT 20000
#define WIFI_DELAY_RETRY 500

static bool advertising;

const IPAddress public_dns_ip(8, 8, 8, 8);
DNSClient dns_client;

static bool first_try_connect = true;

static unsigned long next_try_timestamp = 0;

void WiFi_init(const char* access_point_ssid) {
  WiFi.mode(WIFI_MODE_APSTA);
  advertising = true;
  WiFi.disconnect();
  
  WiFi.softAP(access_point_ssid);

#if defined(ARDUINO_ESP32_POE)
  ETH.begin();
#endif
}

bool WiFi_connect(char* ssid, int ssid_length, char* password, int password_length) {
  if(WiFi.status() != WL_CONNECTED) {
    if(first_try_connect) {
      DPRINTLN("Wi-Fi disconnected, trying to reconnect");
      first_try_connect = false;
    }
    if(ssid_length == 0)
      return false;

    if(millis() < next_try_timestamp) // let it try before 
      return false;

    int n = WiFi.scanNetworks();
    if(n < 1)
      return false;
    bool found = false;
    for(int i = 0; i < n; i++) {
      String found_ssid = WiFi.SSID(i);
      int len = found_ssid.length();
      char found_ssid_char[len];
      found_ssid.toCharArray(found_ssid_char, len + 1);
      if(!memcmp(found_ssid_char, ssid, len)) {
        WiFi.begin(found_ssid_char, password);
        found = true;
        break;
      }
    }
    next_try_timestamp = millis() + WIFI_TIMEOUT;

    return false;
  } else {
    WiFi_advertising_disable();
    return true;  
  }
}

bool WiFi_interface_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFi_advertising_disable() {
  if(advertising) {
    DPRINTLN("Interface connected, disabling access point");
    WiFi.mode(WIFI_STA);
    dns_client.begin(public_dns_ip);
    first_try_connect = true;
    advertising = false;
  }
}

bool WiFi_get_ip_by_name(char* host, IPAddress* resulting_ip) {
  return WiFi.hostByName(host, *resulting_ip);
}
