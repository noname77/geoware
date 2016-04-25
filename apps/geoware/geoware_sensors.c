#include <stdio.h>  /* For printf() */
#include <float.h>  /* for FLT_MAX */

#include "geoware.h"

#ifdef DEBUG_PRINTS
#define debug_printf printf
#else
#define debug_printf(format, args...)
#endif

/*---------------------------------------------------------------------------*/
/* This structure holds information about sensor readings. */
struct reading {
  /* The ->next pointer is needed since we are placing these
     on a Contiki list. */
  struct reading *next;

  /* type of the sensor / reading */
  sensor_t type;

  /* subscription id, used only on gatway 
     TODO: do i need type if i use sID instead on sensing nodes? */
  sid_t sID;

  /* the node which collected this reading, used only on gateway */
  rimeaddr_t owner;

  /* The ->value field (union) holds the actual reading value */
  reading_val value;
};

/*---------------------------------------------------------------------------*/
/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
MEMB(readings_memb, struct reading, MAX_READINGS);

/* The readings_list is a Contiki list that holds past sensor readings. */
LIST(readings_list);

MEMB(sensors_memb, struct sensor, MAX_SENSORS);

LIST(sensors_list);

void
remove_reading_type(sensor_t t)
{
  struct reading *r;

  for(r = list_head(readings_list); r != NULL; r = list_item_next(r)) {
    /* We break out of the loop if the sensor type in quesiton matches current
       readings type. */
    if(t == r->type) {
      break;
    }
  }

  if(r != NULL) {
    debug_printf("removing old %s reading\n", get_mapping(t)->strname);

    list_remove(readings_list, r);
    memb_free(&readings_memb, r);
  }
  else {
    debug_printf("no old %s readings\n", get_mapping(t)->strname);
  }
}

/*---------------------------------------------------------------------------*/

void
sensor_read(void *s)
{
  struct subscription *sub;
  struct reading *new_reading;
  mapping_t *mapping;
  reading_val value;
  sid_t sID;

  sub = (struct subscription*) s;
  mapping = get_mapping(sub->sub.type);
  sID = sub->sub.subscription_hdr.sID;

  if(mapping == NULL) {
    printf("sensor not registered.\n");
    return;
  }

  if(mapping->read == NULL) {
    printf("sensor not supproted.\n");
    return;
  }

  // fire repeatedly
  ctimer_reset(&sub->callback);

  /* allocate space for the new reading */
  new_reading = memb_alloc(&readings_memb);

  /* If we could not allocate a new reading entry, remove
     an old entry of the same type and try again */
  if(new_reading == NULL) {
    debug_printf("Readings list full\n");

    //remove oldest entry of same sid
    remove_reading_sid(sID);
    
    // allocate space again
    new_reading = memb_alloc(&readings_memb);

    if (new_reading == NULL) {
      // give up the second time
      return;
    }
  }

  /* Initialize the fields. */
  new_reading->type = mapping->s;
  new_reading->sID = sID;

  printf("new %s reading: ", mapping->strname);
  // get the reading
  switch(mapping->r) {
    case UINT8:
      value.ui8 = ((uint8_t (*)())mapping->read)();
      printf("%u\n", value.ui8);
      break;
    case UINT16:
      value.ui16 = ((uint16_t (*)())mapping->read)();
      printf("%u\n", value.ui16);
      break;
    case FLOAT:
      value.fl = ((float (*)())mapping->read)();
      printf(PRINTFLOAT"\n", (long)value.fl, decimals(value.fl));
      break;
  }

  // add the new reading to the reading list
  new_reading->value = value;

  /* Place the new reading on the readings list. */
  list_add(readings_list, new_reading);

  if(++sub->num >= sub->sub.aggr_num) {
  	reading_owned aggr;
  	sub->num = 0;

  	if(sub->sub.aggr_type > 0) {
  		aggr = get_aggregate(sID)->func(sID, sub->sub.aggr_num);
  	}
  	else {
  		aggr = get_reading_sid(sID);
  	}
  	
  	publish(sID, aggr.value);	
  }
}

