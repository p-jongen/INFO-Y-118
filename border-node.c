
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

/*
#define SEND_INTERVAL (8 * CLOCK_SECOND)
static linkaddr_t dest_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
*/

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#endif /* MAC_CONF_WITH_TSCH */

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#define RANK 1              // non variable, car border

//int tableRoutage[][];


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(const linkaddr_t *src){
    LOG_INFO_("Border : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_(" \n");
    broadcastMsg msgPrep;           //prepare info
    msgPrep.rank = RANK;
    msgPrep.typeMsg = 2;

    unsigned int header = buildHeader(msgPrep);   //build header (in node.h)
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;
    NETSTACK_NETWORK.output(src);
}
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{

    const uint8_t* payload2 = data;

    LOG_INFO_("AAAAAAAA0=%u\n", payload2[0]);
    LOG_INFO_("AAAAAAAA1=%u\n", payload2[1]);
    LOG_INFO_("AAAAAAAA2=%u\n", payload2[2]);
    LOG_INFO_("AAAAAAAA3=%u\n", payload2[3]);
    LOG_INFO_("AAAAAAAA4=%u\n", payload2[4]);
    LOG_INFO_("AAAAAAAA5=%u\n", payload2[5]);
    LOG_INFO_("AAAAAAAA6=%u\n", payload2[6]);
    LOG_INFO_("AAAAAAAA7=%u\n", payload2[7]);
    LOG_INFO_("AAAAAAAA8=%u\n", payload2[8]);

    /*
    if(len == sizeof(unsigned)) {
        unsigned bufData;
        memcpy(&bufData, data, 2);



        int typeMsgReceived = bufData/10000;
        if(typeMsgReceived == 1){                           //receive parent request
            LOG_INFO_("Border : Receive Parent Request from ");
            LOG_PRINT_LLADDR(src);
            LOG_INFO_(" \n");
            sendParentProposal(src);
        }
        if(typeMsgReceived == 4){                           //infoRoutage
            LOG_INFO_("Border : Receive Parent Request from ");
            LOG_PRINT_LLADDR(src);
            LOG_INFO_(" \n");
            sendParentProposal(src);
        }
         
    }*/
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    nullnet_set_input_callback(input_callback); //LISTENER packet

    etimer_set(&periodic_timer, SEND_INTERVAL);
    //while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        NETSTACK_NETWORK.output(NULL);
        etimer_reset(&periodic_timer);
    //}

    PROCESS_END();
}