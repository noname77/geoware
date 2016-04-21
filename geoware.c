/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Contiki middleware
 * \author
 *         Wiktor Grajkowski <wiktor.grajkowski.12@ucl.ac.uk>
 */

/* contiki includes */
#include "contiki.h"
#include "net/rime.h"
#include "dev/serial-line.h"
#include "dev/button-sensor.h"
#include "lib/random.h"
#include "serial-shell.h"

/* standard library includes */
#include <stdio.h> /* For printf() */
#include <float.h> /* for FLT_MAX */

/* project includes */
#include "geoware.h"
#include "helpers.h"
#include "commands.h"
#include "geo.h"
#include "fake_sensors.h"

/* Uncomment below line to include debug output */
#define DEBUG_PRINTS

#ifdef DEBUG_PRINTS
#define debug_printf printf
#else
#define debug_printf(format, args...)
#endif

// node's position
static pos_t own_pos;

// processes
PROCESS(broadcast_process, "Broadcast process");
PROCESS(multihop_process, "Multihop process");
PROCESS(boot_process, "Boot process");
AUTOSTART_PROCESSES(&boot_process);

// events
process_event_t broadcast_subscription_event;
process_event_t subscribe_event;

/*---------------------------------------------------------------------------*/

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

/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list is a Contiki list that holds the neighbors we
   have seen thus far. */
LIST(neighbors_list);

/*
 * This function prints the neighbor list.
 */
static void
print_neighbors() {
  struct neighbor *n;

  printf("neighbors:\n");
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
    printf("%d.%d ", n->addr.u8[0], n->addr.u8[1]);
    print_pos(n->pos[0]);
  }
}

/*
 * This function is called by the ctimer present in each neighbor
 * table entry. The function removes the neighbor from the table
 * because it has become too old.
 */
static void
remove_neighbor(void *n)
{
  struct neighbor *tmp = n;

  printf("removing neighbor %d.%d\n", tmp->addr.u8[0], tmp->addr.u8[1]);

  list_remove(neighbors_list, tmp);
  memb_free(&neighbors_memb, tmp);
}

/*---------------------------------------------------------------------------*/

/* This structure holds information about sensor readings. */
struct reading {
  /* The ->next pointer is needed since we are placing these
     on a Contiki list. */
  struct reading *next;

  /* type of the sensor / reading */
  sensor_t type;

  /* The ->value field (union) holds the actual reading value */
  reading_val value;
};

/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
MEMB(readings_memb, struct reading, MAX_READINGS);

/* The readings_list is a Contiki list that holds past sensor readings. */
LIST(readings_list);

/* This structure holds information about sensor readings. */
struct subscription {
  /* The ->next pointer is needed since we are placing these
     on a Contiki list. */
  struct subscription *next;

  /* -> sub holds the subscription information */
  subscription_t sub;
};

/* This MEMB() definition defines a memory pool from which we allocate
   subscription entries. */
MEMB(subscriptions_memb, struct subscription, MAX_ACTIVE_SUBSCRIPTIONS);

/* The active_subscriptions is a Contiki list that holds what it says. */
LIST(active_subscriptions);

/* Check if we already subscribed to sID. */
uint8_t already_subscribed(sid_t sID) {
  struct subscription *s;

  for(s = list_head(active_subscriptions); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(sID == s->sub.subscription_hdr.sID) {
      break;
    }
  }

  return s != NULL;
}

subscription_t* add_subscription(subscription_t *sub) {
  struct subscription *new_sub;

  new_sub = memb_alloc(&subscriptions_memb);

  /* If we could not allocate a new neighbor entry, we give up. We
     could have reused an old neighbor entry, but we do not do this
     for now. */
  if(new_sub == NULL) {
    debug_printf("Subscription list full\n");
    return NULL;
  }

  /* Initialize the fields. */
  new_sub->sub = *sub;

  /* Place the subscription on the active_subscriptions list. */
  list_add(active_subscriptions, new_sub);

  debug_printf("subscription added: %u\n", new_sub->sub.subscription_hdr.sID);

  return &new_sub->sub;
}

