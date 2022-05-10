#include "contiki.h"
#include "node.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <stdio.h>

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

  uint8_t payload[64] = { 0 };
  nullnet_buf = payload;
  nullnet_len = 2;          //in byte
  NETSTACK_NETWORK.output(NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/