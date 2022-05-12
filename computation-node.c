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
    LOG_INFO("Into input_call Computation node");
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

PROCESS_BEGIN();

nullnet_set_input_callback(input_callback);
etimer_set(&periodic_timer, SEND_INTERVAL);

while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO("Into the While(1) Compute");
    LOG_INFO_("\n");


    NETSTACK_NETWORK.output(NULL);
    etimer_reset(&periodic_timer);

}

PROCESS_END();
}