#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "structures.h"

void filesystem_init(int size);
void filesystem_read(persisted_data_t data);
void filesystem_write(persisted_data_t data);

#endif
