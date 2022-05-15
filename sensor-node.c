
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

static int lenRoutingTable = 50;            //Max size of routing table
static int init_var = 0;                    //Initialisation variable
static int rank = 99;                       //Basic rank for every new node

static unsigned count = 0;                  //Count that serv as a timer (incremented each second)

static routingRecord routingTable[50];      //Every entry in the table : addSrc, NextHop, TTL

static node_t parent;                       //Structure of the future parent node



/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

linkaddr_t routingNextHopForDest (linkaddr_t checkAddDest){         //Computes the next hop (addr destination) for a received message
    linkaddr_t nextHopForDest;
    int haveFound = 0;
    for(int i = 0 ; i < lenRoutingTable ; i++){
        if(routingTable[i].ttl != -1) {
            if (linkaddr_cmp(&routingTable[i].addDest, &checkAddDest)) {
                nextHopForDest = routingTable[i].nextHop;
                haveFound = 1;
            }
        }
    }
    if (!haveFound) {
        LOG_INFO_("Sensor : ADDR NOT FOUND IN TABLE\n");
    }
    return nextHopForDest;
}

void sendKeepAliveToParent(){                   //Send a keep alive message to parent to stay in the routing table (TTL)
    broadcastMsg msgPrep;                       //Prepare the message to be sent
    msgPrep.typeMsg = 0;                        //Message type 0 : Keep Alive
    msgPrep.addSrc = linkaddr_node_addr;                
    msgPrep.addDest = parent.address;

    nullnet_buf = (uint8_t *)&msgPrep;          //Point the buffer to the message
    nullnet_len = sizeof(struct Message);       //Tell the message length
    NETSTACK_NETWORK.output(&parent.address);   //Send the unicast message
}

void sendParentProposal(broadcastMsg receivedMsg){  //Send a Parent Proposal to whoever send a Parent Request
    broadcastMsg msgPrep;                           //Prepare the message to be sent
    msgPrep.typeMsg = 2;                            //Message type 2 : Parend Proposal
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.rank = rank;                            //Current rank of node

    nullnet_buf = (uint8_t *)&msgPrep;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length
    NETSTACK_NETWORK.output(&msgPrep.addDest);      //Send the unicast message

    //Send a log when Parent Proposal is sent
    LOG_INFO_("Sensor : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(&receivedMsg.addSrc);
    LOG_INFO_(" \n");
}

void sendParentProposalBroadcast(){                 //Send a Parent Proposal in broadcast periodically
    broadcastMsg msgPrep;                           //Prepare the message to be sent
    msgPrep.typeMsg = 2;                            //Message type 2 : Parend Proposal
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.rank = rank;                            //Current rank of node

    nullnet_buf = (uint8_t *)&msgPrep;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length
    NETSTACK_NETWORK.output(NULL);                  //Send the unicast message

    //Send a log when Parent Proposal is sent
    LOG_INFO_("Sensor : Send Broadcast Parent Proposal");
}

void updateRoutingTable(routingRecord receivedRR){      //Add record to the routing table
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

    if(isInTable == 0){                                 //If not already, it's added to the routing table
        routingTable[indexLibre] = receivedRR;
        LOG_INFO_("Sensor : Record added (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].addDest);
        LOG_INFO_(") via next hop (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].nextHop);
        LOG_INFO_(") \n");
    }
}

void addParent(broadcastMsg receivedMsg){       //Add or update info about his parent
    parent.address = receivedMsg.addSrc;
    parent.rank = receivedMsg.rank;
    parent.hasParent = 1;
    
    rank = parent.rank+1;                       //Set rank of the current node
    
    sendKeepAliveToParent();                    //Send a keep alive message
}

void parentProposalComparison(broadcastMsg receivedMsg){    //Compare to see if a new found parent is better than the previous one (if it exist)
    if (parent.hasParent == 0){
        addParent(receivedMsg);

        //Add a new parent 
        LOG_INFO_("Sensor : New parent (");
        LOG_INFO_LLADDR(&receivedMsg.addSrc);
        LOG_INFO_(", rank = %d)\n", parent.rank);
    }else{ 
        if( receivedMsg.rank < parent.rank){
            addParent(receivedMsg);

            //Update parent with a better ranked one
            LOG_INFO_("Sensor : Better parent (");
            LOG_INFO_LLADDR(&receivedMsg.addSrc);
            LOG_INFO_(", rank = %d)\n", parent.rank);
        }
        else if(receivedMsg.rank == parent.rank){
            //TODO verif l'instensité du signal
        }
    }
}

void forwardToParent(broadcastMsg receivedMsg){                 //Copy a message and send it to its parent
    nullnet_buf = (uint8_t *)&receivedMsg;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&parent.address);
}

void forward(broadcastMsg receivedMsg){                         //Copy a message, retrieve the next hop and send it
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addDest);
    
    nullnet_buf = (uint8_t *)&receivedMsg;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&dest);
}

