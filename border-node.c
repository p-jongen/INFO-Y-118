
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

static int lenRoutingTable = 50;
static routingRecord routingTable[50];    //every entry : addSrc, NextHop, TTL

int init_var = 0;
static unsigned count = 0;


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

linkaddr_t routingNextHopForDest(linkaddr_t checkAddDest){
    linkaddr_t nextHopForDest;
    int haveFound = 0;
    for(int i = 0 ; i < lenRoutingTable ; i++){
        if(routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(&routingTable[i].addDest, &checkAddDest)) {
                nextHopForDest = routingTable[i].nextHop;
                haveFound = 1;
            }
        }
    }
    if (!haveFound) {
        LOG_INFO_("Border : ADD NOT FOUND IN TABLE \n");
        //nextHopForDest = checkAddDest;
    }
    return nextHopForDest;
}

void sendParentProposal(broadcastMsg receivedMsg){
    LOG_INFO_("Border : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(&receivedMsg.addSrc);
    LOG_INFO_(" \n");

    broadcastMsg msgPrep;           //prepare info
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.rank = RANK;
    msgPrep.typeMsg = 2;


    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);

    LOG_INFO_("Border : Yaouhou ");
    LOG_INFO_LLADDR(&msgPrep.addDest);
    LOG_INFO_(" \n");
    NETSTACK_NETWORK.output(&msgPrep.addDest);
}

void updateRoutingTable(routingRecord receivedRR){
    int isInTable = 0;
    int indexLibre = 0;
    for(int i = 0 ; i < lenRoutingTable ; i++){
        if (routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(&routingTable[i].addDest, &receivedRR.addDest)) {
                isInTable = 1;
                routingTable[i].ttl = 10;
            }
            indexLibre++;
        }
    }
    if(isInTable == 0){                     //alors, add à la routing table
        LOG_INFO_("Border added record in Table");
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
        receivedRR.nextHop = *src;
        receivedRR.ttl = 10;
        updateRoutingTable(receivedRR);
        
        routingRecord receivedRRNextHop;        //màj du previous hop
        receivedRRNextHop.addDest = *src;
        receivedRR.nextHop = *src;
        receivedRR.ttl = 10;
        updateRoutingTable(receivedRRNextHop);

        //if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr) || linkaddr_cmp(&receivedMsg.addDest, &linkaddr_null)) {  //si msg lui est destiné ou si val par default = broadcast
            if (receivedMsg.typeMsg == 0) {                           //Keep-Alive
                LOG_INFO_("Border received KEEP alive");
                updateRoutingTable(receivedRR);
            }
            if (receivedMsg.typeMsg == 1) {                           //receive parent request
                LOG_INFO_("Border : Receive Parent Request from ");
                LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                LOG_INFO_(" \n");
                //TODO : if place enough to get one more children
                sendParentProposal(receivedMsg);
            }
            if (receivedMsg.typeMsg == 2) {   //received parent proposal
                LOG_INFO_("[UNEXPECTED] Border : Receive Parent Proposal\n");
            }
            if (receivedMsg.typeMsg == 3) {     //receive sensor value  //TODO
                LOG_INFO_("Border received sensor value (fct vide)\n");
            }
        //}


         
    }
}

void deleteRecord(int indexToDelete){
    LOG_INFO_("Record DELETED from routing table : ");
    LOG_INFO_LLADDR(&routingTable[indexToDelete].addDest);
    LOG_INFO_(" (si pas de faute mdr)\n");

    for(int i = indexToDelete ; i < lenRoutingTable ; i++){     //on recule tous les record d'un rank à partir du deleted pour le delete
        routingTable[i] = routingTable[i+1];
    }
    routingRecord defaultRR;
    defaultRR.ttl = -1;
    routingTable[lenRoutingTable-1] = defaultRR;                //reset le dernier de la RT
}

void keepAliveDecreaseAll(){                            //réduit de 1 tous les keep-alive
    for(int i = 0 ; i < lenRoutingTable ; i ++){
        if(routingTable[i].ttl != -1){
            routingTable[i].ttl = (routingTable[i].ttl)-1;
        }if(routingTable[i].ttl == 0){
            deleteRecord(i);
        }
    }
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
    static struct etimer periodic_timer;
    broadcastMsg msgInit;
    msgInit.typeMsg = 10;

    PROCESS_BEGIN();
    if(!init_var){
        routingRecord defaultRR;
        defaultRR.ttl = -1;
        for (int i = 0 ; i < lenRoutingTable ; i++){
            routingTable[i] = defaultRR;
        }
        init_var = 1;
    }

    
    nullnet_buf = (uint8_t *)&msgInit;
    nullnet_len = sizeof(struct Message);

    nullnet_set_input_callback(input_callback); //LISTENER packet

    etimer_set(&periodic_timer, SEND_INTERVAL);

    //INITIALIZER
    NETSTACK_NETWORK.output(NULL);

    while(1) {
        if (count >= 99960){ //éviter roverflow
            count=0;
        }

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        etimer_reset(&periodic_timer);
    }
    if(count%60 == 0){                 //all keep-alive --*
        keepAliveDecreaseAll();
        count++;
    }
    count++;

    PROCESS_END();
}