#include "file_parser.h"
#include <Arduino.h>

#define BUTTON_FILE_ID             51
#define HUMIDITY_FILE_ID           53
#define PUSH7_STATE_FILE_ID        56
#define LIGHT_FILE_ID              57
#define PIR_FILE_ID                58
#define HALL_EFFECT_FILE_ID        59

#define BUTTON_CONFIG_FILE_ID      61
#define HUMIDITY_CONFIG_FILE_ID    63
#define PUSH7_CONFIG_STATE_FILE_ID 66
#define LIGHT_CONFIG_FILE_ID       67
#define PIR_CONFIG_FILE_ID         68
#define HALL_EFFECT_CONFIG_FILE_ID 69

typedef struct {
    union {
        uint8_t bytes[3];
        struct {
            uint8_t button_id;
            bool mask;
            uint8_t buttons_state;
        } __attribute__((__packed__));
    };
} button_file_t;

typedef struct {
    union {
        uint8_t bytes[4];
        struct {
            bool transmit_mask_0;
            bool transmit_mask_1;
            bool button_control_menu;
            bool enabled;
        } __attribute__((__packed__));
    };
} button_config_file_t;

typedef struct
{
    union
    {
        uint8_t bytes[1];
        struct
        {
            bool mask;
        } __attribute__((__packed__));
    };
} pir_file_t;

typedef struct
{
    union
    {
        uint8_t bytes[9];
        struct
        {
            bool transmit_mask_0;
            bool transmit_mask_1;
            uint8_t filter_source; // PYD1598_FILTER_SOURCE_t
            uint8_t window_time; // Window time = [RegisterValue] * 2s + 2s
            uint8_t pulse_counter; // Amount of pulses = [RegisterValue] + 1
            uint16_t blind_time; // seconds
            uint8_t threshold;
            bool enabled;
        } __attribute__((__packed__));
    };
} pir_config_file_t;

typedef struct {
    union {
        uint8_t bytes[8];
        struct {
            int32_t humidity;
            int32_t temperature;
        } __attribute__((__packed__));
    };
} humidity_file_t;

typedef struct {
    union {
        uint8_t bytes[5];
        struct {
            uint32_t interval;
            bool enabled;
        } __attribute__((__packed__));
    };
} humidity_config_file_t;

typedef struct {
    union {
        uint8_t bytes[4];
        struct {
            uint16_t battery_voltage;
            uint8_t hw_version;
            uint8_t sw_version;
        } __attribute__((__packed__));
    };
} push7_state_file_t;

typedef struct {
    union {
        uint8_t bytes[6];
        struct {
            uint32_t interval;
            bool led_flash_state;
            bool enabled;
        } __attribute__((__packed__));
    };
} push7_state_config_file_t;

typedef struct {
    union {
        uint8_t bytes[4];
        struct {
            uint32_t light_level;
            uint16_t light_level_raw;
            bool threshold_high_triggered;
            bool threshold_low_triggered;
        } __attribute__((__packed__));
    };
} light_file_t;

typedef struct {
    union {
        uint8_t bytes[8];
        struct {
            uint32_t interval;
            uint8_t integration_time;
            uint8_t persistence_protect_number;
            uint8_t gain;
            uint16_t threshold_high;
            uint16_t threshold_low;
            bool light_detection_mode;
            uint8_t low_power_mode;
            uint8_t interrupt_check_interval;
            uint8_t threshold_menu_offset;
            bool enabled;
        } __attribute__((__packed__));
    };
} light_config_file_t;

typedef struct {
    union {
        uint8_t bytes[1];
        struct {
            bool mask;
        } __attribute__((__packed__));
    };
} hall_effect_file_t;

typedef struct {
    union {
        uint8_t bytes[3];
        struct {
            bool transmit_mask_0;
            bool transmit_mask_1;
            bool enabled;
        } __attribute__((__packed__));
    };
} hall_effect_config_file_t;

