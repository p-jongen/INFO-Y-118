
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

/* Clock Configuration */
#define SEND_INTERVAL (1 * CLOCK_SECOND)

static int lenRoutingTable = 50;            //Max size of routing table
static int lenDBSensorManaged = 5;          //Number of node that can be handle by computation
static int init_var = 0;                    //Initialisation variable
static int rank = 99;                       //Basic rank for every new node
static int threshold = 52;                  //Treshhold for computation

static unsigned count = 0;                  //Count that serv as a timer (incremented each second)

static routingRecord routingTable[50];      //Every entry in the table : addSrc, NextHop, TTL
static saveValSensor DBSensorManaged[5];    //Every node handleled by the computation node

static node_t parent;                       //Structure of the future parent node

extern const linkaddr_t linkaddr_null;      //Initialize the null addr
extern linkaddr_t linkaddr_node_addr;       //Initialize the node addr


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
        LOG_INFO_("Computation : ADDR NOT FOUND IN TABLE\n");
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
    LOG_INFO_("Computation : Send Unicast Parent Proposal to ");
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
    LOG_INFO_("Computation : Send Broadcast Parent Proposal");
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
        LOG_INFO_("Computation : New parent (");
        LOG_INFO_LLADDR(&receivedMsg.addSrc);
        LOG_INFO_(", rank = %d)\n", parent.rank);
    }else{ 
        if(receivedMsg.rank < parent.rank){
            addParent(receivedMsg);

            //Update parent with a better ranked one
            LOG_INFO_("Computation : Better parent (");
            LOG_INFO_LLADDR(&receivedMsg.addSrc);
            LOG_INFO_(", rank = %d)\n", parent.rank);
        }
        
        
    }
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
        LOG_INFO_("Computation : Record added (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].addDest);
        LOG_INFO_(") via next hop (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].nextHop);
        LOG_INFO_(") \n");
    }
}

void sendSignalOpenToSrc(broadcastMsg receivedMsg){ //Send a message Open Valve to the sensor
    broadcastMsg msgPrep;                           //Prepare the message to be sent
    msgPrep.typeMsg = 4;                            //Message type 2 : Parend Proposal
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.openValve = 1;

    linkaddr_t dest = routingNextHopForDest(receivedMsg.addSrc);

    nullnet_buf = (uint8_t *)&msgPrep;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length
    NETSTACK_NETWORK.output(&dest);                 //Send the unicast message

    //Send a log when Open Valve is sent
    LOG_INFO_("Computation : Send OpenValve to ");
    LOG_INFO_LLADDR(&dest);
    LOG_INFO_(" \n");
}

void computeLeastSquare(broadcastMsg receivedMsg, int indiceInList){        //Compute the Least Square for the last 30 values of the sensor received
    int sum = 0;

    for(int i = 0 ; i < 30 ; i++){
        sum += DBSensorManaged[indiceInList].val[i];
    }

    sum = sum/30;       //Once 30 are reached, we divide it by 30 to have the mean                   

    LOG_INFO_("Computation :  Sum 30 val / 30 = %d\n",sum);

    if (sum > threshold){
        sendSignalOpenToSrc(receivedMsg);
    }else{
        LOG_INFO_("Computation :  Send nothing\n");
    }
}

void computeActionFromSensorValue(broadcastMsg receivedMsg, int inList, int indiceInList, int indiceFreePlaceInList){   //Decide to add or update entities in the computation list
    if(receivedMsg.sensorValue<100 && receivedMsg.sensorValue >0){
        if(!inList){                                                        //If not in the list yet -> in free space
            DBSensorManaged[indiceFreePlaceInList].addSensor = receivedMsg.addSrc;
            DBSensorManaged[indiceFreePlaceInList].val[0] = receivedMsg.sensorValue;
        }else{                                                              //Else -> add val to the tab
            if(DBSensorManaged[indiceInList].val[29] != -1){                //val[30] already full -> add at the end + delete the oldest
                for(int i = 0 ; i < 29 ; i++){
                    DBSensorManaged[indiceInList].val[i] = DBSensorManaged[indiceInList].val[i + 1];
                }
                DBSensorManaged[indiceInList].val[29] = receivedMsg.sensorValue;
                
                computeLeastSquare(receivedMsg, indiceInList);
            }else{                                                          //Simply add value to the val[]
                for(int i = 0 ; i < 30 ; i++){
                    if(DBSensorManaged[indiceInList].val[i] == -1){
                        DBSensorManaged[indiceInList].val[i] = receivedMsg.sensorValue;
                        LOG_INFO_("Computation :  Add value of Sensor(%d) at val(%d)\n",indiceInList, i);
                        if(i == 29){
                            computeLeastSquare(receivedMsg, indiceInList);
                        }
                        i = 30;       
                    }
                }
            }
        }
    }else{
        LOG_INFO_("[ERROR] Computation > computeActionFromSensorValue : wrong sensorValue\n");
    }
}

