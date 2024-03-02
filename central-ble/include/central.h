#ifndef CENTRAL_H
#define CENTRAL_H

#define MAX_BLE_MSG_SIZE 21

#include <stdint.h>
#include <assert.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/att.h>
#include <console/console.h>
#include <kernel/thread.h>

typedef struct
{
    struct bt_conn *conn_ref;
    void (*connectionSuccess)(void *central_state);
} central_state_t;

// methods:
void connectionSuccess(void *state);
int bleCentralStart();

#endif
