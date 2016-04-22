#ifndef FAKE_SENSORS_H
#define FAKE_SENSORS_H

#include <stdint.h>

float get_temperature();
uint16_t get_light();
uint8_t get_humidity();

#endif