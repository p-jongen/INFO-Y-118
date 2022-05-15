
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
static routingRecord routingTable[50];    //every entry : addSrc, NextHop, TTL
static int init_var = 0;
static node_t parent;
static int rank = 99;
static unsigned count = 0;



/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

linkaddr_t routingNextHopForDest (linkaddr_t checkAddDest){
    linkaddr_t nextHopForDest;
    int haveFound = 0;
    for(int i = 0 ; i < lenRoutingTable ; i++){
        if(routingTable[i].ttl != -1) {
            if (linkaddr_cmp(&routingTable[i].addDest, &checkAddDest)) {
                nextHopForDest = routingTable[i].nextHop;
                haveFound = 1;
            }
        
            LOG_INFO_("IN RT SENSOR :");
            LOG_INFO_LLADDR(&routingTable[i].addDest);
            LOG_INFO_(" \n");
        }
    }
    if (!haveFound) {
        LOG_INFO_("ADD NOT FOUND IN TABLE\n");
    }
    return nextHopForDest;
}

void sendKeepAliveToParent(){
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.typeMsg = 0;
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = parent.address;

    nullnet_buf = (uint8_t *)&msgPrep;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&parent.address);
    LOG_INFO_("Sensor : Sent keepalive\n");
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

    nullnet_buf = (uint8_t *)&msgPrep;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&msgPrep.addDest);      //car directement connecté
}

void updateRoutingTable(routingRecord receivedRR){
    int isInTable = 0;
    int indexLibre = 0;
    for(int i = 0 ; i < lenRoutingTable ; i++){
        if (routingTable[i].ttl != -1) {
            if (linkaddr_cmp(&routingTable[i].addDest, &receivedRR.addDest)) {
                isInTable = 1;
                routingTable[i].ttl = 10;
                break;
            }
            indexLibre++;
        }
    }
    if(isInTable == 0){                     //alors, add à la routing table
       
        routingTable[indexLibre] = receivedRR;
        LOG_INFO_("Sensor : Record added : ");
          LOG_INFO_LLADDR(&routingTable[indexLibre].addDest);
        LOG_INFO_(" via next hop : ");
          LOG_INFO_LLADDR(&routingTable[indexLibre].nextHop);
        LOG_INFO_("\n");
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

        LOG_INFO_("New parent for Sensor : ");
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
    nullnet_buf = (uint8_t *)&receivedMsg;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&parent.address);
}

void forward(broadcastMsg receivedMsg){                         //copie le msg, récup le nexhop pour la dest et envoie au nexhop
    nullnet_buf = (uint8_t *)&receivedMsg;
    nullnet_len = sizeof(struct Message);
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addDest);
    NETSTACK_NETWORK.output(&dest);
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){
     
    
    if(len == sizeof(struct Message)) {
        broadcastMsg receivedMsg;
        memcpy(&receivedMsg, data, sizeof(struct Message));

        if(receivedMsg.typeMsg == 10){
            LOG_INFO_("In\n");
        }else{
            //UpdateNextHop qui lui a envoyé le msg
            routingRecord receivedRRNextHop;        
            receivedRRNextHop.addDest = *src;
            receivedRRNextHop.nextHop = *src;
            receivedRRNextHop.ttl = 10;
            updateRoutingTable(receivedRRNextHop);

            //Prepare update record for needed typeMsg
            routingRecord receivedRR;
            receivedRR.addDest = receivedMsg.addSrc;
            receivedRR.nextHop = *src;
            receivedRR.ttl = 10;

            if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr) || receivedMsg.typeMsg == 1 || receivedMsg.typeMsg == 4) {  //si msg lui est destiné ou si val par default = broadcast
                if (receivedMsg.typeMsg == 0) {                           //Keep-Alive
                    updateRoutingTable(receivedRR);
                }
                if (receivedMsg.typeMsg == 1) {                           //receive parent request
                    LOG_INFO_("Sensor : Receive Parent Request from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    //TODO : if place enough to get one more children
                    if (parent.hasParent) {
                        sendParentProposal(receivedMsg);
                    }else{
                        LOG_INFO_("Sensor ignore parent request (car no parent) \n");
                    }
                }
                if (receivedMsg.typeMsg == 2) {   //received parent proposal
                    LOG_INFO_("Sensor : Receive Parent Proposal\n");
                    parentProposalComparison(receivedMsg);
                }
                if (receivedMsg.typeMsg == 3) {     //receive sensor value
                    LOG_INFO_("Sensor : type msg = 3 --> Calcule ou forward\n");
                    
                    if (parent.hasParent) {               //Vérif qu'il a bien un parent
                        forwardToParent(receivedMsg);
                        updateRoutingTable(receivedRR);
                    }
                }
                if (receivedMsg.typeMsg == 4) {   //received parent proposal    //TODO
                    LOG_INFO_("Sensor received openSignal\n");
                    if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr)){  
                        
                    LOG_INFO_("SENSOR ");
                    LOG_PRINT_LLADDR(&linkaddr_node_addr);
                    LOG_INFO_(" OPEN VALVE\n");
                    }else{          //il forward car pas pour lui
                        forward(receivedMsg);
                    }
                }
            }
            else{          //il forward car pas pour lui
                forward(receivedMsg);
            }
        }    
    }
}

void sendValToParent(int val) {
    
    broadcastMsg msgPrep;
    msgPrep.typeMsg = 3;
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.sensorValue = val;

    nullnet_buf = (uint8_t *)&msgPrep;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&parent.address);
}

void requestParentBroadcast(){
    LOG_INFO_("Sensor : Sending Parent Request\n");
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.typeMsg = 1;
    msgPrep.addSrc = linkaddr_node_addr;
    LOG_INFO_("Sensor : %u\n", sizeof(struct Message));

    nullnet_buf = (uint8_t *)&msgPrep;
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

    /* Initialize NullNet */
    nullnet_buf = (uint8_t *)&msgInit;
    nullnet_len = sizeof(struct Message);
    nullnet_set_input_callback(input_callback);

    etimer_set(&periodic_timer, SEND_INTERVAL);

    NETSTACK_NETWORK.output(NULL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        count++;

        if(parent.hasParent == 0 && count%8 == 0){                 //if no parent, send request
            count++;

            //requestParentBroadcast();
            LOG_INFO_("Sensor : Sending Parent Request\n");
            broadcastMsg msgPrep;                   //prepare info
            msgPrep.typeMsg = 1;
            msgPrep.addSrc = linkaddr_node_addr;
            nullnet_buf = (uint8_t *)&msgPrep;
            nullnet_len = sizeof(struct Message);
            NETSTACK_NETWORK.output(NULL); 
        }

        if(parent.hasParent == 1 && count%20 == 0){                 //if parent, recup value
            int r = abs(rand() % 100);
            LOG_INFO_("Value of sensor : %d sended to parent : ", r);
            LOG_INFO_LLADDR(&parent.address);
            LOG_INFO_("\n");
            sendValToParent(r);
        }

        if(parent.hasParent == 1 && count%20 == 0){                 //all keep-alive --
            count++;
            sendKeepAliveToParent();
            //keepAliveDecreaseAll();
        }
    etimer_reset(&periodic_timer);
    }
PROCESS_END();
}