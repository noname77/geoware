#include "contiki.h"

#include <stdio.h> /* For printf() */
#include <float.h>  /* for FLT_MAX */

#include "geoware.h"

/* Uncomment below line to include debug output */
// #define DEBUG_PRINTS

#ifdef DEBUG_PRINTS
#define debug_printf printf
#else
#define debug_printf(format, args...)
#endif

#define MY_MACRO MY_MACRO_COUNTED(__COUNTER__)
#define MY_MACRO_COUNTED(counter) counter + counter

/* This structure holds information about seen but not active subscriptions. */
struct seen_sub {
  /* The ->next pointer is needed since we are placing these
     on a Contiki list. */
  struct seen_sub *next;

  /* -> sID holds the id of seen subscription */
  sid_t sID;
};

/* This MEMB() definition defines a memory pool from which we allocate
   subscription entries. */
MEMB(seen_subs_memb, struct seen_sub, MAX_ACTIVE_SUBSCRIPTIONS);

/* The seen_subscriptions is a Contiki list that holds what it says. */
LIST(seen_subs);

sid_t
add_seen_sub(sid_t sID)
{
  struct seen_sub *new_sub;

  new_sub = memb_alloc(&seen_subs_memb);

  /* If we could not allocate a new neighbor entry, we give up. We
     could have reused an old neighbor entry, but we do not do this
     for now. */
  if(new_sub == NULL) {
    debug_printf("Seen subscriptions list full\n");
    return 0;
  }

  /* Initialize the fields. */
  new_sub->sID = sID;

  /* Place the subscription on the active_subscriptions list. */
  list_add(seen_subs, new_sub);

  debug_printf("seen sub added: %u\n", new_sub->sID);

  return new_sub->sID;
}

sid_t
remove_seen_sub(sid_t sID)
{
  struct seen_sub *s;

  for(s = list_head(seen_subs); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(sID == s->sID) {
      break;
    }
  }

  if(s != NULL) {
    printf("removing seen subscription %u\n", sID);

    list_remove(seen_subs, s);
    memb_free(&seen_subs_memb, s);

    return sID;
  }
  
  printf("wasn't seen: %u\n", sID);
  return 0;
}

uint8_t was_seen(sid_t sID) {
  struct seen_sub *s;

  for(s = list_head(seen_subs); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(sID == s->sID) {
      break;
    }
  }

  return s != NULL;
}

/* This MEMB() definition defines a memory pool from which we allocate
   subscription entries. */
MEMB(subscriptions_memb, struct subscription, MAX_ACTIVE_SUBSCRIPTIONS);

/* The active_subscriptions is a Contiki list that holds what it says. */
LIST(active_subscriptions);