void process_incoming_subscription(subscription_t *subscription) {
  // check if we are in the region of interest
  if (distance(own_pos, subscription->center) > subscription->radius) {
    return;
  }

  // check if we are already subscribed
  if(!already_subscribed(subscription->subscription_hdr.sID)) {
    // TODO: check if we support the sensor type

    // add the subscription
    subscription_t *new_sub = add_subscription(subscription);

    // rebroadcast
    // TODO: only if we havent previously requested
    // TODO: what if we didnt have enough space to add new subscription but 
    // would like to worward?
    if(new_sub != NULL) {
      process_post(&broadcast_process, broadcast_subscription_event, \
        (void*) new_sub);
    }
  }
}

/*---------------------------------------------------------------------------*/
/* This function is called whenever a broadcast message is received. */
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  struct neighbor *n;
  geoware_hdr_t *broadcast_hdr;
  broadcast_pkt_t *broadcast_pkt;
  subscription_pkt_t *subscription_pkt;
  uint8_t i;

  /* The packetbuf_dataptr() returns a pointer to the first data byte
     in the received packet. */
  broadcast_hdr = packetbuf_dataptr();
  
  printf("bcast r: %d, %d, %d\n", broadcast_hdr->ver, \
  	broadcast_hdr->type, broadcast_hdr->len );

  if(broadcast_hdr->ver != GEOWARE_VERSION) {
  	return;
  }

  if(broadcast_hdr->type == GEOWARE_BROADCAST_LOC) {
  	// we know a broadcast packet was received so cast to it to use the fields
  	broadcast_pkt = (broadcast_pkt_t*) broadcast_hdr;
  	
    /* Check if we already know this neighbor. */
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      /* We break out of the loop if the address of the neighbor matches
         the address of the neighbor from which we received this
         broadcast message. */
      if(rimeaddr_cmp(&n->addr, from)) {
        break;
      }
    }

    /* If n is NULL, this neighbor was not found in our list, and we
       allocate a new struct neighbor from the neighbors_memb memory
       pool. */
    if(n == NULL) {
      n = memb_alloc(&neighbors_memb);

      /* If we could not allocate a new neighbor entry, we give up. We
         could have reused an old neighbor entry, but we do not do this
         for now. */
      if(n == NULL) {
        return;
      }

      /* Initialize the fields. */
      rimeaddr_copy(&n->addr, from);

      /* Place the neighbor on the neighbor list. */
      list_add(neighbors_list, n);
    }

    /* update the broadcast timestamp */
    n->timestamp = clock_seconds();

    /* update the expiration timer */
    ctimer_set(&n->ctimer, NEIGHBOR_TIMEOUT * CLOCK_SECOND, remove_neighbor, n);

  	// set number of neighbor's neighbors positions that we received
  	n->neighbors = broadcast_hdr->len;

  	// sanity check
  	if (n->neighbors > MAX_NEIGHBOR_NEIGHBORS) {
  	  n->neighbors = MAX_NEIGHBOR_NEIGHBORS;
  	}

  	// set neighbor's and its neighbors positions
  	for(i=0; i<= n->neighbors; i++) {
  	  n->pos[i] = broadcast_pkt->pos[i];
  	}

  	debug_printf("updated neighbor: %d.%d, ", n->addr.u8[0], n->addr.u8[1]);
  	print_pos(n->pos[0]);
  }

  else if (broadcast_hdr->type == GEOWARE_BROADCAST_SUB) {
    subscription_pkt = (subscription_pkt_t*) broadcast_hdr;

    process_incoming_subscription(&subscription_pkt->subscription);
  }
}
/*---------------------------------------------------------------------------*/
/* Declare the broadcast  structures */
static struct broadcast_conn broadcast;

