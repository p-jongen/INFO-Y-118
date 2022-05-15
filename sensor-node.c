
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

static int lenRoutingTable = 50;
static routingRecord routingTable[lenRoutingTable];    //every entry : addSrc, NextHop, TTL
static int init_var = 0;
static node_t parent;
static int rank = 99;

static int countTimer = 0;


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendKeepAliveToParent(){
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.typeMsg = 0;
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = parent.address;

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&parent.address);
}

linkaddr_t routingNextHopForDest (linkaddr_t checkAddDest){
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
    NETSTACK_NETWORK.output(&msgPrep.addDest);      //car directement connecté
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
        LOG_INFO_("Sensor added record in Table");
        routingTable[indexLibre] = receivedRR;
    }
}


void addParent(broadcastMsg receivedMsg){  //add or update info about his parent
    parent.address = receivedMsg.addSrc;
    parent.rank = receivedMsg.rank;
    rank = parent.rank+1;
    parent.hasParent = 1;
    sendKeepAliveToParent();
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

void forwardToParent(broadcastMsg receivedMsg){                 //quand forward direct to parent car sensor peut pas use
    memcpy(nullnet_buf, &receivedMsg, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&parent.address);
}

void forward(broadcastMsg receivedMsg){                         //copie le msg, récup le nexhop pour la dest et envoie au nexhop
    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addDest);
    NETSTACK_NETWORK.output(&dest);
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

        if((receivedMsg.addDest == linkaddr_node_addr) || (receivedMsg.addDest == linkaddr_null)) {  //si msg lui est destiné ou si val par default = broadcast
            if (receivedMsg.typeMsg == 0) {                           //Keep-Alive
                LOG_INFO_("Sensor received KEEP alive");
                updateRoutingTable(receivedRR);
            }
            if (receivedMsg.typeMsg == 1) {                           //receive parent request
                LOG_INFO_("Sensor : Receive Parent Request from ");
                LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                LOG_INFO_(" \n");
                //TODO : if place enough to get one more children
                if (parent.hasParent) {
                    sendParentProposal(receivedMsg);
                }
            }
            if (receivedMsg.typeMsg == 2) {   //received parent proposal
                LOG_INFO_("Sensor : Receive Parent Proposal\n");
                parentProposalComparison(receivedMsg);
            }
            if (receivedMsg.typeMsg == 3) {     //receive sensor value
                LOG_INFO_("Computation : type msg = 3 --> Calcule ou forward\n");
                if (parent.hasParent) {               //Vérif qu'il a bien un parent
                    forwardToParent(receivedMsg);
                }
            }
            if (receivedMsg.typeMsg == 4) {   //received parent proposal    //TODO
                LOG_INFO_("Sensor received openSignal --> TODO verif id dest + open ou forward\n");
            }
        }
        else{          //il forward car pas pour lui
            routingRecord receivedRRNextHop;        //màj du previous hop
            receivedRRNextHop.addDest = *src;
            receivedRR.nextHop = *src;
            receivedRR.ttl = 10;
            updateRoutingTable(receivedRRNextHop);
            forward(receivedMsg);
        }
    }
}

void sendValToParent(int val) {
    broadcastMsg msgPrep;
    msgPrep.typeMsg = 3;
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.sensorValue = val;

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(NULL);
}

void requestParentBroadcast(){
    LOG_INFO_("Sensor : Sending Parent Request\n");
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.typeMsg = 1;
    msgPrep.addSrc = linkaddr_node_addr;
    LOG_INFO_("Sensor : %u\n", sizeof(struct Message));

    memcpy(nullnet_buf, &msgPrep, sizeof(struct Message));
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(NULL);
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
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer_parentRequest;
    static unsigned count = 0;

    PROCESS_BEGIN();
    if(!init_var){
        routingRecord defaultRR;
        defaultRR.ttl = -1;
        for (int i = 0 ; i < lenRoutingTable ; i++){
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
        if(parent.hasParent == 0 && count%8 == 0){                 //if no parent, send request
            LOG_INFO_("Computation parent : None\n");
            requestParentBroadcast();
        }

        if(parent.hasParent == 1 && countTimer%60 == 0){                 //all keep-alive --
            sendKeepAliveToParent();
            keepAliveDecreaseAll();
            count++;
        }
        if(parent.hasParent == 1 && countTimer%60 == 0){                 //if parent, recup value
            int r = abs(rand() % 100);
            LOG_INFO_("Value of sensor : %d\n", r);
            sendValToParent(r);
            count++;
        }
        count++;
        etimer_reset(&periodic_timer_parentRequest);
    }
    
    PROCESS_END();
}