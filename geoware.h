#ifndef GEOWARE_H
#define GEOWARE_H

#include <stdint.h>

#include "geo.h"

#define GEOWARE_VERSION 1

enum
{
	GEOWARE_RESERVED = 0,
	GEOWARE_BROADCAST,
	GEOWARE_SID_DISCOVERY
};

typedef struct
{
  uint8_t ver  : 2;		/**< Protocol version. */
  uint8_t type : 3;		/**< Packet type. */
  uint8_t len  : 3;
} geoware_hdr_t;

typedef struct
{
	geoware_hdr_t hdr;
	pos_t pos[MAX_NEIGHBOR_NEIGHBORS+1];
} broadcast_t;

typedef struct
{
	uint16_t* sIDs;
} sid_discovery_t;

typedef struct {
  rimeaddr_t owner_addr;  // TODO: need this?
  pos_t owner_pos;
  uint8_t type;
  uint16_t sID;
  uint32_t period; //in units of ms
  uint8_t aggr_type;
  uint8_t aggr_num;
} subscription_t;



uint16_t subscribe(uint8_t type, uint32_t period, uint8_t aggr_type, uint8_t aggr_num);
void unsubscribe(uint16_t type);
void publish(uint16_t sID);

#endif