/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  static struct etimer et;
  static broadcast_pkt_t broadcast_pkt;
  static subscription_pkt_t subscription_pkt;
  struct neighbor *n;
  uint8_t i;

  broadcast_subscription_event = process_alloc_event();

  broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);

  /* Send a broadcast every BROADCAST_PERIOD with a jitter of half of 
     BROADCAST_PERIOD */
  etimer_set(&et, \
    CLOCK_SECOND * \
    (BROADCAST_PERIOD/2 + random_rand()%(BROADCAST_PERIOD+1)));

  while(1) {

    PROCESS_WAIT_EVENT();

    if(etimer_expired(&et)) {
      // prepare the broadcast packet
      broadcast_pkt.hdr.ver = GEOWARE_VERSION;
      broadcast_pkt.hdr.type = GEOWARE_BROADCAST_LOC;
      broadcast_pkt.hdr.len = 0;  // number of neighbors
      broadcast_pkt.pos[0] = own_pos;

      // fill in the neighbors positions
      for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
        broadcast_pkt.pos[++broadcast_pkt.hdr.len] = n->pos[0];

        /* We break out of the loop if the max number of neigbor neighbors is
           reached */
        if(broadcast_pkt.hdr.len == MAX_NEIGHBOR_NEIGHBORS) {
          break;
        }
      }

      debug_printf("bcast s: %d, %d, %d\n", broadcast_pkt.hdr.ver, \
        broadcast_pkt.hdr.type, broadcast_pkt.hdr.len );

      /* log the time of the broadcast */
      debug_printf("[BC] @%lu\n", clock_seconds());

      packetbuf_copyfrom(&broadcast_pkt, \
        sizeof(geoware_hdr_t)+(1+broadcast_pkt.hdr.len)*sizeof(pos_t));

      broadcast_send(&broadcast);

      /* need to set it again here, couldnt think of a better solution.. */
      etimer_set(&et, \
        CLOCK_SECOND * \
        (BROADCAST_PERIOD/2 + random_rand()%(BROADCAST_PERIOD+1)));
    }

    if (ev == broadcast_subscription_event) {
      debug_printf("broadcasting subscription\n");
      subscription_pkt.hdr.ver = GEOWARE_VERSION;
      subscription_pkt.hdr.type = GEOWARE_BROADCAST_SUB;
      subscription_pkt.hdr.len = 0;
      subscription_pkt.subscription = *(subscription_t*) data;

      /* Copy the subscription to the packet buffer. */
      packetbuf_copyfrom(&subscription_pkt, sizeof(subscription_pkt_t));

      broadcast_send(&broadcast);
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/*
 * This function is called at the final recepient of the message.
 */
static void
recv(struct multihop_conn *c, const rimeaddr_t *sender,
     const rimeaddr_t *prevhop,
     uint8_t hops)
{
  geoware_hdr_t *multihop_hdr;
  subscription_pkt_t *subscription_pkt;

  printf("multihop message received. originator: %d.%d hops: %d\n", \
  	sender->u8[0], sender->u8[1], hops);

  multihop_hdr = packetbuf_dataptr();

  if(multihop_hdr->ver != GEOWARE_VERSION) {
    return NULL;
  }

  if (multihop_hdr->type == GEOWARE_SUBSCRIPTION) {
    subscription_pkt = (subscription_pkt_t*) multihop_hdr;
    process_incoming_subscription(&subscription_pkt->subscription);
  }
}
/*
 * This function is called to forward a packet. The function picks the
 * neighbor closest to the destination from the neighbor list and returns
 * its address. If no neighbor is closer than ourselfes, choose from neighbor's
 * neighbors. The multihop layer sends the packet to this address. If no
 * neighbor is found, the function returns NULL to signal to the multihop layer
 * that the packet should be dropped.
 */
static rimeaddr_t *
forward(struct multihop_conn *c,
	const rimeaddr_t *originator, const rimeaddr_t *dest,
	const rimeaddr_t *prevhop, uint8_t hops)
{
  /* Find neighbor closer to the destination to forward to. */
  struct neighbor *n;
  struct neighbor *closest = NULL;
	geoware_hdr_t *multihop_hdr;
	reading_hdr_t *reading_hdr;
  subscription_pkt_t *subscription_pkt;
  pos_t *destination;
  float proximity = EPSILON;
	uint8_t i;

	float min_dist = FLT_MAX;

  /* The packetbuf_dataptr() returns a pointer to the first data byte
     in the received packet. */
  multihop_hdr = packetbuf_dataptr();
  
 //  printf("multihop r: %d, %d, %d\n", multihop_hdr->ver, \
 //  	multihop_hdr->type, multihop_hdr->len );
 //  printf("prevhop: %d.%d\n", prevhop->u8[0], prevhop->u8[1]);
	// print_neighbors();

  if(multihop_hdr->ver != GEOWARE_VERSION || list_length(neighbors_list)==0) {
  	return NULL;
  }

	if(multihop_hdr->type == GEOWARE_READING) {
		reading_hdr = (reading_hdr_t*) multihop_hdr;
    destination = &reading_hdr->subscription_hdr.owner_pos;
  }
  else if (multihop_hdr->type == GEOWARE_SUBSCRIPTION) {
    subscription_pkt = (subscription_pkt_t*) multihop_hdr;
    destination = &subscription_pkt->subscription.center;
    proximity = subscription_pkt->subscription.radius;
  }

	/* Find distance to destination */
	min_dist = distance(own_pos, *destination);
	// printf("min_dist s: "PRINTFLOAT"\n", (long)min_dist, decimals(min_dist));

	/* check if we know a closer neighbor */
	for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
		// prevent passing back and forth, turns out that prevhop == 1.0
		// for the first hop, also hops is undefined..
		if(rimeaddr_cmp(&n->addr, prevhop) && \
        (packetbuf_attr(PACKETBUF_ATTR_HOPS) != 1)) {
			continue;
		}

  	float tmp_dist = distance(n->pos[0], *destination);
		
		// printf("%d.%d: ", n->addr.u8[0], n->addr.u8[1]);
		// print_pos(n->pos[0]);
		// printf("tmp_dist: "PRINTFLOAT"\n", (long)tmp_dist, decimals(tmp_dist));
  	
    // if the distance is less than some small value EPSILON it means we
    // found the destination/subscription owner, set it as packet destination
    if(tmp_dist < proximity) {
      packetbuf_set_addr(PACKETBUF_ADDR_ERECEIVER, &n->addr);
      closest = n;
      break;
    }

  	if(tmp_dist < min_dist) {
  		min_dist = tmp_dist;
  		closest = n;
  		// printf("min_dist t: "PRINTFLOAT"\n", (long)min_dist, decimals(min_dist));
  	}
	}

	if(closest == NULL) {
		/* we didn't find a closer neighbor, check neighbor's neighbors */
	  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
  		// prevent passing back and forth:
  		if(rimeaddr_cmp(&n->addr, prevhop)) {
  			continue;
  		}
    	for(i=1; i<=n->neighbors; i++){
    		// dont consider ourselves
    		if(pos_cmp(own_pos, n->pos[i])) {
    			continue;
    		}
	    	float tmp_dist = distance(n->pos[i], *destination);
	    	
	      // print_pos(n->pos[i]);
				// printf("tmp_dist: "PRINTFLOAT"\n", (long)tmp_dist, decimals(tmp_dist));

	    	if(tmp_dist < min_dist) {
	    		min_dist = tmp_dist;
	    		closest = n;
	    	}
	    }
  	}	
	}

	if(closest != NULL) {
	  printf("%d.%d: Forwarding packet to %d.%d (still "PRINTFLOAT" away), \
	  	hops %d\n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], \
	     closest->addr.u8[0], closest->addr.u8[1], (long)min_dist, \
	     decimals(min_dist), packetbuf_attr(PACKETBUF_ATTR_HOPS));
	  return &closest->addr;
	}

  // printf("%d.%d: did not find a neighbor to foward to\n",
	 // rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
  // return NULL;

  // didnt find anyone closer, nor anyone that knows someone closer
  // but we are in a dense network, so lets just try our luck
  // and forward to someone different than from whom we received
  do {
    int num = random_rand() % list_length(neighbors_list);
    i = 0;
    for(n = list_head(neighbors_list); n != NULL && i != num; \
      n = list_item_next(n)) i++;
  }
  while(rimeaddr_cmp(&n->addr, prevhop));

  printf("%d.%d: Randomly forwarding packet to %d.%d, hops %d\n", \
  	rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], \
    n->addr.u8[0], n->addr.u8[1], packetbuf_attr(PACKETBUF_ATTR_HOPS));
  return &n->addr;
}
/*---------------------------------------------------------------------------*/
/* Declare multihop structures */
static const struct multihop_callbacks multihop_call = {recv, forward};
static struct multihop_conn multihop;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multihop_process, ev, data)
{
  PROCESS_EXITHANDLER(multihop_close(&multihop);)
    
