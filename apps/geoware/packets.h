#ifndef PACKETS_H
#define PACKETS_H

#include "subscriptions.h"
#include "geoware_sensors.h"
#include "geo.h"

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
  pos_t pos;
} geoware_hdr_t;

typedef struct {
	geoware_hdr_t hdr;
	pos_t npos[MAX_NEIGHBOR_NEIGHBORS];
} broadcast_pkt_t;

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
	reading_hdr_t reading_hdr;
	reading_val value;
} reading_pkt_t;

typedef struct {
  sid_t* sIDs;
} sid_discovery_t;


void process_subscription(subscription_pkt_t *sub_pkt);
void process_unsubscription(unsubscription_pkt_t *unsub_pkt);
uint8_t prepare_unsub_pkt(unsubscription_pkt_t *unsub_pkt, sid_t sID);
void print_unsubscription(unsubscription_pkt_t *unsub_pkt);

#endif