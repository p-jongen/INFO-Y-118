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
typedef struct Node node_t;
struct Node {
    linkaddr_t address;
    uint8_t rank;
    node_t *parent; //if parent == NULL => Border router node
};

typedef struct Message broadcastMsg;
struct Message {
    uint8_t rank;
    int typeMsg;
    // 1 = Broadcas_Information_from_Border (init Network, to contact Computation)
    // 2 = Demande de parent
};

// ======== global functions =========
int buildHeader(broadcastMsg msgPrep){          //max 65535
    int header = 0;
    header += msgPrep.typeMsg*10000;    //1 = demande de parent; 2 = envoi parent proposal
    header += msgPrep.rank*100;
    //reste 2 derniers libres (*1 et *10)
    return header;
}