#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include "node.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(computation_process, "computation_process");
AUTOSTART_PROCESSES(&computation_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    if(len == sizeof(unsigned)) {
        unsigned count;
        memcpy(&count, data, sizeof(count));
        LOG_INFO("Received %u from ", count);
        LOG_INFO_LLADDR(src);
        LOG_INFO_("\n");
    }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(computation_process, ev, data)
{
static struct etimer periodic_timer;
static unsigned count = 0;

PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */

/* Initialize NullNet */
nullnet_buf = (uint8_t *)&count;
nullnet_len = sizeof(count);
nullnet_set_input_callback(input_callback);

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

PROCESS_END();
}