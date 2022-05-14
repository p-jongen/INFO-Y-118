
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include "node.h"
#include <stdlib.h> 
#include <math.h> 

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

static int countTimer = 0;


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

linkaddr_t routingNextHopForDest (linkaddr_t checkAddDest){
    linkaddr_t nextHopForDest;
    int haveFound = 0;
    for(int i = 0 ; i < sizeof(routingTable) ; i++){
        if(routingTable[i].ttl != -1) {
            if (!linkaddr_cmp(&routingTable[i].addDest, &checkAddDest)) {
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

void sendParentProposal(broadcastMsg receivedMsg){
    LOG_INFO_("Sensor : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(&receivedMsg.addSrc);
    LOG_INFO_(" \n");

    broadcastMsg msgPrep;           //prepare info
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.rank = rank;
    msgPrep.typeMsg = 2;

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addSrc);
    NETSTACK_NETWORK.output(&dest);
}

void updateRoutingTable(routingRecord receivedRR){
    int isInTable = 0;
    int indexLibre = 0;
    for(int i = 0 ; i < sizeof(routingTable) ; i++){
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

void addParent(broadcastMsg receivedMsg){  //add or update info about his parent
    parent.address = receivedMsg.addSrc;
    parent.rank = receivedMsg.rank;
    rank = parent.rank+1;
    parent.hasParent = 1;
}

void parentProposalComparison(broadcastMsg receivedMsg){
    if (parent.hasParent == 0){
        addParent(receivedMsg);

        LOG_INFO_("New parent for Senspr : ");
        LOG_INFO_LLADDR(&receivedMsg.addSrc);
        LOG_INFO_(" (Parent rank = %d)\n", parent.rank);
    }else{  //déjà un parent
        if( receivedMsg.rank < parent.rank){
            addParent(receivedMsg);

            LOG_INFO_("Better parent for Sensor :  ");
            LOG_INFO_LLADDR(&receivedMsg.addSrc);
            LOG_INFO_(" (Parent rank = %d)\n", parent.rank);
        }
        else if(receivedMsg.rank == parent.rank){
            //TODO verif l'instensité du signal
        }
        else{
            LOG_INFO_("Old parent holded for Sensor");
        }
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
            //TODO recept la valeur des 2 derniers chiffres du header. Si c'est un computation, faire le calcul et renvoyer la réponse vers sensor qui a envoyé ce paquet. Si c'est un sensor, forward vers parent
            LOG_INFO_("Sensor : type msg = 3\n");
        }
    }
}

/*void sendValToParent(int val) {
    broadcastMsg msgPrep;
    msgPrep.rank = rank;
    msgPrep.typeMsg = 3;

    unsigned int header = buildHeader(msgPrep);
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    NETSTACK_NETWORK.output(&parent.address);
}*/

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
    static struct etimer periodic_timer;
    broadcastMsg msg;

    PROCESS_BEGIN();

    if(!init_var){
        routingRecord defaultRR;
        defaultRR.ttl = -1;
        for (int i = 0 ; i < sizeof(routingTable) ; i++){
            routingTable[i] = defaultRR;
        }
        init_var = 1;
    }

    // Initialize NullNet
    nullnet_buf = (uint8_t * ) &msg;
    nullnet_len = sizeof(struct Message);

    nullnet_set_input_callback(input_callback);         //LISTENER

    //INITIALIZER
    NETSTACK_NETWORK.output(NULL);

    while(1) {
        //parent
        etimer_set(&periodic_timer, SEND_INTERVAL);
        countTimer++;
        if (countTimer >= 99960){ //éviter roverflow
            countTimer=0;
        }
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        etimer_reset(&periodic_timer);

        if(parent.hasParent == 0 && countTimer%8 == 0){                 //if no parent, send request
            LOG_INFO_("Computation parent : None\n");
            requestParentBroadcast(rank);
        }

        if(parent.hasParent == 1 && countTimer%60 == 0){                 //if no parent, send request
            int r = abs(rand() % 100);
            LOG_INFO_("Value of sensor : %d\n", r);
            //sendValToParent(r);
        }
    }
    PROCESS_END();
}