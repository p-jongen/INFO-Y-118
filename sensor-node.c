
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
#define SEND_INTERVAL (8 * CLOCK_SECOND)
#define SEND_INTERVAL_VALUE (60 * CLOCK_SECOND)


static node_t parent;
static int rank = 99;


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

void sendParentProposal(const linkaddr_t *src){
    LOG_INFO_("Computation : Send Unicast Parent Proposal to ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_(" \n");
    broadcastMsg msgPrep;           //prepare info
    msgPrep.rank = rank;
    msgPrep.typeMsg = 2;

    unsigned int header = buildHeader(msgPrep);    //build header (node.h)
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    NETSTACK_NETWORK.output(src);
}

void addParent(const linkaddr_t *src, int parentRankReceived){  //add or update info about his parent
    parent.address = *src;
    parent.rank = parentRankReceived;
    rank = parent.rank+1;
    parent.hasParent = 1;
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){
    int parentRankReceived = 0;
    if(len == sizeof(unsigned)) {
        unsigned bufData;
        memcpy(&bufData, data, 2);
        int typeMsgReceived = bufData/10000;

        if(typeMsgReceived == 1){   //Received parent request
            LOG_INFO_("Sensor : Receive Parent Request\n");
            if (parent.hasParent == 1){                   //n'envoie pas de proposal s'il n'a pas de parent lui-même
                sendParentProposal(src);
            }
        }

        if(typeMsgReceived == 2){   //received parent proposal
            LOG_INFO_("Sensor : Receive Parent Proposal\n");
            parentRankReceived = (bufData/100)%100;
            if (parent.hasParent == 0){
                addParent(src, parentRankReceived);

                LOG_INFO_("New parent for Sensor : ");
                LOG_INFO_LLADDR(src);
                LOG_INFO_(" (Parent rank = %d)\n", parent.rank);
            }else{  //déjà un parent
                if( parentRankReceived < parent.rank){
                    addParent(src, parentRankReceived);

                    LOG_INFO_("Better parent for Sensor :  ");
                    LOG_INFO_LLADDR(src);
                    LOG_INFO_(" (Parent rank = %d)\n", parent.rank);
                }
                else{
                    LOG_INFO_("Old parent holded for Sensor : ");
                }
            }
        }
        if(typeMsgReceived == 3){
            //TODO recept la valeur des 2 derniers chiffres du header. Si c'est un computation, faire le calcul et renvoyer la réponse vers sensor qui a envoyé ce paquet. Si c'est un sensor, forward vers parent

        }
    }
}

void sendValToParent(int val) {
    broadcastMsg msgPrep;
    msgPrep.rank = rank;
    msgPrep.typeMsg = 3;
    msgPrep.twoLast = val;

    unsigned int header = buildHeader(msgPrep);
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    NETSTACK_NETWORK.output(&parent.address);
}

    void requestParent(short rank){
    LOG_INFO_("Sensor : Sending Parent Request\n");
    broadcastMsg msgPrep;                   //prepare info
    msgPrep.rank = rank;
    msgPrep.typeMsg = 1;

    unsigned int header = buildHeader(msgPrep);    //(in node.h) -> build header TypeMsg(1)-Rank(23)-vide(45) = 12345
    nullnet_buf = (uint8_t *)&header;
    nullnet_len = 2;

    NETSTACK_NETWORK.output(NULL);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
    static struct etimer periodic_timer_parentRequest;
    static struct etimer periodic_timer_valueSensor;

    PROCESS_BEGIN();

    nullnet_set_input_callback(input_callback);         //LISTENER


    while(1) {
        //parent
        if(parent.hasParent == 0){                 //if no parent, send request
            etimer_set(&periodic_timer_parentRequest, SEND_INTERVAL);
            LOG_INFO_("Sensor parent : None\n");
            requestParent(rank);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_parentRequest));
            etimer_reset(&periodic_timer_parentRequest);
        }

        if(parent.hasParent == 1){                 //if no parent, send request
            int r = abs(rand() % 100);
            LOG_INFO_("Value of sensor : %d\n", r);
            sendValToParent(r);
            etimer_set(&periodic_timer_valueSensor, SEND_INTERVAL_VALUE);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer_valueSensor));
            etimer_reset(&periodic_timer_valueSensor);
        }
    
        

    }

    //nullnet_set_input_callback(input_callback);

    PROCESS_END();
}