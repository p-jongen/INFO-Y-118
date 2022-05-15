
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

#define RANK 1              // non variable, car border

static int lenRoutingTable = 50;            //Max size of routing table
static int init_var = 0;                    //Initialisation variable

static unsigned count = 0;                  //Count that serv as a timer (incremented each second)

static routingRecord routingTable[50];      //Every entry in the table : addSrc, NextHop, TTL

/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

linkaddr_t routingNextHopForDest(linkaddr_t checkAddDest){          //Computes the next hop (addr destination) for a received message
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
        LOG_INFO_("Border : ADDR NOT FOUND IN TABLE \n");
    }
    return nextHopForDest;
}

void sendParentProposal(broadcastMsg receivedMsg){  //Send a Parent Proposal to whoever send a Parent Request
    broadcastMsg msgPrep;                           //Prepare the message to be sent
    msgPrep.typeMsg = 2;                            //Message type 2 : Parend Proposal
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.rank = RANK;                            //Current rank of node
    

    nullnet_buf = (uint8_t *)&msgPrep;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length
    NETSTACK_NETWORK.output(&msgPrep.addDest);      //Send the unicast message

    //Send a log when Parent Proposal is sent
    LOG_INFO_("Border : Send Unicast Parent Proposal to ");
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
    LOG_INFO_("Border : Send Broadcast Parent Proposal");
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
        LOG_INFO_("Border : Record added (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].addDest);
        LOG_INFO_(") via next hop (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].nextHop);
        LOG_INFO_(") \n");
    }
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
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){ //Handle of every received packet
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

            if (receivedMsg.typeMsg == 0) {             //Received a Keep-Alive message
                updateRoutingTable(receivedRR);
            }

            if (receivedMsg.typeMsg == 1) {             //Receive Parent Request
                sendParentProposal(receivedMsg);

                LOG_INFO_("Border : Receive Parent Request from ");
                LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                LOG_INFO_(" \n");
            }

            if (receivedMsg.typeMsg == 3) {             //Received a Sensor message
                updateRoutingTable(receivedRR);

                LOG_INFO_("[SENSOR_VALUE] Border : Receive sensor value = %d ", receivedMsg.sensorValue);
                LOG_INFO_LLADDR(&receivedMsg.addSrc);
                LOG_INFO_("\n");
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
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

    
    nullnet_buf = (uint8_t *)&msgInit;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length

    nullnet_set_input_callback(input_callback);     //Listener packet

    etimer_set(&periodic_timer, SEND_INTERVAL);

    //INITIALIZER
    NETSTACK_NETWORK.output(NULL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        count++;

        if(count%60 == 0){                 //all keep-alive --*
            keepAliveDecreaseAll();
            count++;
        }
        etimer_reset(&periodic_timer);
    }
    PROCESS_END();
}