/*---------------------------------------------------------------------------*/

reading_val
get_reading_type(sensor_t t)
{
  struct reading *r;
  reading_val value;

  for(r = list_head(readings_list); r != NULL; r = list_item_next(r)) {
    /* We break out of the loop if the sensor type in quesiton matches current
       readings type. */
    if(t == r->type) {
      value = r->value;
      break;
    }
  }

  if(r != NULL) {
    debug_printf("removing old %s reading\n", get_mapping(t)->strname);

    list_remove(readings_list, r);
    memb_free(&readings_memb, r);
  }
  else {
    debug_printf("no old %s readings\n", get_mapping(t)->strname);
    value.fl = FLT_MAX;
  }

  return value;
}

/*---------------------------------------------------------------------------*/

void
remove_reading_sid(sid_t sID)
{
  struct reading *r;

  for(r = list_head(readings_list); r != NULL; r = list_item_next(r)) {
    /* We break out of the loop if the sensor type in quesiton matches current
       readings type. */
    if(sID == r->sID) {
      break;
    }
  }

  if(r != NULL) {
    debug_printf("removing old reading for sID: %u\n", sID);

    list_remove(readings_list, r);
    memb_free(&readings_memb, r);
  }
  else {
    debug_printf("no old readings with sID: %u\n", sID);
  }
}

/*---------------------------------------------------------------------------*/

uint8_t
reading_add(sid_t sID, rimeaddr_t* owner, reading_val* value)
{
  struct reading *new_reading;

  /* allocate space for the new reading */
  new_reading = memb_alloc(&readings_memb);

  /* If we could not allocate a new reading entry, remove
     an old entry of the same type and try again */
  if(new_reading == NULL) {
    debug_printf("Readings list full\n");

    //remove oldest entry of same sID?
    remove_reading_sid(sID);
    
    // allocate space again
    new_reading = memb_alloc(&readings_memb);

    if (new_reading == NULL) {
      // give up the second time
      return 0;
    }
  }

  /* Initialize the type field. */
  new_reading->type = get_subscription(sID)->type;

  new_reading->sID = sID;

  new_reading->owner = *owner;

  // add the new reading to the reading list
  new_reading->value = *value;

  /* Place the new reading on the readings list. */
  list_add(readings_list, new_reading);

  return 1;
}

/*---------------------------------------------------------------------------*/

reading_owned
get_reading_sid(sid_t sID)
{
  struct reading *r;
  reading_owned value;

  for(r = list_head(readings_list); r != NULL; r = list_item_next(r)) {
    /* We break out of the loop if the sensor type in quesiton matches current
       readings type. */
    if(sID == r->sID) {
      value.value = r->value;
      value.owner = r->owner;
      break;
    }
  }

  if(r != NULL) {
    // debug_printf("removing old reading of sID: %u\n", sID);

    list_remove(readings_list, r);
    memb_free(&readings_memb, r);
  }
  else {
    debug_printf("no old readings of sID: %u\n", sID);
    value.value.fl = FLT_MAX;
  }

  return value;
}

/*---------------------------------------------------------------------------*/

mapping_t*
get_mapping(sensor_t type)
{
  struct sensor *s;
  for(s = list_head(sensors_list); s != NULL; s = list_item_next(s)) {
    /* We break out of the loop if the sID in quesiton matches current
       subscription. */
    if(s->mapping->s == type) {
      return s->mapping;
    }
  }

  return NULL;
}

/*---------------------------------------------------------------------------*/

void
sensor_add(mapping_t* sensor)
{
	list_add(sensors_list, memb_alloc(&sensors_memb));
	((struct sensor*)list_tail(sensors_list))->mapping = sensor;
}

/*---------------------------------------------------------------------------*/

void
sensors_init()
{
  memb_init(&sensors_memb);
  list_init(sensors_list);
    /* Initialize the memory for the reading entries. */
  memb_init(&readings_memb);
  /* Initialize the list used for the sensor readings. */
  list_init(readings_list);
}

/*---------------------------------------------------------------------------*/