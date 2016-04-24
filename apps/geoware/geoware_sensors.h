#ifndef SENSORS_H
#define SENSORS_H

#include "net/rime.h"

// info: the sensor types have to be declared in the same order on all nodes
// otherwise the __COUNTER__ will result in different values for the same
// sensor
#define SENSOR_CREATE(name, strname, type, func)    \
  const uint8_t name = __COUNTER__/2 + 1; \
  mapping_t sensor_##name = {__COUNTER__/2 + 1, strname, type , \
    (void (*)())func}

#define sensor_init(name) \
  sensor_add(&sensor_##name)

#define HAS_MORE_READINGS(reading) ((reading).value.fl != FLT_MAX)

// sid_t already defined in subscriptions.h but dont know how to make it better
// without creating a header just for this..
typedef uint16_t sid_t;

enum reading_types { UINT8, UINT16, UINT32, FLOAT };
typedef uint8_t reading_t;
typedef uint8_t sensor_t;

typedef union {
  uint8_t ui8;
  uint16_t ui16;
  uint32_t ui32;
  float fl;
} reading_val;

typedef struct {
  rimeaddr_t owner;
  reading_val value;
} reading_owned;

typedef struct {
  sensor_t s;
  const char* strname;
  reading_t r;
  void (*read)();
} mapping_t;

struct sensor {
  struct sensor *next;
  mapping_t *mapping;
};

void sensors_init();
void remove_reading_type(sensor_t t);
void remove_reading_sid(sid_t sID);
void sensor_read(void *s);
void sensor_add(mapping_t* sensor);
reading_val get_reading_type(sensor_t t);
reading_owned get_reading_sid(sid_t sID);
mapping_t* get_mapping(sensor_t type);

#endif