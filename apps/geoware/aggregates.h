#ifndef AGGREGATES_H
#define AGGREGATES_H

#include <stdint.h>

#include "geoware_sensors.h"

#define AGGREGATE_CREATE(type, func)    \
  aggr_mapping_t aggr_##type = {type, func};

#define aggr_init(type) \
  list_add(aggregate_list, memb_alloc(&aggrs_memb)); \
  ((struct aggregate*)list_tail(aggregate_list))->mapping = &aggr_##type;

// sid_t already defined in subscriptions.h but dont know how to make it better
// without a header just for this..
typedef uint16_t sid_t;
typedef uint8_t aggr_t;
typedef reading_owned (*aggr_func)(sid_t sID, uint8_t num);

typedef struct {
  aggr_t type;
  aggr_func func;
} aggr_mapping_t;

struct aggregate {
  struct aggregate *next;
  aggr_mapping_t *mapping;
};

extern list_t aggregate_list;
extern struct memb aggrs_memb;

void aggregates_init();
aggr_mapping_t* get_aggregate(sid_t sID);

#endif