#ifndef SUBSCRIPTIONS_H
#define SUBSCRIPTIONS_H

#include "geoware.h"

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
};

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

typedef union {
  uint8_t ui8;
  uint16_t ui16;
  float fl;
} reading_val;



sid_t add_seen_sub(sid_t sID);
sid_t remove_seen_sub(sid_t sID);
uint8_t was_seen(sid_t sID);
uint8_t is_subscribed(sid_t sID);
subscription_t* add_subscription(subscription_t *sub);
subscription_t* get_subscription(sid_t sID);
sid_t remove_subscription(sid_t sID);
void process_subscription(subscription_t *subscription);
void process_unsubscription(unsubscription_pkt_t *unsubscription_pkt);
uint8_t prepare_unsub_pkt(unsubscription_pkt_t *unsubscription_pkt, sid_t sID);
void print_subscription(subscription_t *sub);
void subscriptions_init();

#endif