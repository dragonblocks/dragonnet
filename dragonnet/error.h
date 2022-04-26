#ifndef _DRAGONNET_ERROR_H_
#define _DRAGONNET_ERROR_H_

#include <stdbool.h>

void dragonnet_perror(const char *str);
bool dragonnet_isconnerr();
bool dragonnet_isintrerr();

#endif
