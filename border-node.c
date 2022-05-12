
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
#define SEND_INTERVAL (8 * CLOCK_SECOND)


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    LOG_INFO("Receiving Border out IF\n");
    if(len == sizeof(int)) {

        LOG_INFO("Receiving Border in IF INT\n");
        unsigned count;
        memcpy(&count, data, sizeof(count));
        /*
        LOG_INFO("Received %u from ", count);
        LOG_INFO_LLADDR(src);
        LOG_INFO_("\n");
         */
    }else if (len == sizeof(uint8_t)){
        LOG_INFO("Receiving Border in IF UINT8_t\n");
    }else if (len == sizeof(uint16_t)){
        LOG_INFO("Receiving Border in IF UINT16_t\n");
    }else if (len == sizeof(int[64])){
        LOG_INFO("Receiving Border in IF INT[64]\n");
    }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data){
    static struct etimer periodic_timer;

    PROCESS_BEGIN();

    nullnet_set_input_callback(input_callback); //LISTENER packet

    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        NETSTACK_NETWORK.output(NULL);

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}