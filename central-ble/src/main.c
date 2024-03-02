#include <zephyr.h>
#include <kernel.h>
#include <console/console.h>
#include <central.h>

central_state_t state;

void main()
{
    printk("Starting Central...\n");

    state.conn_ref = NULL;
    state.connectionSuccess = connectionSuccess;
    bleCentralStart(&state);

    return;
}
