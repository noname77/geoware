#include "contiki.h"
#include "shell.h"
#include "dev/cc2420.h"

#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "geoware.h"

PROCESS(radio_power_process, "Change radio tx power");
SHELL_COMMAND(radio_power_command, "txp", "txp: change transmit power", &radio_power_process);
/* --------------------------------- */
PROCESS_THREAD(radio_power_process, ev, data) {
PROCESS_BEGIN();
  uint8_t txp = (uint8_t) atoi((char *)data);
  static char shell_out[4];

  if(txp > CC2420_TXPOWER_MAX) {
  	txp = CC2420_TXPOWER_MAX;
  }

  cc2420_set_txpower(txp);

  snprintf(shell_out, sizeof(shell_out), "%d", txp);

  shell_output_str(&radio_power_command, "TX power: ", shell_out);
PROCESS_END();
}

PROCESS(subscribe_process, "Subscribe process");
SHELL_COMMAND(subscribe_command, "sub", "sub: subscribe to a sensor reading", &subscribe_process);
/* --------------------------------- */
PROCESS_THREAD(subscribe_process, ev, data) {
PROCESS_BEGIN();
  static char shell_out[6];
  sid_t id;

  pos_t center = {20.0, 20.0};
  float radius = 11;

  id = subscribe(HUMIDITY, 1000, 0, 0, center, radius);

  snprintf(shell_out, sizeof(shell_out), "%u", id);

  shell_output_str(&radio_power_command, "subscribed, id: ", shell_out);
PROCESS_END();
}

PROCESS(unsubscribe_process, "Unubscribe process");
SHELL_COMMAND(unsubscribe_command, "usub", "usub: unsubscribe from a sensor reading", &unsubscribe_process);
/* --------------------------------- */
PROCESS_THREAD(unsubscribe_process, ev, data) {
PROCESS_BEGIN();
  static char shell_out[6];
  sid_t id;

  id = atoi((char*) data);

  unsubscribe(id);

  snprintf(shell_out, sizeof(shell_out), "%u", id);

  shell_output_str(&radio_power_command, "unsubscribed, id: ", shell_out);
PROCESS_END();
}

void
commands_init()
{
  // contiki commands
  shell_reboot_init();
  
  // own commands
  shell_register_command(&radio_power_command);
  shell_register_command(&subscribe_command);
  shell_register_command(&unsubscribe_command);
}
