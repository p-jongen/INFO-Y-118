#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"

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
};

// ======== global functions =========
unsigned constructHeader(unsigned header, broadcastMsg objet){
    header = 100000;                     //caractère de début de header
    header += objet.typeMsg*1;
    header += (objet.rank*100);
    return header;
}