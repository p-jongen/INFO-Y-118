#include "contiki.h"
#include <stdio.h>

typedef struct Node node_t;
struct Node {
    linkaddr_t address;
    uint8_t rank;
    node_t *parent; //if parent == NULL => Border router node
}