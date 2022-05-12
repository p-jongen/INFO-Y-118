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
void constructHeader(uint8_t header[64], broadcastMsg objet){
    //add typeMessg dans Header
    uint8_t typeMsgTab[2];
    typeMsgTab[1] = (objet.typeMsg)/10;
    typeMsgTab[0] = (objet.typeMsg%10);
    for(int i = 0 ; i < lenTypeMsg ; i++){
        header[i] = typeMsgTab[i];
    }

    printf("\n");
    //add Rank dans Header
    uint8_t rankTab[3];
    rankTab[2] = (objet.rank)/100;
    rankTab[1] = (objet.rank%100)/10;
    rankTab[0] = (objet.rank%10);
    for(int i = 0 ; i < lenRank ; i++){
        header[i+lenTypeMsg] = rankTab[i];
    }
}