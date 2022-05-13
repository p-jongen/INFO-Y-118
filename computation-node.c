
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

static int parent_Add = -1;
static int rank = 99;

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(){
    broadcastMsg msgPrep;
    msgPrep.rank = rank;
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
    int parentRankReceived = 0;
    if(len == sizeof(unsigned)) {
        unsigned bufData;
        memcpy(&bufData, data, 2);
        int typeMsgReceived = bufData/10000;
        if(typeMsgReceived == 1){   //parent request
            LOG_INFO("Receive parent request (1) from ");
            LOG_INFO_LLADDR(src);
            LOG_INFO_("\n");

            sendParentProposal();
        }
        if(typeMsgReceived == 2){   //parent proposal
            parentRankReceived = (bufData/100)%100;
            LOG_INFO_("ID parent start = %d\n", parent_Add);
            LOG_INFO_("ParentRankReceived = %d\n", parentRankReceived);
            if (parent_Add == -1){
                parent_Add = parentRankReceived;
            }else{
                if( parentRankReceived < parent_Add){
                    parent_Add = (bufData/100)%100;
                }
            }
            LOG_INFO_("ID parent end = %d\n", parent_Add);
        }
    }
}

void requestParent(short rank){
    broadcastMsg msgPrep;
    msgPrep.rank = rank;
    msgPrep.typeMsg = 1;

    unsigned int header = buildHeader(msgPrep);    //max 65535
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    LOG_INFO("Sending Parent");
    LOG_INFO_LLADDR(NULL);
    LOG_INFO_("\n");

    NETSTACK_NETWORK.output(NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer;

    PROCESS_BEGIN();
    
    #if MAC_CONF_WITH_TSCH
    tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
    #endif /* MAC_CONF_WITH_TSCH */

    //nullnet_set_input_callback(input_callback);
    
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        if(parent_Add==-1){
            requestParent(rank);
        }else{
            LOG_INFO("Parent != -1\n");
            LOG_INFO_("\n");
        }

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}