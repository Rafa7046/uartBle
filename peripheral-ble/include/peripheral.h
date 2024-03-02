#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#define MAX_BLE_MSG_SIZE 21

typedef enum
{
    bleConnected,
    peripheralDisconnected,
} status;

typedef struct
{
    status status;
    struct bt_conn *connRef;
} currentPeripheralState;

ssize_t onReceiveData(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

int peripheralStart(currentPeripheralState *state);
void initUart(currentPeripheralState *state);

#endif
