#ifndef GEOWARE_H
#define GEOWARE_H

#include <stdint.h>

#include "geo.h"
#include "lib/list.h"
#include "lib/memb.h"

#define GEOWARE_VERSION 1

// info: the sensor types have to be declared in the same order on all nodes
// otherwise the __COUNTER__ will result in different values for the same
// sensor
#define SENSOR_CREATE(name, strname, type, func)    \
  const uint8_t name = __COUNTER__/2; \
  mapping_t sensor_##name = {__COUNTER__/2, strname, type , (void (*)())func};
  // TODO: add to sensors list

#define sensor_init(name) \
  list_add(sensors_list, memb_alloc(&sensors_memb)); \
  ((struct sensor*)list_tail(sensors_list))->mapping = &sensor_##name;


enum reading_types { UINT8, UINT16, FLOAT };
typedef uint8_t reading_t;
typedef uint8_t sensor_t;

enum {
	GEOWARE_RESERVED = 0,
	GEOWARE_BROADCAST_LOC,
  GEOWARE_SUBSCRIPTION,
  GEOWARE_UNSUBSCRIPTION,
	GEOWARE_SID_DISCOVERY,
  GEOWARE_READING
};

typedef struct {
  uint8_t ver  : 2;		/**< Protocol version. */
  uint8_t type : 3;		/**< Packet type. */
  uint8_t len  : 3;
} geoware_hdr_t;

typedef struct {
	geoware_hdr_t hdr;
	pos_t pos[MAX_NEIGHBOR_NEIGHBORS+1];
} broadcast_pkt_t;


typedef uint16_t sid_t;
typedef struct {
  sid_t sID;
  pos_t owner_pos;
} subscription_hdr_t;

typedef struct {
  subscription_hdr_t subscription_hdr;
  sensor_t type;
  uint32_t period; //in units of ms
  uint8_t aggr_type;
  uint8_t aggr_num;
  pos_t center;
  float radius;
} subscription_t;

typedef struct {
  geoware_hdr_t hdr;
  subscription_t subscription;
} subscription_pkt_t;

typedef struct {
  geoware_hdr_t hdr;
  sid_t sID;
  pos_t center;
  float radius;
} unsubscription_pkt_t;

typedef struct {
  geoware_hdr_t hdr;
  subscription_hdr_t subscription_hdr;
} reading_hdr_t;

typedef struct {
  sid_t* sIDs;
} sid_discovery_t;

typedef struct {
  sensor_t s;
  const char* strname;
  reading_t r;
  void (*read)();
} mapping_t;

typedef union {
  uint8_t ui8;
  uint16_t ui16;
  float fl;
} reading_val;

extern process_event_t geoware_reading_event;

struct sensor {
  struct sensor *next;
  mapping_t *mapping;
};

// extern void *LIST_CONCAT(sensors_list,_list);
extern list_t sensors_list;
extern struct memb sensors_memb;

sid_t subscribe(sensor_t type, uint32_t period, \
    uint8_t aggr_type, uint8_t aggr_num, pos_t center, \
    float radius);
void unsubscribe(sid_t sID);
void publish(sid_t sID);

void geoware_init();
void sensors_init();

#endif