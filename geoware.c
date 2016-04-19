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

#include "contiki.h"
#include "net/rime.h"
#include "dev/serial-line.h"
#include "lib/random.h"
#include "serial-shell.h"

#include <stdio.h> /* For printf() */

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

// nodes position
static pos_t own_pos;

subscription_t subscriptions[MAX_ACTIVE_SUBSCRIPTIONS];

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

/* This function is called whenever a broadcast message is received. */
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  struct neighbor *n;
  geoware_hdr_t *broadcast_hdr;
  broadcast_t *broadcast_pkt;
  uint8_t i;

  /* The packetbuf_dataptr() returns a pointer to the first data byte
     in the received packet. */
  broadcast_hdr = packetbuf_dataptr();
  
  printf("bcast r: %d, %d, %d\n", broadcast_hdr->ver, \
  	broadcast_hdr->type, broadcast_hdr->len );

  if(broadcast_hdr->ver != GEOWARE_VERSION) {
  	return;
  }

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

  if(broadcast_hdr->type == GEOWARE_BROADCAST) {
  	// we know a broadcast packet was received so cast to it to use the fields
  	broadcast_pkt = (broadcast_t*) broadcast_hdr;
  	
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
}
/*---------------------------------------------------------------------------*/
/* Declare the broadcast  structures */
static struct broadcast_conn broadcast;

/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
PROCESS(broadcast_process, "Broadcast process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  static struct etimer et;
  static broadcast_t broadcast_pkt;
  struct neighbor *n;
  uint8_t i;

  broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);

  while(1) {
    /* Send a broadcast every BROADCAST_PERIOD with a jitter of half of 
       BROADCAST_PERIOD */
    etimer_set(&et, \
    	CLOCK_SECOND * \
    	(BROADCAST_PERIOD/2 + random_rand()%(BROADCAST_PERIOD+1)));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // prepare the broadcast packet
  	broadcast_pkt.hdr.ver = GEOWARE_VERSION;
  	broadcast_pkt.hdr.type = GEOWARE_BROADCAST;
  	broadcast_pkt.hdr.len = 0;	// number of neighbors
  	broadcast_pkt.pos[0] = own_pos;

	for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
	  broadcast_pkt.pos[broadcast_pkt.hdr.len++] = n->pos[0];

	  /* We break out of the loop if the max number of neigbor neighbors is
	     reached */
	  if(broadcast_pkt.hdr.len == MAX_NEIGHBOR_NEIGHBORS) {
	    break;
	  }
	}

  	printf("bcast s: %d, %d, %d\n", broadcast_pkt.hdr.ver, \
  		broadcast_pkt.hdr.type, broadcast_pkt.hdr.len );

    packetbuf_copyfrom(&broadcast_pkt, \
    	sizeof(geoware_hdr_t)+(1+broadcast_pkt.hdr.len)*sizeof(pos_t));

    /* log the time of the broadcast */
    printf("[BC] @%lu\n", clock_seconds());

    broadcast_send(&broadcast);
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

/* This function is called for every incoming runicast packet. */
static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  debug_printf("runicast message received from %d.%d, at: %lu\n",
       from->u8[0], from->u8[1], clock_seconds());
}

/*---------------------------------------------------------------------------*/

/* callback on ACK reception */
static void
sent_runicast(struct runicast_conn *c, const rimeaddr_t *to, \
	uint8_t retransmissions)
{
  debug_printf("runicast message sent to %d.%d, at: %lu, retransmissions %d\n",
       to->u8[0], to->u8[1], clock_seconds(), retransmissions);
}

/*---------------------------------------------------------------------------*/

/* callback on delivery timeout */
static void
timedout_runicast(struct runicast_conn *c, const rimeaddr_t *to, \
	uint8_t retransmissions)
{
  debug_printf("runicast message timed out when sending to %d.%d, \
  	retransmissions %d\n",
         to->u8[0], to->u8[1], retransmissions);

  // TODO: remove the neighbor from list
}

/*---------------------------------------------------------------------------*/
/* Declare runicast structures */
static struct runicast_conn runicast;
static const struct runicast_callbacks runicast_callbacks = { \
	recv_runicast, sent_runicast, timedout_runicast };
PROCESS(runicast_process, "Runicast process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(runicast_process, ev, data)
{
  PROCESS_EXITHANDLER(runicast_close(&runicast);)
    
  PROCESS_BEGIN();

  static struct etimer et;
  static struct neighbor *n;

  runicast_open(&runicast, RUNICAST_CHANNEL, &runicast_callbacks);

  while(1) {
    etimer_set(&et, CLOCK_SECOND * random_rand()%3);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // debug_printf("can send runicast here\n");
    // packetbuf_copyfrom(&mv, sizeof(dtn_header) + no*sizeof(dtn_message));
    // runicast_send(&runicast, &n->addr, MAX_RETRANSMISSIONS);
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/




uint16_t subscribe(uint8_t type, uint32_t period, \
    uint8_t aggr_type, uint8_t aggr_num) {
  return 0;
}
void unsubscribe(uint16_t type) {
}
void publish(uint16_t sID) {
}


/*---------------------------------------------------------------------------*/
PROCESS(boot_process, "Boot process");
AUTOSTART_PROCESSES(&boot_process);
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
  printf("size of broadcast_t: %d\n", sizeof(broadcast_t));
  

  // init shell
  serial_shell_init();
  // and shell commands
  commands_init();
  // start broadcast process
  process_start(&broadcast_process, NULL);
  // start runicast process
  process_start(&runicast_process, NULL);

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
