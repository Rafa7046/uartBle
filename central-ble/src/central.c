#include <central.h>

static int lookForDevices();
static void onConnect(struct bt_conn *conn, uint8_t status);
static void onDisconnect(struct bt_conn *conn, uint8_t status);
static void onDeviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple *buf);
static int gatt_discover();
static int subscribe();
static void initMessageSend();
static void sendMessage(char *message);
static void writeSubscribe(struct bt_conn *conn, uint8_t status, struct bt_gatt_write_params *params);
static uint8_t notifySubscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t length);
static uint8_t gattFunction(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params);

static struct bt_uuid_16 uuidUartUppercase = BT_UUID_INIT_16(0xAB01);
static struct bt_uuid_16 uuidUartReceiveData = BT_UUID_INIT_16(0xAB02);
static struct bt_uuid_16 uuidUartSendData = BT_UUID_INIT_16(0xAB03);

static struct bt_gatt_discover_params findFunctions = {
    .uuid = &uuidUartUppercase.uuid,
    .func = gattFunction,
    .start_handle = BT_ATT_FIRST_ATTTRIBUTE_HANDLE,
    .end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE,
    .type = BT_GATT_DISCOVER_PRIMARY,
};

static struct bt_gatt_subscribe_params subscriptionParams = {
    .notify = notifySubscribe,
    .write = writeSubscribe,
    .ccc_handle = 0,
    .disc_params = &findFunctions,
    .end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE,
    .value = BT_GATT_CCC_NOTIFY,
};

static central_state_t *currentState;
static uint16_t receivedData;
static atomic_t isWrite = false;

static central_state_t *currentState = NULL;

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = onConnect,
    .disconnected = onDisconnect,
};

static int checkStatusInt(int status)
{
    if (0 != status)
        return status;

    return 0;
}

static int lookForDevices()
{
    int status = bt_le_scan_start(BT_LE_SCAN_PASSIVE, onDeviceFound);
    return checkStatusInt(status);
}

static void onDeviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple *buf)
{
    assert(NULL != currentState);

    if (NULL != currentState->conn_ref)
        return;

    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, BT_ADDR_LE_STR_LEN);

    if (0 != bt_le_scan_stop() || (adv_type != BT_GAP_ADV_TYPE_ADV_IND))
        return;

    int status = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &currentState->conn_ref);
    if (0 != status)
        lookForDevices();

    return;
}

static void onConnect(struct bt_conn *conn, uint8_t status)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, BT_ADDR_LE_STR_LEN);

    if (0 != status)
    {
        if (currentState->conn_ref)
            bt_conn_unref(currentState->conn_ref);
        currentState->conn_ref = NULL;

        lookForDevices();

        return;
    }

    if (conn != currentState->conn_ref)
        return;

    if (NULL != currentState->connectionSuccess)
        currentState->connectionSuccess(currentState);
}

static void onDisconnect(struct bt_conn *conn, uint8_t reason)
{
    if (conn != currentState->conn_ref)
        return;

    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, BT_ADDR_LE_STR_LEN);

    bt_conn_unref(currentState->conn_ref);
    currentState->conn_ref = NULL;

    lookForDevices();
}

int bleCentralStart(central_state_t *state)
{
    assert(NULL != state);

    int status = bt_enable(NULL);

    if (0 != status)
        return status;

    currentState = state;

    return lookForDevices();
}

K_THREAD_DEFINE(transmitter_thread, 1024, initMessageSend, NULL, NULL, NULL, 1, 0, 0);

static void initMessageSend()
{
    while (!atomic_get(&isWrite))
        k_sleep(K_MSEC(100));

    console_getline_init();

    while (1)
    {
        printk(">>> ");
        char *line = console_getline();

        if (NULL == line)
            continue;

        sendMessage(line);

        k_sleep(K_MSEC(100));
    }
}

static void sendMessage(char *message)
{

    assert(NULL != currentState->conn_ref);

    int status = bt_gatt_write_without_response(currentState->conn_ref, receivedData, message, strlen(message), false);
    if (0 != status)
        return;
}

static uint8_t notifySubscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t length)
{
    short response_len = length >= MAX_BLE_MSG_SIZE ? (MAX_BLE_MSG_SIZE - 1) : length;
    char response[MAX_BLE_MSG_SIZE];
    memcpy(response, data, response_len);
    response[response_len] = '\0';
    return BT_GATT_ITER_CONTINUE;
}

static void writeSubscribe(struct bt_conn *conn, uint8_t status, struct bt_gatt_write_params *params) {
    atomic_set(&isWrite, true);
}

static uint8_t gattFunction(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
    if (NULL == attr)
    {
        memset(params, 0, sizeof(*params));

        subscribe();

        return BT_GATT_ITER_STOP;
    }

    if (0 == bt_uuid_cmp(params->uuid, &uuidUartUppercase.uuid))
    {

        params->uuid = NULL;
        params->start_handle = attr->handle + 1;
        params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

        int status = bt_gatt_discover(conn, params);
        if (0 != status)
            printk("Error : %d", status);

        return BT_GATT_ITER_STOP;
    }
    else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC)
    {
        struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

        if (0 == bt_uuid_cmp(chrc->uuid, &uuidUartReceiveData.uuid))
            receivedData = chrc->value_handle;
        else if (0 == bt_uuid_cmp(chrc->uuid, &uuidUartSendData.uuid))
            k_sleep(K_MSEC(100));
    }

    return BT_GATT_ITER_CONTINUE;
}

static int gatt_discover()
{
    assert(NULL != currentState);
    assert(NULL != currentState->conn_ref);

    int status = bt_gatt_discover(currentState->conn_ref, &findFunctions);
    return checkStatusInt(status);
}

static int subscribe()
{
    assert(NULL != currentState);
    assert(NULL != currentState->conn_ref);

    subscriptionParams.value_handle = receivedData;
    int status = bt_gatt_subscribe(currentState->conn_ref, &subscriptionParams);
    return checkStatusInt(status);
}

void connectionSuccess(void *state)
{
    currentState = (central_state_t *)state;
    gatt_discover();
    return;
}
