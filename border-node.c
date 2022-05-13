
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

#define RANK 1


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(){
    broadcastMsg msgPrep;
    msgPrep.rank = RANK;
    msgPrep.typeMsg = 2;

    unsigned int header = buildHeader(msgPrep);    //max 65535  6 = type, 55 = rank, 35 = libre
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;
    LOG_INFO_("Border send parent Proposal (rank = %d\n", msgPrep.rank);
    NETSTACK_NETWORK.output(NULL);
}
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    if(len == sizeof(unsigned)) {
        unsigned bufData;
        memcpy(&bufData, data, 2);
        int typeMsgReceived = bufData/10000;
        if(typeMsgReceived == 1){
            LOG_INFO("Receive parent request (1) from ");
            LOG_INFO_LLADDR(src);
            LOG_INFO_("\n");

            sendParentProposal();
        }
    }
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    #if MAC_CONF_WITH_TSCH
    tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
    #endif /* MAC_CONF_WITH_TSCH */

    nullnet_set_input_callback(input_callback); //LISTENER packet

    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        NETSTACK_NETWORK.output(NULL);
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}