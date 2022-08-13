#include "file_parser.h"

#define BUTTON_FILE_ID             51
#define HALL_EFFECT_FILE_ID        52
#define HUMIDITY_FILE_ID           53
#define PUSH7_STATE_FILE_ID        56
#define LIGHT_FILE_ID              57
#define PIR_FILE_ID                58

#define BUTTON_CONFIG_FILE_ID      61
#define HALL_CONFIG_EFFECT_FILE_ID 62
#define HUMIDITY_CONFIG_FILE_ID    63
#define PUSH7_CONFIG_STATE_FILE_ID 66
#define LIGHT_CONFIG_FILE_ID       67
#define PIR_CONFIG_FILE_ID         68

typedef struct {
    union {
        uint8_t bytes[3];
        struct {
            uint8_t button_id;
            uint8_t mask;
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
            uint32_t light_als;
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
