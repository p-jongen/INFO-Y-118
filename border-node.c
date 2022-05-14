
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

static routingRecord routingTable[50];    //every entry : addSrc, NextHop, TTL
routingRecord defaultRR;
defaultRR.ttl = -1;
for (int i = 0 ; i < sizeOf(routingTable) ; i++){
    routingTable[i] = defaultRR;
}


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(broadcastMsg receivedMsg){
    LOG_INFO_("Border : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(receivedMsg.addSrc);
    LOG_INFO_(" \n");

    broadcastMsg msgPrep;           //prepare info
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.rank = RANK;
    msgPrep.typeMsg = 2;

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addSrc);
    NETSTACK_NETWORK.output(dest);
}

linkaddr_t routingNextHopForDest (linkaddr_t checkAddDest){
    linkaddr_t nextHopForDest;
    int haveFound = 0;
    for(int i = 0 ; i < sizeof(routingTable) ; i++){
        if(routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(routingTable[i].addDest, checkAddDest)) {
                nextHopForDest = routingTable[i].nextHop;
                haveFound = 1;
            }
        }
    }
    if (!haveFound) {
        LOG_INFO_("ADD NOT FOUND IN TABLE");
    }
    return nextHopForDest;
}

void updateRoutingTable(routingRecord receivedRR){
    int isInTable = 0;
    int indexLibre = 0;
    for(int i = 0 ; i < sizeof(routingTable) ; i++){
        if (routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(routingTable[i].addDest, receivedRR.addDest)) {
                isInTable = 1;
                routingTable[i].ttl = 10;
            }
            indexLibre++;
        }
    }
    if(isInTable == 0){                     //alors, add à la routing table
        routingTable[indexLibre] = receivedRR;
    }
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    if(len == sizeof(struct Message)) {
        broadcastMsg receivedMsg;
        memcpy(&receivedMsg, data, sizeof(struct Message));

        routingRecord receivedRR;
        receivedRR.addDest = receivedMsg.addSrc;
        receivedRR.nextHop = src;
        receivedRR.ttl = 10;
        updateRoutingTable(receivedRR);

        if(receivedMsg.typeMsg == 1){                           //receive parent request
            LOG_INFO_("Border : Receive Parent Request from ");
            LOG_PRINT_LLADDR(receivedMsg.addSrc);
            LOG_INFO_(" \n");
            //TODO : if place enough to get one more children
                sendParentProposal(receivedMsg));
        }

        if(receivedMsg.typeMsg == 2){   //received parent proposal
            LOG_INFO_("Border : Receive Parent Proposal\n");
            parentProposalComparison(receivedMsg);
        }

        if(typeMsgReceived == 4){                           //infoRoutage
            LOG_INFO_("Border : Receive Parent Request from ");
            LOG_PRINT_LLADDR(src);
            LOG_INFO_(" \n");
            sendParentProposal(src);
        }
         
    }
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    // Initialize NullNet
    nullnet_buf = (uint8_t * ) &broadcastMsg;
    nullnet_len = sizeof(struct Message);


    nullnet_set_input_callback(input_callback); //LISTENER packet

    etimer_set(&periodic_timer, SEND_INTERVAL);

    //INITIALIZER
    NETSTACK_NETWORK.output(NULL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        NETSTACK_NETWORK.output(NULL);
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}