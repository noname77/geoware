#ifndef SUBSCRIPTIONS_H
#define SUBSCRIPTIONS_H

#include <stdint.h>

#include "geoware_sensors.h"
#include "geo.h"
#include "aggregates.h"

typedef uint16_t sid_t;

typedef struct {
  sid_t sID;
  pos_t owner_pos;
} subscription_hdr_t;

typedef struct {
  subscription_hdr_t subscription_hdr;
  sensor_t type;
  uint32_t period; //in units of ms
  aggr_t aggr_type;
  uint8_t aggr_num;
  pos_t center;
  float radius;
} subscription_t;

/* This structure holds information about active subscriptions. */
struct subscription {
  /* The ->next pointer is needed since we are placing these
     on a Contiki list. */
  struct subscription *next;

  /* -> sub holds the subscription information */
  subscription_t sub;

  /* -> callback is a ctimer that gets fired according to the subscription
     and reads the specified sensor type */
  struct ctimer callback;

  uint8_t num;

  struct process *proc;
};

extern list_t active_subscriptions;

sid_t add_seen_sub(sid_t sID);
sid_t remove_seen_sub(sid_t sID);
uint8_t was_seen(sid_t sID);
uint8_t is_subscribed(sid_t sID);
subscription_t* add_subscription(subscription_t *sub);
subscription_t* get_subscription(sid_t sID);
struct subscription* get_subscription_struct(sid_t sID);
sid_t remove_subscription(sid_t sID);
void print_subscription(subscription_t *sub);
reading_val get_reading_type(sensor_t t);
uint8_t reading_add(sid_t sID, rimeaddr_t* owner, reading_val* value);

#endif