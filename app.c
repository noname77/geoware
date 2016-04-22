#include "contiki.h"
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

  static sid_t id;

  sensors_init();

  sensor_init(TEMPERATURE);
  sensor_init(LIGHT);
  sensor_init(HUMIDITY);

  // start the subscription if we are the sink
  if(rimeaddr_node_addr.u8[0] == 1) {
	  pos_t center = {20.0, 20.0};
	  float radius = 11;

	  id = subscribe(HUMIDITY, 2000, 0, 0, center, radius);
  }

  while(1) {
  	PROCESS_WAIT_EVENT();

  	if(event == geoware_reading_event) {
  		printf("received reading\n");
  	}
	}
  // geoware_init();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
