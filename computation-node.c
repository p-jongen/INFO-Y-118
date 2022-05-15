
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
//#define SEND_INTERVAL_EIGHT (8 * CLOCK_SECOND)
//#define SEND_INTERVAL_SIXTY (60 * CLOCK_SECOND)
#define SEND_INTERVAL (1 * CLOCK_SECOND)

static int lenRoutingTable = 50;
static int lenDBSensorManaged = 30;
static routingRecord routingTable[50];    //every entry : addSrc, NextHop, TTL
static int init_var = 0;
static node_t parent;
static int rank = 99;
static unsigned count = 0;

extern const linkaddr_t linkaddr_null;
extern linkaddr_t linkaddr_node_addr;

static saveValSensor DBSensorManaged[5];
static int threshold = 60;

//static int countTimer = 0;

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
        }
    }
    if (!haveFound) {
        LOG_INFO_("Computation : ADDR NOT FOUND IN TABLE\n");
        //nextHopForDest = checkAddDest;
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

    nullnet_buf = (uint8_t *)&msgPrep;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(&msgPrep.addDest);
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

        LOG_INFO_("Computation : New parent (");
        LOG_INFO_LLADDR(&receivedMsg.addSrc);
        LOG_INFO_(", rank = %d)\n", parent.rank);
    }else{  //déjà un parent
        if(receivedMsg.rank < parent.rank){
            addParent(receivedMsg);

            LOG_INFO_("Computation : Better parent (");
            LOG_INFO_LLADDR(&receivedMsg.addSrc);
            LOG_INFO_(", rank = %d)\n", parent.rank);
        }
        else if(receivedMsg.rank == parent.rank){
            //TODO verif l'instensité du signal
        }
        
    }
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
        LOG_INFO_("Computation : Record added (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].addDest);
        LOG_INFO_(") via next hop (");
        LOG_INFO_LLADDR(&routingTable[indexLibre].nextHop);
        LOG_INFO_(") \n");
    }
}

void sendSignalOpenToSrc(broadcastMsg receivedMsg){
    broadcastMsg msgPrep;           //prepare info
    msgPrep.addSrc = linkaddr_node_addr;
    msgPrep.addDest = receivedMsg.addSrc;
    msgPrep.typeMsg = 4;
    msgPrep.openValve = 1;

    nullnet_buf = (uint8_t *)&msgPrep;
    nullnet_len = sizeof(struct Message);
    
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addSrc);
    NETSTACK_NETWORK.output(&dest);
    LOG_INFO_("Computation : Send OpenValve to ");
    LOG_INFO_LLADDR(&dest);
    LOG_INFO_(" \n");
}

computeLeastSquare(broadcastMsg receivedMsg, int indiceInList){
    int sum = 0;
    for(int i = 0 ; i < 30 ; i++){
        sum += DBSensorManaged[indiceInList].val[i];
        LOG_INFO_("[COMPUTE VAL] :  sum 30 val = ");
    }
    sum = sum/30;
    LOG_INFO_("[COMPUTE VAL] :  sum 30 val / 30 = %d",sum);
    if (sum > threshold){
        sendSignalOpenToSrc(receivedMsg);
        LOG_INFO_("[COMPUTE VAL] :  Send Open");
    }else{
        LOG_INFO_("[COMPUTE VAL] :  Send nothing");
    }
}

void computeActionFromSensorValue(broadcastMsg receivedMsg, int inList, int indiceInList, int indiceFreePlaceInList){
    LOG_INFO_("[COMPUTE VAL] :  INTO computeActionFromSensorValue");
    if(receivedMsg.sensorValue<100 && receivedMsg.sensorValue >0){
        if(!inList){                                            //If not yet in the list --> in free space
            LOG_INFO_("[COMPUTE VAL] :  Add Sensor in Manage List and value as val[0] at indice list = %d", indiceFreePlaceInList);
            DBSensorManaged[indiceFreePlaceInList].addSensor = receivedMsg.addSrc;
            DBSensorManaged[indiceFreePlaceInList].val[0] = receivedMsg.sensorValue;
        }else{                                                  //Else --> add val to the tab
            LOG_INFO_("[COMPUTE VAL] :  Sensor already in list at indice %d", indiceInList);
            if(DBSensorManaged[indiceInList].val[29] != -1){    //val[30] already full --> add at the end + delete the oldest
                LOG_INFO_("[COMPUTE VAL] :  is 30ème val registered for this Sensor --> décale + computeLeastSquare");
                for(int i = 0 ; i < 29 ; i++){
                    DBSensorManaged[indiceInList].val[i] = DBSensorManaged[indiceInList].val[i + 1];
                }
                DBSensorManaged[indiceInList].val[i] = receivedMsg.sensorValue;
                computeLeastSquare(receivedMsg, indiceInList);
            }else{                                              //just add value to the val[]
                LOG_INFO_("[COMPUTE VAL] :  add value of Sensor indice = %d at val[%d]",indiceInList);
                for(int i = 0 ; i < 30 ; i++){
                    if(DBSensorManaged[indiceInList].val[i] == -1){
                        DBSensorManaged[indiceInList].val[i] = receivedMsg.sensorValue;
                        LOG_INFO_("[COMPUTE VAL] :  add value of Sensor indice = %d at val[%d]",indiceInList, i);
                        if(i == 30){
                            LOG_INFO_("[COMPUTE VAL] :  30ème val[] --> computeLeastSquare");
                            computeLeastSquare(receivedMsg, indiceInList);
                        }
                        i = 30;         //out the for()
                    }
                }
            }
        }
    }else{
        LOG_INFO_("[ERROR] Computation > computeActionFromSensorValue : wrong sensorValue\n");
    }
}

