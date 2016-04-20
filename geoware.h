#ifndef GEOWARE_H
#define GEOWARE_H

#include <stdint.h>

#include "geo.h"

#define GEOWARE_VERSION 1

typedef enum { UINT8, UINT16, FLOAT } reading_type;

typedef union
{
  uint8_t ui8;
  uint16_t ui16;
  float fl;
} reading_val;

enum
{
	GEOWARE_RESERVED = 0,
	GEOWARE_BROADCAST,
  GEOWARE_SUBSCRIPTION,
	GEOWARE_SID_DISCOVERY,
  GEOWARE_READING
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
} broadcast_pkt_t;

typedef struct
{
  geoware_hdr_t hdr;
  uint16_t sID;
  pos_t owner_pos;
} reading_hdr_t;



typedef struct
{
	uint16_t* sIDs;
} sid_discovery_t;

typedef struct {
  uint16_t sID;
  pos_t owner_pos;
  rimeaddr_t owner_addr;
  uint8_t type;
  uint32_t period; //in units of ms
  uint8_t aggr_type;
  uint8_t aggr_num;
  pos_t center;
  float radius;
} subscription_t;



uint16_t subscribe(uint8_t type, uint32_t period, uint8_t aggr_type, uint8_t aggr_num);
void unsubscribe(uint16_t type);
void publish(uint16_t sID);

#endif