  PROCESS_BEGIN();

  static reading_hdr_t reading_hdr;
  static subscription_pkt_t subscription_pkt;
  pos_t owner_pos = {0.0, 0.0};
  static rimeaddr_t to;

  subscribe_event = process_alloc_event();

  /* Activate the button sensor. We use the button to drive traffic -
     when the button is pressed, a packet is sent. */
  SENSORS_ACTIVATE(button_sensor);

	/* Open a multihop connection on Rime channel MULTIHOP_CHANNEL. */
  multihop_open(&multihop, MULTIHOP_CHANNEL, &multihop_call);

  /* Set the Rime address of the final receiver of the packet to
     255.255 as we rely on the x,y coordinates to deliver the packet.
     once the recipient (distance to node is less then EPSILON)
     the address of the destination is updated. */
  to.u8[0] = 255;
  to.u8[1] = 255;

  /* Loop forever, send a packet when the button is pressed. */
  while(1) {
    

    /* Wait until we get a sensor event with the button sensor as data. */
    PROCESS_WAIT_EVENT();
    if (ev == sensors_event && data == &button_sensor) {
      printf("Creating multihop packet\n");

      // prepare the multihop packet
    	reading_hdr.hdr.ver = GEOWARE_VERSION;
    	reading_hdr.hdr.type = GEOWARE_READING;
    	reading_hdr.hdr.len = 0;
    	reading_hdr.subscription_hdr.owner_pos = owner_pos;

      /* Copy the reading to the packet buffer. */
      packetbuf_copyfrom(&reading_hdr, sizeof(reading_hdr_t));

  		print_neighbors();

      /* Send the packet. */
      multihop_send(&multihop, &to);
    }
    else if (ev == subscribe_event) {
      subscription_pkt.hdr.ver = GEOWARE_VERSION;
      subscription_pkt.hdr.type = GEOWARE_SUBSCRIPTION;
      subscription_pkt.hdr.len = 0;
      subscription_pkt.subscription = *(subscription_t*) data;

      /* Copy the subscription to the packet buffer. */
      packetbuf_copyfrom(&subscription_pkt, sizeof(subscription_pkt_t));

      /* Send the packet. */
      multihop_send(&multihop, &to);
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

uint16_t subscribe(sensor_t type, uint32_t period, \
    uint8_t aggr_type, uint8_t aggr_num, pos_t center, \
    float radius) {

  // create and fill the subscription structure
  static subscription_t new_sub;
  new_sub.subscription_hdr.sID = random_rand() % UINT16_MAX;
  new_sub.subscription_hdr.owner_pos = own_pos;
  new_sub.type = type;
  new_sub.period = period;
  new_sub.aggr_type = aggr_type;
  new_sub.aggr_num = aggr_num;
  new_sub.center = center;
  new_sub.radius = radius;

  // send out the news
  process_post(&multihop_process, subscribe_event, (void*) &new_sub);

  return new_sub.subscription_hdr.sID;
}
void unsubscribe(uint16_t type) {
}

// send an updated value to the subscription owner
void publish(uint16_t sID) {
  
}



/*---------------------------------------------------------------------------*/
PROCESS_THREAD(boot_process, ev, data)
{
  PROCESS_BEGIN();
  
  // get the node coordinates from cooja, found at:
  // https://sourceforge.net/p/contiki/mailman/message/34696644/
  printf("Waiting for node coordinates from Cooja..\n");
  PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
  own_pos.x = stof((char *)data);
  PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
  own_pos.y = stof((char *)data);
  
  print_pos(own_pos);

  // just a test
  pos_t origin;
  float d = distance(own_pos, origin);
  printf("Distance from origin: "PRINTFLOAT"\n", (long)d, decimals(d));
  printf("size of subscription: %d\n", sizeof(subscription_t));
  printf("size of broadcast_pkt_t: %d\n", sizeof(broadcast_pkt_t));
  

  // init shell
  serial_shell_init();
  // and shell commands
  commands_init();
  /* Initialize the memory for the neighbor table entries. */
  memb_init(&neighbors_memb);
  /* Initialize the list used for the neighbor table. */
  list_init(neighbors_list);
  /* Initialize the memory for the reading entries. */
  memb_init(&readings_memb);
  /* Initialize the list used for the sensor readings. */
  list_init(readings_list);
  // start broadcast process
  process_start(&broadcast_process, NULL);
  // start runicast process
  process_start(&multihop_process, NULL);

  printf("%d.%d: Booting completed.\n", \
  	rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);

  // int i;
  // for(i=0; i<1000; i++) {
  // 	float temp = get_temperature();
  // 	uint16_t light = get_light();
  // 	uint8_t humidity = get_humidity();

  // 	printf("temp: "PRINTFLOAT"C light: %u lux humidity: %d %% \n", \
  // 		(long)temp, decimals(temp), light, humidity);
  // }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
