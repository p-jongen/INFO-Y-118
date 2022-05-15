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
    int hasParent;
};

typedef struct Record routingRecord;
struct Record {
    linkaddr_t addDest;
    linkaddr_t nextHop;
    int ttl;
};

typedef struct Message broadcastMsg;
struct Message {
    uint8_t rank;
    linkaddr_t addSrc;
    linkaddr_t addDest;
    int typeMsg;
    int sensorValue;
    int openValve;
};

typedef struct DB saveValSensor;
struct Message {
    linkaddr_t addSensor;
    int val[30];
};




















