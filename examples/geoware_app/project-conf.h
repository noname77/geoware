#ifndef PROJECT_H_
#define PROJECT_H_

/* Use 2 byte rime address */
#undef RIMEADDR_CONF_SIZE
#define RIMEADDR_CONF_SIZE			2

/* Broadcast channel number */
#define BROADCAST_CHANNEL			229
/* Multihop channel number */
#define MULTIHOP_CHANNEL			228

/* Defines how often (in seconds) a brodcast packet will be sent
 * (with a jitter of half of that time).
 */
#define BROADCAST_PERIOD 			30
/* Defines the maximum number of neighbors we can remember. */
#define MAX_NEIGHBORS				16
/* Defines the maximum number of active subscriptions we can hold. */
#define MAX_ACTIVE_SUBSCRIPTIONS	6
/* How long (in seconds) before a neighbor becomes stale */
#define NEIGHBOR_TIMEOUT			60
/* How many 2nd degree neighbors will be reported in the broadcast */
#define MAX_NEIGHBOR_NEIGHBORS		8

/* Maximum number of readings we can store and use with aggregate functions */
#define MAX_READINGS				30

/* maximum number of sensors geoware will support, used to allocate memory
   for the sensor mappings */
#define MAX_SENSORS					5

#endif /* PROJECT_H_ */