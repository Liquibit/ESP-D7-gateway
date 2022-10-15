#include "WiFi_interface.h"
#include "WiFi.h"
#include <Dns.h>
#include "structures.h"

#define WIFI_TIMEOUT 20000
#define WIFI_DELAY_RETRY 500

const IPAddress public_dns_ip(8, 8, 8, 8);
DNSClient dns_client;

static bool first_try_connect = true;

void WiFi_init(const char* access_point_ssid) {
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.disconnect();
  
  WiFi.softAP(access_point_ssid);
}

bool WiFi_connect(char* ssid, int ssid_length, char* password, int password_length) {
  if(WiFi.status() != WL_CONNECTED) {
    if(first_try_connect) {
      DPRINTLN("Wi-Fi disconnected, trying to reconnect");
      first_try_connect = false;
    }
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
      if(!memcmp(found_ssid_char, ssid, len)) {
        WiFi.begin(found_ssid_char, password);
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

    if(WiFi.status() == WL_CONNECTED) {
      DPRINTLN("connected to Wi-Fi");
      WiFi.mode(WIFI_STA);
      dns_client.begin(public_dns_ip);
      first_try_connect = true;
      return true;
    } else {
      DPRINT("Wi-Fi SSID found but failed to connect: ");
      DPRINTLN(WiFi.status());
      return false;
    }
  } else
    return true;
}

bool WiFi_interface_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFi_get_ip_by_name(char* host, IPAddress* resulting_ip) {
  return WiFi.hostByName(host, *resulting_ip);
}
