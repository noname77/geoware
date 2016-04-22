#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>

#define PRINTFLOAT "%ld.%03u"

unsigned decimals(float f);
float stof(const char* s);
uint32_t powi(uint16_t x, uint16_t y);

#endif