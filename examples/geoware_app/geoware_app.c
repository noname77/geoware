#include "contiki.h"
#include "net/rime.h"

#include <stdio.h>  /* For printf() */
#include <float.h>  /* for FLT_MAX */

#include "geoware.h"
#include "fake_sensors.h"
#include "aggr.h"

SENSOR_CREATE(TEMPERATURE, "temperature", FLOAT, get_temperature);
SENSOR_CREATE(LIGHT, "light", UINT16, get_light);
SENSOR_CREATE(HUMIDITY, "humidity", UINT8, get_humidity);

AGGREGATE_CREATE(MAXIMUM, maximum);
AGGREGATE_CREATE(AVERAGE, average);


PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data)
{
  PROCESS_BEGIN();

	static struct etimer et;
  static sid_t id;

  // initializes the sensors list
  sensors_init();

  // initialize the sensors (adds them to a list)
  sensor_init(TEMPERATURE);
  sensor_init(LIGHT);
  sensor_init(HUMIDITY);

  // initializes the aggregate functions list
  aggregates_init();

  // initialize the aggregate functions (adds them to a list)
  aggr_init(MAXIMUM);
  aggr_init(AVERAGE);

  // starts the middleware process
  geoware_init();

  printf("app started\n");

  // wait for 1 minute to let the network settle
  etimer_set(&et, CLOCK_SECOND * 90);

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  // start the subscription if we are the sink
  if(rimeaddr_node_addr.u8[0] == 1) {
	  pos_t center = {20.0, 20.0};
	  float radius = 11;

    id = subscribe(HUMIDITY, 5000, AVERAGE, 10, center, radius);
    printf("subscribed to %u\n", id);
  }

  while(1) {
  	PROCESS_WAIT_EVENT();

  	if(ev == geoware_reading_event) {
      if (data == NULL){
        continue;
      }

      sid_t sID = *((sid_t*)data);

      // check if the subscription matches what we are interested in
      if(sID == id) {
        reading_owned new_value;
        int i = 0;
        // read all the readings present in the list for given sid
        while(HAS_MORE_READINGS(new_value = get_reading_sid(sID))) {
          printf("%d: received reading from: %d.%d sID: %u value: %u\n", \
           i++, new_value.owner.u8[0], new_value.owner.u8[1],\
           sID, new_value.value.ui8);
        }
      }
  	}
	}

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