void forward(broadcastMsg receivedMsg){         //Copy a message, retrieve the next hop and send it
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addDest);
    
    nullnet_buf = (uint8_t *)&receivedMsg;      //Point the buffer to the message
    nullnet_len = sizeof(struct Message);       //Tell the message length
    NETSTACK_NETWORK.output(&dest);             //Send the unicast message

    //Send a log when sensor info is forwarded
    LOG_INFO_("Computation : Forward OpenValveValue to ");
    LOG_PRINT_LLADDR(&dest);
    LOG_INFO_(" \n");
}

void requestParentBroadcast(){              //Send a message Parent Request to the sensor
    broadcastMsg msgPrep;                   //Prepare the message to be sent
    msgPrep.typeMsg = 1;                    //Message type 1 : Parent Request
    msgPrep.addSrc = linkaddr_node_addr;

    nullnet_buf = (uint8_t *)&msgPrep;      //Point the buffer to the message
    nullnet_len = sizeof(struct Message);   //Tell the message length
    NETSTACK_NETWORK.output(NULL);          //Send the broadcast message

    LOG_INFO_("Computation : Sending Parent Request (broadcast)\n");
}

void deleteRecord(int indexToDelete){                           //Delete record from the routing table
    for(int i = indexToDelete ; i < lenRoutingTable ; i++){     //We decrease each rank from the deleted record to take the free place
        routingTable[i] = routingTable[i+1];
    }
    routingRecord defaultRR;
    defaultRR.ttl = -1;
    routingTable[lenRoutingTable-1] = defaultRR;                //Reset the last of the routing table
}

void keepAliveDecreaseAll(){                                    //Decrease every TTL of 1
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

            if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr) || receivedMsg.typeMsg == 1 || receivedMsg.typeMsg == 3) {  //If msg is at its destination or received broadcastly
                if (receivedMsg.typeMsg == 0){                              //Received a Keep-Alive message
                    updateRoutingTable(receivedRR);
                }

                if (receivedMsg.typeMsg == 1){                              //Received a Parent Request message
                    if (parent.hasParent) {
                        sendParentProposal(receivedMsg);
                    }

                    LOG_INFO_("Computation : Receive parent request from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                }

                if (receivedMsg.typeMsg == 2){                              //Received a Parent Proposal message
                    parentProposalComparison(receivedMsg);
                    
                    LOG_INFO_("Computation : Receive parent proposal from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                }

                if (receivedMsg.typeMsg == 3){                              //Received a Sensor message
                    LOG_INFO_("Computation : Receive sensor value from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");

                    if (parent.hasParent) {                                 //Verify that he has a parent
                        int hasToForward = 1;
                        int inList = 0;
                        int indiceInList = 0;
                        int indiceFreePlaceInList = 0;

                        for(int i = (lenDBSensorManaged -1) ; i >= 0 ; i--){
                            if(linkaddr_cmp(&DBSensorManaged[i].addSensor, &receivedMsg.addSrc)){ //Verify if sensor is already in its managed list
                                hasToForward = 0;                           
                                inList = 1;
                                indiceInList = i;
                            }else if(DBSensorManaged[i].val[0] == -1){
                                indiceFreePlaceInList = i;                  //Will decrease until the first free place
                                hasToForward = 0;                           //Because there is a free spot in his managed list
                            }
                        }

                        if(hasToForward){                                   //If he can't manage it, then it's forwarded
                            forward(receivedMsg);
                        }else{                                              //If he can, it's managed
                            computeActionFromSensorValue(receivedMsg, inList, indiceInList, indiceFreePlaceInList);
                        }
                        updateRoutingTable(receivedRR);
                    }
                }
                
                if (receivedMsg.typeMsg == 4) {                             //Received a Open Valve message
                    LOG_INFO_("Computation : Received Open Signal from \n");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");

                    if(parent.hasParent ==1){
                        forward(receivedMsg);
                    }
                }
            }
            else{          //Forward because not for him
                if(parent.hasParent ==1){
                    forward(receivedMsg);
                }
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
        saveValSensor defaultValSensor;

        for (int i = 0 ; i < 30 ; i++){
            defaultValSensor.val[i] = -1;
        }


        for (int i = 0 ; i < lenDBSensorManaged ; i++){
            DBSensorManaged[i] = defaultValSensor;
        }
        init_var = 1;
    }

    /* Initialize NullNet */
    nullnet_buf = (uint8_t *)&msgInit;              //Point the buffer to the message
    nullnet_len = sizeof(struct Message);           //Tell the message length
    nullnet_set_input_callback(input_callback);     //Send the unicast message

    etimer_set(&periodic_timer, SEND_INTERVAL);     //Set the 1 second timer

    NETSTACK_NETWORK.output(NULL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        count++;                                    //Increment count every second

        if(parent.hasParent == 0 && count%8 == 0){  //If no parent and every 8 second, send Parent Request
            count++;
            requestParentBroadcast();
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