void forward(broadcastMsg receivedMsg){                              //copie le msg, récup le nexhop pour la dest et envoie au nexhop
    nullnet_buf = (uint8_t *)&receivedMsg;
    nullnet_len = sizeof(struct Message);
    linkaddr_t dest = routingNextHopForDest(receivedMsg.addDest);
    LOG_INFO_("Computation : Forward OpenValveValue to ");
    LOG_PRINT_LLADDR(&dest);
    LOG_INFO_(" \n");
    NETSTACK_NETWORK.output(&dest);
}


/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){
    if(len == sizeof(struct Message)) {
        broadcastMsg receivedMsg;
        memcpy(&receivedMsg, data, sizeof(struct Message));
        
        if(receivedMsg.typeMsg != 10){
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

            if(linkaddr_cmp(&receivedMsg.addDest, &linkaddr_node_addr) || receivedMsg.typeMsg == 1 || receivedMsg.typeMsg == 3) {  //si msg lui est destiné ou si val par default = broadcast
                if (receivedMsg.typeMsg == 0) {                           //Keep-Alive received
                    updateRoutingTable(receivedRR);
                }
                if (receivedMsg.typeMsg == 1) {                           //receive parent request
                    LOG_INFO_("Computation : Receive parent request from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    //TODO : if place enough to get one more children
                    if (parent.hasParent) {
                        sendParentProposal(receivedMsg);
                    }
                }

                if (receivedMsg.typeMsg == 2) {   //received parent proposal
                    LOG_INFO_("Computation : Receive parent proposal from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    parentProposalComparison(receivedMsg);
                }
                if (receivedMsg.typeMsg == 3) {     //receive sensor value
                    LOG_INFO_("Computation : Receive sensor value from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    LOG_INFO_("[COMPUTE VAL] :  Receive sensor value");

                    if (parent.hasParent) {               //Vérif qu'il a bien un parent
                        //TODO vérif s'il peut prendre un sensor de plus sous son aile
                        //Verify it has still place to manage one more sensor
                        int hasToForward = 1;
                        int inList = 0;
                        int indiceInList = 0;
                        int indiceFreePlaceInList = 0;
                        for(int i = (lenDBSensorManaged -1) ; i >= 0 ; i--){
                            if(linkaddr_cmp(&DBSensorManaged[i].addSensor, &receivedMsg.addSrc){
                                LOG_INFO_("[COMPUTE VAL] :  Sensor already registered in Manage list at indice %d/5", i);

                                hasToForward = 0;   //Bc the Sensor is in his list of managed Sensor
                                inList = 1;
                                indiceInList = i;
                            }else if(DBSensorManaged[i].val[0] == -1){
                                LOG_INFO_("[COMPUTE VAL] :  check for last Free Space in Manage list : %d/5 is free", i);

                                indiceFreePlaceInList = i;              //will decrease until the first free place
                                hasToForward = 0;   //Bc free place for a Sensor in his list of managed Sensor
                            }
                        }
                        if(hasToForward){           //If he can't manage it --> forward
                            LOG_INFO_("[COMPUTE VAL] :  Manage list full --> forward the Sensor Value");

                            forward(receivedMsg);
                        }else{                      //Else --> manage it
                            LOG_INFO_("[COMPUTE VAL] :  ComputeAction --> at place %d in list, the %d ème value, %d indice is first free");

                            computeActionFromSensorValue(receivedMsg, inList, indiceInList, indiceFreePlaceInList);
                        }
                        updateRoutingTable(receivedRR);
                    }
                }
                if (receivedMsg.typeMsg == 4) {   //TODO
                    LOG_INFO_("Computation : Received Open Signal from ");
                    LOG_PRINT_LLADDR(&receivedMsg.addSrc);
                    LOG_INFO_(" \n");
                    forward(receivedMsg);
                }
            }
            else{          //il forward car pas pour lui (ni on add, ni broadcast)
                forward(receivedMsg);
            }
        }    
    }
}

void requestParentBroadcast(){
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.typeMsg = 1;
    msgPrep.addSrc = linkaddr_node_addr;

    nullnet_buf = (uint8_t *)&msgPrep;
    nullnet_len = sizeof(struct Message);
    NETSTACK_NETWORK.output(NULL); 

    LOG_INFO_("Computation : Sending Parent Request (broadcast)\n");
}

void deleteRecord(int indexToDelete){
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
        saveValSensor defaultValSensor;

        LOG_INFO_("[COMPUTE VAL] :  init the DB of 5 sensors values handled (all val to -1)");
        for (int i = 0 ; i < 30 ; i++){
            defaultValSensor.val[i] = -1;
        }
        for (int i = 0 ; i < lenDBSensorManaged ; i++){
            DBSensorManaged[i] = defaultValSensor;
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
            requestParentBroadcast();
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