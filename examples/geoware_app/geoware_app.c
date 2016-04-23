#include "contiki.h"
#include "net/rime.h"

#include "geoware.h"
#include "fake_sensors.h"

SENSOR_CREATE(TEMPERATURE, "temperature", FLOAT, get_temperature);
SENSOR_CREATE(LIGHT, "light", UINT16, get_light);
SENSOR_CREATE(HUMIDITY, "humidity", UINT8, get_humidity);


PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data)
{
  PROCESS_BEGIN();

	static struct etimer et;
  static sid_t id;

  sensors_init();

  sensor_init(TEMPERATURE);
  sensor_init(LIGHT);
  sensor_init(HUMIDITY);

  geoware_init();

  printf("app started\n");

  // wait for 15 seconds to let the network settle
  etimer_set(&et, CLOCK_SECOND * 60);

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  // start the subscription if we are the sink
  if(rimeaddr_node_addr.u8[0] == 1) {
	  pos_t center = {20.0, 20.0};
	  float radius = 11;

	  // id = subscribe(HUMIDITY, 5000, 0, 0, center, radius);

	  // printf("subscribed to %u\n", id);
  }

  while(1) {
  	PROCESS_WAIT_EVENT();

  	if(ev == geoware_reading_event) {
  		printf("received reading\n");
  	}
	}

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
