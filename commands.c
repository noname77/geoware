#include "contiki.h"
#include "shell.h"
#include "dev/cc2420.h"

#include <stdlib.h>

#include "commands.h"

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

void
commands_init()
{
  // contiki commands
  shell_reboot_init();
  
  // own commands
  shell_register_command(&radio_power_command);
}