typedef struct {
  union {
    uint8_t bytes[9];
    button_file_t button_file;
    button_config_file_t button_config_file;
    pir_file_t pir_file;
    pir_config_file_t pir_config_file;
    humidity_file_t humidity_file;
    humidity_config_file_t humidity_config_file;
    push7_state_file_t push7_state_file;
    push7_state_config_file_t push7_state_config_file;
    light_file_t light_file;
    light_config_file_t light_config_file;
    hall_effect_file_t hall_effect_file;
    hall_effect_config_file_t hall_effect_config_file;
  };
} custom_file_t;

static uint8_t max_publish_results;

void file_parser_init(uint8_t max_size) {
  max_publish_results = max_size;
}

uint8_t parse_custom_files(custom_file_contents_t* custom_file_contents, publish_object_t* results)
{
  custom_file_t* file = (custom_file_t*) custom_file_contents->buffer;
  uint8_t file_id = custom_file_contents->file_id;
  uint8_t offset = custom_file_contents->offset;
  uint8_t length = custom_file_contents->length;

  uint8_t number_of_publish_objects = 0;

  memset(results, 0, sizeof(publish_object_t) * max_publish_results);

  sprintf(results[0].uid, "%02X%02X%02X%02X%02X%02X%02X%02X", custom_file_contents->uid[0], custom_file_contents->uid[1], custom_file_contents->uid[2], custom_file_contents->uid[3], custom_file_contents->uid[4], custom_file_contents->uid[5], custom_file_contents->uid[6], custom_file_contents->uid[7]);

  switch(file_id) {
    case BUTTON_FILE_ID:
      {
      sprintf(results[0].name, "button%d", file->button_file.button_id+1);
      sprintf(results[0].object_id, "%s_button%d", results[0].uid, file->button_file.button_id+1);
      sprintf(results[0].component, "binary_sensor");
      sprintf(results[0].state, "%s", file->button_file.mask ? "ON" : "OFF");
      results[0].default_shown = true;

      sprintf(results[1].name, "received signal strength");
      sprintf(results[1].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].category, "diagnostic");
      sprintf(results[1].device_class, "measurement");
      sprintf(results[1].unit, "dBm");
      sprintf(results[1].device_class, "signal_strength");
      sprintf(results[1].state, "%i", custom_file_contents->rssi);
      results[1].default_shown = true;

      number_of_publish_objects = 2;
      }
      break;
    case HALL_EFFECT_FILE_ID:
      {
      sprintf(results[0].name, "hall effect");
      sprintf(results[0].object_id, "%s_hall_effect", results[0].uid);
      sprintf(results[0].component, "binary_sensor");
      sprintf(results[0].state, "%s", file->hall_effect_file.mask ? "ON" : "OFF");
      sprintf(results[0].icon, "mdi:magnet");
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "received signal strength");
      sprintf(results[1].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].category, "diagnostic");
      sprintf(results[1].device_class, "measurement");
      sprintf(results[1].unit, "dBm");
      sprintf(results[1].device_class, "signal_strength");
      sprintf(results[1].state, "%i", custom_file_contents->rssi);
      results[1].default_shown = true;

      number_of_publish_objects = 2;
      }
      break;
    case HUMIDITY_FILE_ID:
      {
      sprintf(results[0].name, "temperature");
      sprintf(results[0].object_id, "%s_temperature", results[0].uid);
      sprintf(results[0].component, "sensor");
      sprintf(results[0].device_class, "temperature");
      sprintf(results[0].state_class, "measurement");
      sprintf(results[0].unit, "°C");
      sprintf(results[0].icon, "mdi:thermometer");
      float degrees = (float) file->humidity_file.temperature / 10;
      sprintf(results[0].state, "%.1f", degrees);
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "humidity");
      sprintf(results[1].object_id, "%s_humidity", results[1].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].device_class, "humidity");
      sprintf(results[1].state_class, "measurement");
      sprintf(results[1].unit, "%");
      sprintf(results[1].icon, "mdi:water-percent");
      int16_t percentage = round((float) file->humidity_file.humidity / 10);
      sprintf(results[1].state, "%i", percentage);
      results[1].default_shown = true;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "received signal strength");
      sprintf(results[2].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[2].component, "sensor");
      sprintf(results[2].category, "diagnostic");
      sprintf(results[2].device_class, "measurement");
      sprintf(results[2].unit, "dBm");
      sprintf(results[2].device_class, "signal_strength");
      sprintf(results[2].state, "%i", custom_file_contents->rssi);
      results[2].default_shown = true;

      number_of_publish_objects = 3;
      }
      break;
    case PUSH7_STATE_FILE_ID:
      {
      sprintf(results[0].name, "battery voltage");
      sprintf(results[0].object_id, "%s_battery_voltage", results[0].uid);
      sprintf(results[0].component, "sensor");
      sprintf(results[0].device_class, "voltage");
      sprintf(results[0].state_class, "measurement");
      sprintf(results[0].category, "diagnostic");
      sprintf(results[0].unit, "V");
      sprintf(results[0].icon, "mdi:sine-wave");
      float voltage = (float) file->push7_state_file.battery_voltage / 1000;
      sprintf(results[0].state, "%.3f", voltage);

      sprintf(results[0].sw_version, "%d",file->push7_state_file.sw_version);

      sprintf(results[0].model, "Push7_v%d", file->push7_state_file.hw_version);
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "received signal strength");
      sprintf(results[1].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].category, "diagnostic");
      sprintf(results[1].device_class, "measurement");
      sprintf(results[1].unit, "dBm");
      sprintf(results[1].device_class, "signal_strength");
      sprintf(results[1].state, "%i", custom_file_contents->rssi);
      results[1].default_shown = true;

      number_of_publish_objects = 2;
      }
      break;
    case LIGHT_FILE_ID:
      {
      sprintf(results[0].name, "light level");
      sprintf(results[0].object_id, "%s_light_level", results[0].uid);
      sprintf(results[0].component, "sensor");
      sprintf(results[0].device_class, "illuminance");
      sprintf(results[0].state_class, "measurement");
      sprintf(results[0].unit, "lx");
      float lux = (float) file->light_file.light_level / 10;
      sprintf(results[0].state, "%.1f", lux);
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "light level raw");
      sprintf(results[1].object_id, "%s_light_raw", results[0].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].state_class, "measurement");
      sprintf(results[1].icon, "mdi:sun-wireless");
      sprintf(results[1].state, "%i", file->light_file.light_level_raw);
      results[1].default_shown = false;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "light threshold high triggered");
      sprintf(results[2].object_id, "%s_light_threshold_high_triggered", results[0].uid);
      sprintf(results[2].component, "binary_sensor");
      sprintf(results[2].state, "%s", file->light_file.threshold_high_triggered ? "ON" : "OFF");
      sprintf(results[2].device_class, "motion");
      results[2].default_shown = false;

      sprintf(results[3].uid, "%s", results[0].uid);
      sprintf(results[3].name, "light threshold low triggered");
      sprintf(results[3].object_id, "%s_light_threshold_low_triggered", results[0].uid);
      sprintf(results[3].component, "binary_sensor");
      sprintf(results[3].state, "%s", file->light_file.threshold_low_triggered ? "ON" : "OFF");
      sprintf(results[3].device_class, "motion");
      results[3].default_shown = false;

      sprintf(results[4].uid, "%s", results[0].uid);
      sprintf(results[4].name, "received signal strength");
      sprintf(results[4].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[4].component, "sensor");
      sprintf(results[4].category, "diagnostic");
      sprintf(results[4].device_class, "measurement");
      sprintf(results[4].unit, "dBm");
      sprintf(results[4].device_class, "signal_strength");
      sprintf(results[4].state, "%i", custom_file_contents->rssi);
      results[4].default_shown = true;

      number_of_publish_objects = 5;
      }
      break;
    case PIR_FILE_ID:
      {
      sprintf(results[0].name, "pir");
      sprintf(results[0].object_id, "%s_pir", results[0].uid);
      sprintf(results[0].component, "binary_sensor");
      sprintf(results[0].state, "%s", file->pir_file.mask ? "ON" : "OFF");
      sprintf(results[0].device_class, "motion");
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "received signal strength");
      sprintf(results[1].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].category, "diagnostic");
      sprintf(results[1].device_class, "measurement");
      sprintf(results[1].unit, "dBm");
      sprintf(results[1].device_class, "signal_strength");
      sprintf(results[1].state, "%i", custom_file_contents->rssi);
      results[1].default_shown = true;

      number_of_publish_objects = 2;
      }
      break;
    case BUTTON_CONFIG_FILE_ID:
      {
      sprintf(results[0].name, "buttons transmit 0");
      sprintf(results[0].object_id, "%s_buttons_transmit_0", results[0].uid);
      sprintf(results[0].component, "binary_sensor");
      sprintf(results[0].state, "%s", file->button_config_file.transmit_mask_0 ? "ON" : "OFF");
      sprintf(results[0].category, "diagnostic");
      results[0].default_shown = false;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "buttons transmit 1");
      sprintf(results[1].object_id, "%s_buttons_transmit_1", results[0].uid);
      sprintf(results[1].component, "binary_sensor");
      sprintf(results[1].state, "%s", file->button_config_file.transmit_mask_1 ? "ON" : "OFF");
      sprintf(results[1].category, "diagnostic");
      results[1].default_shown = false;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "buttons control menu");
      sprintf(results[2].object_id, "%s_buttons_control_menu", results[0].uid);
      sprintf(results[2].component, "binary_sensor");
      sprintf(results[2].state, "%s", file->button_config_file.button_control_menu ? "ON" : "OFF");
      sprintf(results[2].category, "diagnostic");
      results[2].default_shown = false;

      sprintf(results[3].uid, "%s", results[0].uid);
      sprintf(results[3].name, "buttons enabled");
      sprintf(results[3].object_id, "%s_buttons_enabled", results[0].uid);
      sprintf(results[3].component, "binary_sensor");
      sprintf(results[3].state, "%s", file->button_config_file.enabled ? "ON" : "OFF");
      sprintf(results[3].category, "diagnostic");
      sprintf(results[3].device_class, "running");
      results[3].default_shown = true;

      sprintf(results[4].uid, "%s", results[0].uid);
      sprintf(results[4].name, "received signal strength");
      sprintf(results[4].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[4].component, "sensor");
      sprintf(results[4].category, "diagnostic");
      sprintf(results[4].device_class, "measurement");
      sprintf(results[4].unit, "dBm");
      sprintf(results[4].device_class, "signal_strength");
      sprintf(results[4].state, "%i", custom_file_contents->rssi);
      results[4].default_shown = true;

      number_of_publish_objects = 5;
      }
      break;
    case HALL_EFFECT_CONFIG_FILE_ID:
      {
      sprintf(results[0].name, "hall effect transmit 0");
      sprintf(results[0].object_id, "%s_hall_effect_transmit_0", results[0].uid);
      sprintf(results[0].component, "binary_sensor");
      sprintf(results[0].state, "%s", file->hall_effect_config_file.transmit_mask_0 ? "ON" : "OFF");
      sprintf(results[0].category, "diagnostic");
      results[0].default_shown = false;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "hall effect transmit 1");
      sprintf(results[1].object_id, "%s_hall_effect_transmit_1", results[0].uid);
      sprintf(results[1].component, "binary_sensor");
      sprintf(results[1].state, "%s", file->hall_effect_config_file.transmit_mask_1 ? "ON" : "OFF");
      sprintf(results[1].category, "diagnostic");
      results[1].default_shown = false;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "hall effect enabled");
      sprintf(results[2].object_id, "%s_hall_effect_enabled", results[0].uid);
      sprintf(results[2].component, "binary_sensor");
      sprintf(results[2].state, "%s", file->hall_effect_config_file.enabled ? "ON" : "OFF");
      sprintf(results[2].category, "diagnostic");
      sprintf(results[2].device_class, "running");
      results[2].default_shown = true;

      sprintf(results[3].uid, "%s", results[0].uid);
      sprintf(results[3].name, "received signal strength");
      sprintf(results[3].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[3].component, "sensor");
      sprintf(results[3].category, "diagnostic");
      sprintf(results[3].device_class, "measurement");
      sprintf(results[3].unit, "dBm");
      sprintf(results[3].device_class, "signal_strength");
      sprintf(results[3].state, "%i", custom_file_contents->rssi);
      results[3].default_shown = true;

      number_of_publish_objects = 4;
      }
      break;
    case HUMIDITY_CONFIG_FILE_ID:
      {
      sprintf(results[0].name, "humidity temperature interval");
      sprintf(results[0].object_id, "%s_humidity_temperature_interval", results[0].uid);
      sprintf(results[0].component, "sensor");
      sprintf(results[0].device_class, "duration");
      sprintf(results[0].category, "diagnostic");
      sprintf(results[0].unit, "s");
      sprintf(results[0].state, "%i", file->humidity_config_file.interval);
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "humidity temperature enabled");
      sprintf(results[1].object_id, "%s_humidity_temperature_enabled", results[0].uid);
      sprintf(results[1].component, "binary_sensor");
      sprintf(results[1].state, "%s", file->humidity_config_file.enabled ? "ON" : "OFF");
      sprintf(results[1].category, "diagnostic");
      sprintf(results[1].device_class, "running");
      results[1].default_shown = true;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "received signal strength");
      sprintf(results[2].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[2].component, "sensor");
      sprintf(results[2].category, "diagnostic");
      sprintf(results[2].device_class, "measurement");
      sprintf(results[2].unit, "dBm");
      sprintf(results[2].device_class, "signal_strength");
      sprintf(results[2].state, "%i", custom_file_contents->rssi);
      results[2].default_shown = true;

      number_of_publish_objects = 3;
      }
      break;
    case PUSH7_CONFIG_STATE_FILE_ID:
      {
      sprintf(results[0].name, "state interval");
      sprintf(results[0].object_id, "%s_state_interval", results[0].uid);
      sprintf(results[0].component, "sensor");
      sprintf(results[0].device_class, "duration");
      sprintf(results[0].category, "diagnostic");
      sprintf(results[0].unit, "s");
      sprintf(results[0].state, "%i", file->push7_state_config_file.interval);
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "state flash led");
      sprintf(results[1].object_id, "%s_state_flash_led", results[0].uid);
      sprintf(results[1].component, "binary_sensor");
      sprintf(results[1].state, "%s", file->push7_state_config_file.led_flash_state ? "ON" : "OFF");
      sprintf(results[1].category, "diagnostic");
      results[1].default_shown = false;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "state enabled");
      sprintf(results[2].object_id, "%s_state_enabled", results[0].uid);
      sprintf(results[2].component, "binary_sensor");
      sprintf(results[2].state, "%s", file->push7_state_config_file.enabled ? "ON" : "OFF");
      sprintf(results[2].category, "diagnostic");
      sprintf(results[2].device_class, "running");
      results[2].default_shown = true;

      sprintf(results[3].uid, "%s", results[0].uid);
      sprintf(results[3].name, "received signal strength");
      sprintf(results[3].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[3].component, "sensor");
      sprintf(results[3].category, "diagnostic");
      sprintf(results[3].device_class, "measurement");
      sprintf(results[3].unit, "dBm");
      sprintf(results[3].device_class, "signal_strength");
      sprintf(results[3].state, "%i", custom_file_contents->rssi);
      results[3].default_shown = true;

      number_of_publish_objects = 4;
      }
      break;
    case LIGHT_CONFIG_FILE_ID:
      {
      sprintf(results[0].name, "light interval");
      sprintf(results[0].object_id, "%s_light_interval", results[0].uid);
      sprintf(results[0].component, "sensor");
      sprintf(results[0].device_class, "duration");
      sprintf(results[0].category, "diagnostic");
      sprintf(results[0].unit, "s");
      sprintf(results[0].state, "%i", file->light_config_file.interval);
      results[0].default_shown = true;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "light integration time");
      sprintf(results[1].object_id, "%s_light_integration_time", results[0].uid);
      sprintf(results[1].component, "sensor");
      sprintf(results[1].device_class, "duration");
      sprintf(results[1].category, "diagnostic");
      sprintf(results[1].unit, "s");
      sprintf(results[1].state, "%i", file->light_config_file.integration_time);
      results[1].default_shown = false;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "light persistence protect number");
      sprintf(results[2].object_id, "%s_light_persistence_protect_number", results[0].uid);
      sprintf(results[2].component, "sensor");
      sprintf(results[2].category, "diagnostic");
      sprintf(results[2].state, "%i", file->light_config_file.persistence_protect_number);
      results[2].default_shown = false;

      sprintf(results[3].uid, "%s", results[0].uid);
      sprintf(results[3].name, "light gain");
      sprintf(results[3].object_id, "%s_light_gain", results[0].uid);
      sprintf(results[3].component, "sensor");
      sprintf(results[3].category, "diagnostic");
      sprintf(results[3].state, "%i", file->light_config_file.gain);
      results[3].default_shown = false;

      sprintf(results[4].uid, "%s", results[0].uid);
      sprintf(results[4].name, "light threshold high");
      sprintf(results[4].object_id, "%s_light_threshold_high", results[0].uid);
      sprintf(results[4].component, "sensor");
      sprintf(results[4].category, "diagnostic");
      sprintf(results[4].state, "%i", file->light_config_file.threshold_high);
      results[4].default_shown = false;

      sprintf(results[5].uid, "%s", results[0].uid);
      sprintf(results[5].name, "light threshold low");
      sprintf(results[5].object_id, "%s_light_threshold_low", results[0].uid);
      sprintf(results[5].component, "sensor");
      sprintf(results[5].category, "diagnostic");
      sprintf(results[5].state, "%i", file->light_config_file.threshold_low);
      results[5].default_shown = false;

      sprintf(results[6].uid, "%s", results[0].uid);
      sprintf(results[6].name, "light detection mode");
      sprintf(results[6].object_id, "%s_light_detection_mode", results[0].uid);
      sprintf(results[6].component, "binary_sensor");
      sprintf(results[6].category, "diagnostic");
      sprintf(results[6].state, "%s", file->light_config_file.light_detection_mode ? "ON" : "OFF");
      results[6].default_shown = false;

      sprintf(results[7].uid, "%s", results[0].uid);
      sprintf(results[7].name, "light low power mode");
      sprintf(results[7].object_id, "%s_light_low_power_mode", results[0].uid);
      sprintf(results[7].component, "sensor");
      sprintf(results[7].category, "diagnostic");
      sprintf(results[7].state, "%i", file->light_config_file.low_power_mode);
      results[7].default_shown = false;

      sprintf(results[8].uid, "%s", results[0].uid);
      sprintf(results[8].name, "light interrupt check interval");
      sprintf(results[8].object_id, "%s_light_interrupt_check_interval", results[0].uid);
      sprintf(results[8].component, "sensor");
      sprintf(results[8].category, "diagnostic");
      sprintf(results[8].unit, "s");
      sprintf(results[8].device_class, "duration");
      sprintf(results[8].state, "%i", file->light_config_file.interrupt_check_interval);
      results[8].default_shown = false;

      sprintf(results[9].uid, "%s", results[0].uid);
      sprintf(results[9].name, "light threshold menu offset");
      sprintf(results[9].object_id, "%s_light_threshold_menu_offset", results[0].uid);
      sprintf(results[9].component, "sensor");
      sprintf(results[9].category, "diagnostic");
      sprintf(results[9].state, "%i", file->light_config_file.threshold_menu_offset);
      results[9].default_shown = false;

      sprintf(results[10].uid, "%s", results[0].uid);
      sprintf(results[10].name, "light enabled");
      sprintf(results[10].object_id, "%s_light_enabled", results[0].uid);
      sprintf(results[10].component, "binary_sensor");
      sprintf(results[10].state, "%s", file->light_config_file.enabled ? "ON" : "OFF");
      sprintf(results[10].category, "diagnostic");
      sprintf(results[10].device_class, "running");
      results[10].default_shown = true;

      sprintf(results[11].uid, "%s", results[0].uid);
      sprintf(results[11].name, "received signal strength");
      sprintf(results[11].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[11].component, "sensor");
      sprintf(results[11].category, "diagnostic");
      sprintf(results[11].device_class, "measurement");
      sprintf(results[11].unit, "dBm");
      sprintf(results[11].device_class, "signal_strength");
      sprintf(results[11].state, "%i", custom_file_contents->rssi);
      results[11].default_shown = true;

      number_of_publish_objects = 12;
      }
      break;
    case PIR_CONFIG_FILE_ID:
      {
      sprintf(results[0].name, "pir transmit 0");
      sprintf(results[0].object_id, "%s_pir_transmit_0", results[0].uid);
      sprintf(results[0].component, "binary_sensor");
      sprintf(results[0].state, "%s", file->pir_config_file.transmit_mask_0 ? "ON" : "OFF");
      sprintf(results[0].category, "diagnostic");
      results[0].default_shown = false;

      sprintf(results[1].uid, "%s", results[0].uid);
      sprintf(results[1].name, "pir transmit 1");
      sprintf(results[1].object_id, "%s_pir_transmit_1", results[0].uid);
      sprintf(results[1].component, "binary_sensor");
      sprintf(results[1].state, "%s", file->pir_config_file.transmit_mask_1 ? "ON" : "OFF");
      sprintf(results[1].category, "diagnostic");
      results[1].default_shown = false;

      sprintf(results[2].uid, "%s", results[0].uid);
      sprintf(results[2].name, "pir filter source");
      sprintf(results[2].object_id, "%s_pir_filter_source", results[0].uid);
      sprintf(results[2].component, "sensor");
      sprintf(results[2].category, "diagnostic");
      sprintf(results[2].state, "%i", file->pir_config_file.filter_source);
      results[2].default_shown = false;

      sprintf(results[3].uid, "%s", results[0].uid);
      sprintf(results[3].name, "pir window time");
      sprintf(results[3].object_id, "%s_pir_window_time", results[0].uid);
      sprintf(results[3].component, "sensor");
      sprintf(results[3].device_class, "duration");
      sprintf(results[3].category, "diagnostic");
      sprintf(results[3].unit, "s");
      sprintf(results[3].state, "%i", file->pir_config_file.window_time * 2 + 2);
      results[3].default_shown = false;

      sprintf(results[4].uid, "%s", results[0].uid);
      sprintf(results[4].name, "pir pulse counter");
      sprintf(results[4].object_id, "%s_pir_pulse_counter", results[0].uid);
      sprintf(results[4].component, "sensor");
      sprintf(results[4].category, "diagnostic");
      sprintf(results[4].state, "%i", file->pir_config_file.pulse_counter);
      results[4].default_shown = false;

      sprintf(results[5].uid, "%s", results[0].uid);
      sprintf(results[5].name, "pir blind time");
      sprintf(results[5].object_id, "%s_pir_blind_time", results[0].uid);
      sprintf(results[5].component, "sensor");
      sprintf(results[5].device_class, "duration");
      sprintf(results[5].category, "diagnostic");
      sprintf(results[5].unit, "s");
      sprintf(results[5].state, "%i", file->pir_config_file.blind_time);
      results[5].default_shown = false;

      sprintf(results[6].uid, "%s", results[0].uid);
      sprintf(results[6].name, "pir threshold");
      sprintf(results[6].object_id, "%s_pir_threshold", results[0].uid);
      sprintf(results[6].component, "sensor");
      sprintf(results[6].category, "diagnostic");
      sprintf(results[6].state, "%i", file->pir_config_file.threshold);
      results[6].default_shown = false;

      sprintf(results[7].uid, "%s", results[0].uid);
      sprintf(results[7].name, "pir enabled");
      sprintf(results[7].object_id, "%s_pir_enabled", results[0].uid);
      sprintf(results[7].component, "binary_sensor");
      sprintf(results[7].state, "%s", file->pir_config_file.enabled ? "ON" : "OFF");
      sprintf(results[7].category, "diagnostic");
      sprintf(results[7].device_class, "running");
      results[7].default_shown = true;

      sprintf(results[8].uid, "%s", results[0].uid);
      sprintf(results[8].name, "received signal strength");
      sprintf(results[8].object_id, "%s_received_signal_strength", results[0].uid);
      sprintf(results[8].component, "sensor");
      sprintf(results[8].category, "diagnostic");
      sprintf(results[8].device_class, "measurement");
      sprintf(results[8].unit, "dBm");
      sprintf(results[8].device_class, "signal_strength");
      sprintf(results[8].state, "%i", custom_file_contents->rssi);
      results[8].default_shown = false;

      number_of_publish_objects = 9;
      }
      break;
  }

  return number_of_publish_objects;
}
