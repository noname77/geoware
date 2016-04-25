#ifndef GEOWARE_H
#define GEOWARE_H

#include "lib/list.h"
#include "lib/memb.h"
#include "net/rime.h"

#include <stdint.h>

#include "subscriptions.h"
#include "geo.h"
#include "aggregates.h"
#include "geoware_sensors.h"
#include "packets.h"
#include "helpers.h"

#define GEOWARE_VERSION 1

#define MEMB_GLOBAL(name, structure, num) \
        char CC_CONCAT(name,_memb_count)[num]; \
        structure CC_CONCAT(name,_memb_mem)[num]; \
        struct memb name = {sizeof(structure), num, \
                                  CC_CONCAT(name,_memb_count), \
                                  (void *)CC_CONCAT(name,_memb_mem)}

#define LIST_GLOBAL(name) \
        void *LIST_CONCAT(name,_list) = NULL; \
        list_t name = (list_t)&LIST_CONCAT(name,_list)

/* This structure holds information about neighbors. */
struct neighbor {
  /* The ->next pointer is needed since we are placing these
     on a Contiki list. */
  struct neighbor *next;

  /* The ->addr field holds the Rime address of the neighbor. */
  rimeaddr_t addr;

  /* The ->pos field holds the location of the neighbor [0] and his 
     neighbors rest */
  pos_t pos[MAX_NEIGHBOR_NEIGHBORS+1];

  /* The -> neighbors holds the number of neighbor neighbors that we 
     currently store */
  uint8_t neighbors;

  /* The ->timestamp contains the last time we heard from 
     this neighbour */
  uint32_t timestamp;

  /* The ->ctimer is used to remove old neighbors */
  struct ctimer ctimer;
};

extern list_t neighbors_list;

extern process_event_t geoware_reading_event;
extern pos_t own_pos;

extern process_event_t broadcast_subscription_event;
extern process_event_t broadcast_unsubscription_event;


PROCESS_NAME(broadcast_process);
PROCESS_NAME(multihop_process);

void geoware_init();
sid_t subscribe(sensor_t type, uint32_t period, \
                uint8_t aggr_type, uint8_t aggr_num, pos_t center, \
                float radius);
void unsubscribe(sid_t sID);
void publish(sid_t sID, reading_val value);
void print_neighbors();


#endif