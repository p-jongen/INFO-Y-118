
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
#define SEND_INTERVAL (1 * CLOCK_SECOND)

static routingRecord routingTable[50];    //every entry : addSrc, NextHop, TTL
static int init_var = 0;
static node_t parent;
static int rank = 99;
broadcastMsg msg;

//static int countTimer = 0;

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

linkaddr_t routingNextHopForDest (linkaddr_t checkAddDest){
    linkaddr_t nextHopForDest;
    int haveFound = 0;
    for(int i = 0 ; i < 50 ; i++){
        if(routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(&routingTable[i].addDest, &checkAddDest)) {
                nextHopForDest = routingTable[i].nextHop;
                haveFound = 1;
            }
        }
    }
    if (!haveFound) {
        LOG_INFO_("ADD NOT FOUND IN TABLE");
        nextHopForDest = checkAddDest;
    }
    return nextHopForDest;
}

void sendParentProposal(broadcastMsg receivedMsg){
    LOG_INFO_("Computation : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(&receivedMsg.addSrc);
    LOG_INFO_(" \n");

    broadcastMsg msgPrep;           //prepare info
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.rank = rank;
    msgPrep.typeMsg = 2;
    LOG_INFO_("Border : ssss ");
    LOG_INFO_LLADDR(&msgPrep.addDest);
    LOG_INFO_(" \n");

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addSrc);
    LOG_INFO_("Border : Yaouhou ");
    LOG_INFO_LLADDR(&dest);
    LOG_INFO_(" \n");
    NETSTACK_NETWORK.output(&dest);
}

void addParent(broadcastMsg receivedMsg){  //add or update info about his parent
    parent.address = receivedMsg.addSrc;
    parent.rank = receivedMsg.rank;
    rank = parent.rank+1;
    parent.hasParent = 1;
}

void parentProposalComparison(broadcastMsg receivedMsg){
    if (parent.hasParent == 0){
        addParent(receivedMsg);

        LOG_INFO_("New parent for Computation : ");
        LOG_INFO_LLADDR(&receivedMsg.addSrc);
        LOG_INFO_(" (Parent rank = %d)\n", parent.rank);
    }else{  //déjà un parent
        if(receivedMsg.rank < parent.rank){
            addParent(receivedMsg);

            LOG_INFO_("Better parent for Computation :  ");
            LOG_INFO_LLADDR(&receivedMsg.addSrc);
            LOG_INFO_(" (Parent rank = %d)\n", parent.rank);
        }
        else if(receivedMsg.rank == parent.rank){
            //TODO verif l'instensité du signal
        }
        else{
            LOG_INFO_("Old parent holded for Computation");
        }
    }
}

void updateRoutingTable(routingRecord receivedRR){
    int isInTable = 0;
    int indexLibre = 0;
    for(int i = 0 ; i < 50 ; i++){
        if (routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(&routingTable[i].addDest, &receivedRR.addDest)) {
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
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){
    if(len == sizeof(struct Message)) {
        broadcastMsg receivedMsg;
        memcpy(&receivedMsg, data, sizeof(struct Message));

        routingRecord receivedRR;
        receivedRR.addDest = receivedMsg.addSrc;
        receivedRR.nextHop = *src;
        receivedRR.ttl = 10;
        updateRoutingTable(receivedRR);

        if(receivedMsg.typeMsg == 1){                           //receive parent request
            LOG_INFO_("Computation : Receive Parent Request from ");
            LOG_PRINT_LLADDR(&receivedMsg.addSrc);
            LOG_INFO_(" \n");
            //TODO : if place enough to get one more children
            if (parent.hasParent){
                sendParentProposal(receivedMsg);
            }
        }

        if(receivedMsg.typeMsg == 2){   //received parent proposal
            LOG_INFO_("Computation : Receive Parent Proposal\n");
            parentProposalComparison(receivedMsg);
        }
        if(receivedMsg.typeMsg == 3){
            LOG_INFO_("Comparison : type msg = 3\n");
        }
    }
}

void requestParentBroadcast(){
    LOG_INFO_("Computation : Sending Parent Request\n");
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.typeMsg = 1;
    msgPrep.addSrc = linkaddr_node_addr;

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer_parentRequest;
    static unsigned count = 0;

    PROCESS_BEGIN();
    if(!init_var){
        routingRecord defaultRR;
        defaultRR.ttl = -1;
        for (int i = 0 ; i < 50 ; i++){
            routingTable[i] = defaultRR;
        }
        init_var = 1;
    }

    // Initialize NullNet
    //msg.typeMsg = 3;


    nullnet_buf = (uint8_t *)&count;
    nullnet_len = sizeof(count);
    memcpy(nullnet_buf, &count, sizeof(count));
    nullnet_set_input_callback(input_callback);         //LISTENER
    
    if (count >= 99960){ //éviter roverflow
            count=0;
    }

    etimer_set(&periodic_timer_parentRequest, SEND_INTERVAL);
       
    while(1){    
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_parentRequest));
        //PARENT 
        memcpy(nullnet_buf, &count, sizeof(count));
        nullnet_len = sizeof(count);
        NETSTACK_NETWORK.output(NULL);
        if(parent.hasParent == 0 && count%9 == 0){                 //if no parent, send request
            LOG_INFO_("Computation parent : None\n");
            requestParentBroadcast();
            count++;
        }
        
       
        count++;
        
        etimer_reset(&periodic_timer_parentRequest);
    }
    
    PROCESS_END();
}