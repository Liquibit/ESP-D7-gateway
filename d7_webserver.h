#ifndef D7_WEBSERVER_H
#define D7_WEBSERVER_H
#include "structures.h"

typedef void (*webserver_update_callback) ();

void webserver_init(const char* mdns_hostname, webserver_update_callback callback, persisted_data_t data);

void webserver_handle();

#endif
