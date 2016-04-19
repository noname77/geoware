#ifndef PROJECT_H_
#define PROJECT_H_

/* Use 2 byte rime address */
#undef RIMEADDR_CONF_SIZE
#define RIMEADDR_CONF_SIZE			2

/* Broadcast channel number */
#define BROADCAST_CHANNEL			229
/* Runicast channel number */
#define RUNICAST_CHANNEL			228

/* Defines how often (in seconds) a brodcast packet will be sent
 * (with a jitter of half of that time).
 */
#define BROADCAST_PERIOD 			4
/* Defines the maximum number of neighbors we can remember. */
#define MAX_NEIGHBORS				16
/* Defines the maximum number of active subscriptions we can hold. */
#define MAX_ACTIVE_SUBSCRIPTIONS	6
/* How long (in seconds) before a neighbor becomes stale */
#define NEIGHBOR_TIMEOUT			10
/* How many 2nd degree neighbors will be reported in the broadcast */
#define MAX_NEIGHBOR_NEIGHBORS		8

#endif /* PROJECT_H_ */