/* Check if we already subscribed to sID. */
uint8_t
is_subscribed(sid_t sID)
{
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

subscription_t*
add_subscription(subscription_t *sub)
{
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

  /* set the sensor reading callback timer, only if we are not the owner */
  if(distance(sub->subscription_hdr.owner_pos, own_pos) != 0) {
    ctimer_set(&new_sub->callback, new_sub->sub.period * CLOCK_SECOND / 1000, sensor_read, (void*) new_sub);
  }
  else {
    /* otherwise set the process that called us to be able to send the
       received readings back to it */
    new_sub->proc = PROCESS_CURRENT();
  }

  /* Place the subscription on the active_subscriptions list. */
  list_add(active_subscriptions, new_sub);

  debug_printf("subscription added: %u\n", new_sub->sub.subscription_hdr.sID);

  return &new_sub->sub;
}

subscription_t*
get_subscription(sid_t sID)
{
  struct subscription *s;
  for(s = list_head(active_subscriptions); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(sID == s->sub.subscription_hdr.sID) {
      return &s->sub;
    }
  }

  return NULL;
}

struct subscription*
get_subscription_struct(sid_t sID)
{
  struct subscription *s;
  for(s = list_head(active_subscriptions); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(sID == s->sub.subscription_hdr.sID) {
      return s;
    }
  }

  return NULL; 
};

sid_t
remove_subscription(sid_t sID)
{
  struct subscription *s;

  for(s = list_head(active_subscriptions); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(sID == s->sub.subscription_hdr.sID) {
      break;
    }
  }

  if(s != NULL) {
    printf("removing subscription %u\n", sID);

    ctimer_stop(&s->callback);
    list_remove(active_subscriptions, s);
    memb_free(&subscriptions_memb, s);

    return sID;
  }
  
  printf("wasn't subscribed: %u\n", sID);
  return 0;
}

void
process_subscription(subscription_pkt_t *sub_pkt)
{
  struct neighbor *n;

  subscription_t *subscription = &sub_pkt->subscription;

  // check if we are already subscribed or already seen this subscription
  if(is_subscribed(subscription->subscription_hdr.sID) || \
      was_seen(subscription->subscription_hdr.sID)) {
    return;
  }

  // check if we are in the region of interest
  if (distance(own_pos, subscription->center) <= subscription->radius) {
    // add the subscription
    // TODO: what if we dont support the given sensor type?
    subscription_t *new_sub = add_subscription(subscription);

    // rebroadcast
    // TODO: only if we havent previously requested
    // TODO: what if we didnt have enough space to add new subscription but 
    // would like to forward?
    // posting synchronously to avoid having to copy buffers
    if(new_sub != NULL) {
      process_post_synch(&broadcast_process, broadcast_subscription_event, \
        NULL);
    }
  }
  else {
    // check if we know any neighbors that could be in the region of the interest
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      // find the distance to the center of interest for current neighbor
      float tmp_dist = distance(n->pos[0], subscription->center);

      if(tmp_dist <= subscription->radius) {
        break;
      }
    }

    if(n != NULL) {
      // add subscription to the seen list
      add_seen_sub(subscription->subscription_hdr.sID);
      
      process_post_synch(&broadcast_process, broadcast_subscription_event, \
        NULL);
    }
  }


}

void
process_unsubscription(unsubscription_pkt_t *unsub_pkt)
{
  // check if we are in the region of interest
  // if (distance(own_pos, unsubscription_pkt->center) > unsubscription_pkt->radius) {
  //   return;
  // }

  // printf("processing unsubscription:\n");
  // print_unsubscription(unsub_pkt);

  if(is_subscribed(unsub_pkt->sID)) {
    /* Remove the subscription */
    remove_subscription(unsub_pkt->sID);

    process_post_synch(&broadcast_process, broadcast_unsubscription_event, \
      NULL);
  }
  else if(was_seen(unsub_pkt->sID)) {
    remove_seen_sub(unsub_pkt->sID);

    process_post_synch(&broadcast_process, broadcast_unsubscription_event, \
      NULL);
  }
}


uint8_t
prepare_unsub_pkt(unsubscription_pkt_t *unsub_pkt, sid_t sID)
{
  subscription_t *sub = get_subscription(sID);

  if(sub != NULL) {
    unsub_pkt->hdr.ver = GEOWARE_VERSION;
    unsub_pkt->hdr.type = GEOWARE_UNSUBSCRIPTION;
    unsub_pkt->hdr.len = 0;
    unsub_pkt->hdr.pos = own_pos;
    unsub_pkt->sID = sub->subscription_hdr.sID;
    unsub_pkt->center = sub->center;
    unsub_pkt->radius = sub->radius;
  }

  return sub != NULL;
}

void
print_subscription(subscription_t *sub)
{
  printf("sID: %u\n", sub->subscription_hdr.sID);
  printf("owner pos: ");
  print_pos(sub->subscription_hdr.owner_pos);
  printf("type: %u\n", sub->type);
  printf("period: %lu\n", sub->period);
  printf("center: ");
  print_pos(sub->center);
  printf("radius: "PRINTFLOAT"\n", (long)sub->radius, decimals(sub->radius));
}

void
print_unsubscription(unsubscription_pkt_t *unsub_pkt)
{
  printf("sID: %u\n", unsub_pkt->sID);
  printf("center: ");
  print_pos(unsub_pkt->center);
  printf("radius: "PRINTFLOAT"\n", (long)unsub_pkt->radius, decimals(unsub_pkt->radius));
}