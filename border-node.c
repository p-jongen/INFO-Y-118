
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

#define RANK 1              // non variable, car border


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(){
    LOG_INFO_("Border send parent Proposal (rank = %d\n)", RANK);
    broadcastMsg msgPrep;           //prepare info
    msgPrep.rank = RANK;
    msgPrep.typeMsg = 2;

    unsigned int header = buildHeader(msgPrep);   //build header (in node.h)
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;
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
        if(typeMsgReceived == 1){                           //receive parent request
            LOG_INFO("Border receive parent request \n");
            LOG_INFO_LLADDR(src);
            sendParentProposal();
        }
    }
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    nullnet_set_input_callback(input_callback); //LISTENER packet

    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        NETSTACK_NETWORK.output(NULL);
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}