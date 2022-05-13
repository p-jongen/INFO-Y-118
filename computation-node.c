
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

static node_t parent;
static int rank = 99;

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(){
    LOG_INFO_("Computation send parent proposal rank = %d", rank);
    broadcastMsg msgPrep;           //prepare info
    msgPrep.rank = rank;
    msgPrep.typeMsg = 2;

    unsigned int header = buildHeader(msgPrep);    //build header (node.h)
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    NETSTACK_NETWORK.output(NULL);
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    LOG_INFO("Computation Receive\n");
    int parentRankReceived = 0;
    if(len == sizeof(unsigned)) {
        unsigned bufData;
        memcpy(&bufData, data, 2);
        int typeMsgReceived = bufData/10000;

        if(typeMsgReceived == 1){   //received parent request
            LOG_INFO("Computation receive parent request\n");
            sendParentProposal();
        }

        if(typeMsgReceived == 2){   //received parent proposal
            LOG_INFO_("Computation receive parent proposal\n");
            parentRankReceived = (bufData/100)%100;
            if (!linkaddr_cmp(&parent.address,&linkaddr_null)){
                parent.address = *src;
                parent.rank = parentRankReceived;
                //LOG_INFO_("New parent for Computation :  %d, rank = %d\n", parent.address, parent.rank);
                LOG_INFO_("New parent for Computation\n");
            }else{  //déjà un parent
                if( parentRankReceived < parent.rank){
                    parent.rank = (bufData/100)%100;
                    parent.address = *src;
                    //LOG_INFO_("Better parent for Computation :  %d, rank = %d\n", parent.address, parent.rank);
                    LOG_INFO_("Better parent for Computation\n");
                }
                //LOG_INFO_("Old parent holded for Computation :  %d, rank = %d\n", parent.address, parent.rank);
                LOG_INFO_("Old parent holded for Computation\n");
            }
        }
    }
}

void requestParent(short rank){
    LOG_INFO("Computation Sending Parent Request\n");
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.rank = rank;
    msgPrep.typeMsg = 1;

    unsigned int header = buildHeader(msgPrep);    //(in node.h) -> build header TypeMsg(1)-Rank(23)-vide(45) = 12345
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    NETSTACK_NETWORK.output(NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer;
    parent.address = linkaddr_null;
    
    PROCESS_BEGIN();

    nullnet_set_input_callback(input_callback);         //LISTENER
    
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        if(!linkaddr_cmp(&parent.address,&linkaddr_null)){                 //if no parent, send request
            LOG_INFO("Computation has no parent\n");
            requestParent(rank);
        }

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}