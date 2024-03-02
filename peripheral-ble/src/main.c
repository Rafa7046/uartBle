#include <zephyr.h>
#include <kernel.h>
#include <peripheral.h>

int main()
{
    currentPeripheralState state = {
        .connRef = NULL,
        .status = peripheralDisconnected,
    };

    int status = peripheralStart(&state);
    if (0 != status)
        return status;

    initUart(&state);

    return 0;
}
