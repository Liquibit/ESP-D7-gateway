#include <ETH.h>
#include "structures.h"

static bool eth_connected = false;

void EthernetEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      DPRINTLN("ETH Started");
      //set eth hostname here
      ETH.setHostname("Dash7-gateway");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      DPRINTLN("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      DPRINTLN("ETH got IP!");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      DPRINTLN("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      DPRINTLN("ETH Stopped");
      eth_connected = false;
      break;
    default:
      DPRINT("unknown ethernet event: ");
      DPRINTLN(event);
      break;
  }
}

void ethernet_init()
{
  WiFi.onEvent(EthernetEvent);
  ETH.begin();
}

bool ethernet_interface_is_connected()
{
    return eth_connected;
}
