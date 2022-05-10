#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"

typedef struct Node node_t;
struct Node {
    linkaddr_t address;
    uint8_t rank;
    node_t *parent; //if parent == NULL => Border router node
};