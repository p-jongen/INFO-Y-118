#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define lenRank 3       // rank --> 999
#define lenTypeMsg 2    // max 99 types messages
#define lenHeader 64    // length msg broad/uni-casted

// ========= STRUCTURES ===========
static typedef struct Node node_t;
struct Node {
    linkaddr_t address;
    uint8_t rank;
    node_t *parent; //if parent == NULL => Border router node
    int hasParent;
};

static typedef struct Record routingRecord;
struct Record {
    linkaddr_t addDest;
    linkaddr_t nextHop;
    int ttl;
};

static typedef struct Message broadcastMsg;
struct Message {
    uint8_t rank;
    linkaddr_t addSrc;
    linkaddr_t addDest;
    int typeMsg;
    int sensorValue;
    // 1 = Parent request
    // 2 = Parent proposal
};

// ======== global functions =========
int buildHeader(broadcastMsg msgPrep){          //max 65535
    int header = 0;
    header += msgPrep.typeMsg*10000;    //1 = demande de parent; 2 = envoi parent proposal
    header += msgPrep.rank*100;
    if(msgPrep.typeMsg==3){
        header+=msgPrep.twoLast;
    }
    //reste 2 derniers libres (*1 et *10)
    return header;
}