void sendValToParent(int val) {                 //Send the sensor value to the parent
    broadcastMsg msgPrep;                       //Prepare the message to be sent
    msgPrep.typeMsg = 3;                        //Message type 3 : Send value to parent
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.sensorValue = val;

    nullnet_buf = (uint8_t *)&msgPrep;          //Point the buffer to the message
    nullnet_len = sizeof(struct Message);       //Tell the message length
    NETSTACK_NETWORK.output(&parent.address);   //Send the unicast message to the parent
}

void requestParentBroadcast(){                  //Send a message Parent Request to the sensor
    broadcastMsg msgPrep;                       //Send the sensor value to the parent
    msgPrep.typeMsg = 1;                        //Message type 1 : Parent Request
    msgPrep.addSrc = linkaddr_node_addr;

    nullnet_buf = (uint8_t *)&msgPrep;          //Point the buffer to the message
    nullnet_len = sizeof(struct Message);       //Tell the message length
    NETSTACK_NETWORK.output(NULL);              //Send the broadcast message

    LOG_INFO_("Computation : Sending Parent Request (broadcast)\n");
}

void deleteRecord(int indexToDelete){                           //Delete record from the routing table
    for(int i = indexToDelete ; i < lenRoutingTable ; i++){     //We increase each rank by one to add a new one
        routingTable[i] = routingTable[i+1];
    }
    routingRecord defaultRR;
    defaultRR.ttl = -1;
    routingTable[lenRoutingTable-1] = defaultRR;                //Reset the last of the routing table
}

void keepAliveDecreaseAll(){                            //Decrease every TTL of 1
    for(int i = 0 ; i < lenRoutingTable ; i ++){
        if(routingTable[i].ttl != -1){
            routingTable[i].ttl = (routingTable[i].ttl)-1;
        }if(routingTable[i].ttl == 0){
            deleteRecord(i);
        }
    }
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){  //Handle of every received packet
    if(len == sizeof(struct Message)) {
        broadcastMsg receivedMsg;
        memcpy(&receivedMsg, data, sizeof(struct Message));

        if(receivedMsg.typeMsg != 10){
            //UpdateNextHop that sent a message
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

            if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr) || receivedMsg.typeMsg == 1 || receivedMsg.typeMsg == 4) {  //If msg is at its destination or received broadcastly
                if (receivedMsg.typeMsg == 0) {                             //Received a Keep-Alive message
                    updateRoutingTable(receivedRR);
                }

                if (receivedMsg.typeMsg == 1) {                             //Receive Parent Request
                    if (parent.hasParent) {
                        sendParentProposal(receivedMsg);
                    }
                    
                    LOG_INFO_("Sensor : Receive parent request from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                }

                if (receivedMsg.typeMsg == 2) {                             //Received a Parent Proposal message
                    parentProposalComparison(receivedMsg);

                    LOG_INFO_("Sensor : Receive parent proposal from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                }

                if (receivedMsg.typeMsg == 3) {                             //Received a Sensor message
                    LOG_INFO_("Sensor : Receive sensor value from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    
                    if (parent.hasParent) {                                 //Forward the message to the next node
                        forwardToParent(receivedMsg);
                        updateRoutingTable(receivedRR);
                    }
                }

                if (receivedMsg.typeMsg == 4) {                             //Received a Open Valve message
                    LOG_INFO_("Sensor : Received Open Signal from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    
                    if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr)){  
                        LOG_INFO_("Sensor (");
                        LOG_PRINT_LLADDR(&linkaddr_node_addr);
                        LOG_INFO_(") : Open valve\n");
                    }else{
                        forward(receivedMsg);
                    }
                }
            }
            else{          //Forward because not for him
                forward(receivedMsg);
            }
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

    if(!init_var){                                  //Initialize different variable and table only once in the begining
        routingRecord defaultRR;
        defaultRR.ttl = -1;
        for (int i = 0 ; i < lenRoutingTable ; i++){
            routingTable[i] = defaultRR;
        }
        init_var = 1;
    }

    /* Initialize NullNet */
    nullnet_buf = (uint8_t *)&msgInit;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length
    nullnet_set_input_callback(input_callback);  

    etimer_set(&periodic_timer, SEND_INTERVAL);     //Listener function

    NETSTACK_NETWORK.output(NULL);                  //Send the unicast message

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        count++;                                    //Increment count every sec

        if(parent.hasParent == 0 && count%8 == 0){  //If no parent and every 8 second, send Parent Request
            count++;
            requestParentBroadcast();
        }

        if(parent.hasParent == 1 && count%20 == 0){  //If has parent and every 60 second, send sensor value
            int r = abs(rand() % 100);
            LOG_INFO_("Sensor : Value (%d) sended to ", r);
            LOG_INFO_LLADDR(&parent.address);
            LOG_INFO_("\n");
            sendValToParent(r);
        }

        if(parent.hasParent == 1 && count%60 == 0){ //If has parent and every 60 second, send Keep Alive
            count++;
            sendKeepAliveToParent();
            keepAliveDecreaseAll();
        }
    etimer_reset(&periodic_timer);                  //Reset the timer.
    }
PROCESS_END();
}