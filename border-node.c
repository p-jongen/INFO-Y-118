#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"

#define SEND_INTERVAL (8 * CLOCK_SECOND)

static short static_rank;

PROCESS(border_process, "Border router node");
AUTOSTART_PROCESSES(&border_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();
  printf("[BORDER NODE] Starting unicast and broadcast...");
  static_rank = 1;

    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        LOG_INFO("Sending %u to ", count);
        LOG_INFO_LLADDR(NULL);
        LOG_INFO_("\n");

        memcpy(nullnet_buf, &count, sizeof(count));
        nullnet_len = sizeof(count);

        NETSTACK_NETWORK.output(NULL);
        count++;
        etimer_reset(&periodic_timer);
    }
    /*
  uint8_t payload[64] = { 0 };
  nullnet_buf = payload;
  nullnet_len = 2;
  NETSTACK_NETWORK.output(NULL);
*/
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/