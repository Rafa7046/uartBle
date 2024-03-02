#include <peripheral.h>
#include <ctype.h>

static const struct bt_data adv_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};
static struct bt_uuid_16 uuidUartUppercase = BT_UUID_INIT_16(0xAB01);
static struct bt_uuid_16 uuidUartReceiveData = BT_UUID_INIT_16(0xAB02);
static struct bt_uuid_16 uuidUartSendData = BT_UUID_INIT_16(0xAB03);
static struct bt_gatt_attr uart_gatt_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(&uuidUartUppercase),
    BT_GATT_CHARACTERISTIC(&uuidUartReceiveData.uuid, BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE, NULL, onReceiveData, NULL),
    BT_GATT_CHARACTERISTIC(&uuidUartSendData.uuid, BT_GATT_CHRC_READ, BT_GATT_PERM_WRITE, NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_WRITE),
};

static const int dataSizeAdvertising = ARRAY_SIZE(adv_data);
static int onStart(currentPeripheralState *state);
static void onConnect(struct bt_conn *conn, uint8_t status);
static void onDisconnect(struct bt_conn *conn, uint8_t status);
static int initAdvertising();

static currentPeripheralState *currentState = NULL;
static struct bt_gatt_service uart_to_uppercase_service = BT_GATT_SERVICE(uart_gatt_attrs);
static currentPeripheralState *currentState;

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = onConnect,
    .disconnected = onDisconnect,
};

static int onStart(currentPeripheralState *state)
{
    assert(NULL != state);
    currentState = state;

    return initAdvertising();
}

static void onConnect(struct bt_conn *conn, uint8_t status)
{
    assert(NULL != currentState);

    if (0 != status)
        return;

    currentState->connRef = bt_conn_ref(conn);
    currentState->status = bleConnected;
}

static void onDisconnect(struct bt_conn *conn, uint8_t reason)
{
    assert(NULL != currentState);

    if (NULL != currentState->connRef)
        bt_conn_unref(currentState->connRef);
    currentState->connRef = NULL;
    currentState->status = peripheralDisconnected;


    initAdvertising();
}

static int initAdvertising()
{
    bt_le_adv_start(BT_LE_ADV_CONN_NAME, adv_data, dataSizeAdvertising, NULL, 0);

    return 0;
}

int peripheralStart(currentPeripheralState *state)
{
    int status = bt_enable(NULL);
    if (0 != status)
        return status;

    onStart(state);

    return 0;
}

ssize_t onReceiveData(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    int datalen = len >= MAX_BLE_MSG_SIZE ? (MAX_BLE_MSG_SIZE - 1) : len;
    uint8_t data[MAX_BLE_MSG_SIZE];

    memcpy(data, buf, datalen);
    data[datalen] = '\0';

    char uppercase_data[MAX_BLE_MSG_SIZE];
    for (int i = 0; i < datalen; i++)
        uppercase_data[i] = toupper(data[i]);

    uppercase_data[datalen] = '\0';

    printk("Message: [ %s ] (%u/%u bytes)\n", uppercase_data, datalen, len);

    if (flags & BT_GATT_WRITE_FLAG_PREPARE)
        return BT_GATT_ERR(BT_ATT_ERR_SUCCESS);

    return datalen;
}

void initUart(currentPeripheralState *state)
{
    currentState = state;

    bt_gatt_service_register(&uart_to_uppercase_service);
}
