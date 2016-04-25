#include "geoware.h"

/*---------------------------------------------------------------------------*/

void
process_subscription(subscription_pkt_t *sub_pkt)
{
  struct neighbor *n;

  subscription_t *subscription = &sub_pkt->subscription;

  /* check if we are already subscribed or already seen this subscription */
  if(is_subscribed(subscription->subscription_hdr.sID) || \
      was_seen(subscription->subscription_hdr.sID)) {
    return;
  }

  /* check if we are in the region of interest */
  if (distance(own_pos, subscription->center) <= subscription->radius) {
    /* add the subscription */
    // TODO: what if we dont support the given sensor type?
    subscription_t *new_sub = add_subscription(subscription);

    // rebroadcast
    // TODO: only if we havent previously requested
    // TODO: what if we didnt have enough space to add new subscription but 
    // would like to forward?
    // posting synchronously to avoid having to copy buffers
    printf("firework: %s\n", sub_pkt->hdr.firewrk ? "true" : "false");
    if(new_sub != NULL && sub_pkt->hdr.firewrk) {
      process_post_synch(&broadcast_process, broadcast_subscription_event, \
        (void*)sub_pkt);
    }
  }
  else {
    /* check if we know any neighbors that could be in the region of 
       the interest */
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      /* find the distance to the center of interest for current neighbor */
      float tmp_dist = distance(n->pos[0], subscription->center);

      if(tmp_dist <= subscription->radius) {
        break;
      }
    }

    if(n != NULL) {
      /* add subscription to the seen list */
      add_seen_sub(subscription->subscription_hdr.sID);
      
      process_post_synch(&broadcast_process, broadcast_subscription_event, \
        (void*)sub_pkt);
    }
  }
}

/*---------------------------------------------------------------------------*/

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

    if(unsub_pkt->hdr.firewrk) {
      process_post_synch(&broadcast_process, broadcast_unsubscription_event, \
        (void*)unsub_pkt);
    }
  }
  else if(was_seen(unsub_pkt->sID)) {
    remove_seen_sub(unsub_pkt->sID);

    if(unsub_pkt->hdr.firewrk) {
      process_post_synch(&broadcast_process, broadcast_unsubscription_event, \
        (void*)unsub_pkt);
    }
  }
}

/*---------------------------------------------------------------------------*/

uint8_t
prepare_sub_pkt(subscription_pkt_t *sub_pkt, sid_t sID)
{
  subscription_t *sub = get_subscription(sID);

  if(sub != NULL) {
    sub_pkt->hdr.ver = GEOWARE_VERSION;
    sub_pkt->hdr.type = GEOWARE_SUBSCRIPTION;
    sub_pkt->hdr.len = 0;
    sub_pkt->hdr.pos = own_pos;
    sub_pkt->hdr.firewrk = 1;
    sub_pkt->subscription = *sub;
  }

  return sub != NULL;
}

/*---------------------------------------------------------------------------*/

uint8_t
prepare_unsub_pkt(unsubscription_pkt_t *unsub_pkt, sid_t sID)
{
  subscription_t *sub = get_subscription(sID);

  if(sub != NULL) {
    unsub_pkt->hdr.ver = GEOWARE_VERSION;
    unsub_pkt->hdr.type = GEOWARE_UNSUBSCRIPTION;
    unsub_pkt->hdr.len = 0;
    unsub_pkt->hdr.pos = own_pos;
    unsub_pkt->hdr.firewrk = 1;
    unsub_pkt->sID = sub->subscription_hdr.sID;
    unsub_pkt->center = sub->center;
    unsub_pkt->radius = sub->radius;
  }

  return sub != NULL;
}

/*---------------------------------------------------------------------------*/

void
print_unsubscription(unsubscription_pkt_t *unsub_pkt)
{
  printf("sID: %u\n", unsub_pkt->sID);
  printf("center: ");
  print_pos(unsub_pkt->center);
  printf("radius: "PRINTFLOAT"\n", (long)unsub_pkt->radius, \
    decimals(unsub_pkt->radius));
}

/*---------------------------------------------------------------------------*/