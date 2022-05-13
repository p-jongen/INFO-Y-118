
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
static linkaddr_t coordinator_addr =  {{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

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
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer;
    //uint8_t  border_header[64] = {[0 ... 63] = -1};
    static short rank = 255;            //max rank car cherche un parent
    static int parent_Add = -1;
    //static unsigned count = 0;

    PROCESS_BEGIN();
    
    #if MAC_CONF_WITH_TSCH
    tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
    #endif /* MAC_CONF_WITH_TSCH */

    //nullnet_len = sizeof(count);
    //nullnet_set_input_callback(input_callback);
    
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        if(parent_Add==-1){
            broadcastMsg msgPrep;
            msgPrep.rank = rank;
            msgPrep.typeMsg = 3;

            int header[8] = {[0 ... 7] = -1};
            constructHeader(header, msgPrep);
            for(int i = 0; i < 7; i++){
                LOG_INFO("%d", header[i]);
            }
            nullnet_buf = (uint8_t *)&header;
            nullnet_len = 2;

            LOG_INFO("Sending Parent");
            LOG_INFO_LLADDR(NULL);
            LOG_INFO_("\n");

            NETSTACK_NETWORK.output(NULL);
        }else{
            LOG_INFO("Parent != -1\n");
            LOG_INFO_("\n");
        }

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}