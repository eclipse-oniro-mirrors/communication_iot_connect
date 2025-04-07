
/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "iotc_sle_ext.h"
#include "iotc_sle_def.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_sle_server.h"

static uint8_t g_chara_val[] = {0x11, 0x22, 0x33, 0x44};
static uint8_t g_desc_val[]  = {0x55, 0x66, 0x77, 0x88};
#define UUID16_LEN 2
#define UUID32_LEN 4
#define UUID128_LEN 16
#define BLE_MAX_SERVICES_NUMS 16
#define BLE_HILINK_SERVER_LOG "[BLE_HILINK_SERVER]"
#define BLE_ADV_HANDLE_DEFAULT 1
#define INVALID_SERVER_ID 0
#define EXT_ADV_OR_SCAN_RSP_DATA_LEN 251
#define MAX_READ_REQ_LEN 200

static uint16_t g_services_handle[BLE_MAX_SERVICES_NUMS] = {0};
static uint16_t g_server_request_id = 0;
static uint16_t g_srvc_handle = 0;
static uint16_t g_cb_chara_handle = 0;
static uint16_t g_cb_desc_handle = 0;
static uint16_t g_indicate_handle = 17;
static uint8_t g_adv_status = 0;
static uint8_t g_io_cap_mode = 0;
static uint8_t g_sc_mode = 0;
static uint8_t g_gatt_write_flag = 0;   /* 0:write 1:read */
static uint8_t g_service_flag = 0;      /* 0:enable 1:disable start service */
static uint8_t g_server_id = INVALID_SERVER_ID;  /* gatt server ID */


static uint8_t g_hilink_group_cnt = 0;
#define HILINK_SERVICE_NUM 3
#define HILINK_CHARA_NUM 4
static char g_hilink_group_uuid[HILINK_SERVICE_NUM + HILINK_CHARA_NUM][OHOS_SLE_UUID_MAX_LEN] = {
    {0x15, 0xF1, 0xE4, 0x00, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00},
    {0x15, 0xF1, 0xE4, 0x01, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00},
    {0x15, 0xF1, 0xE5, 0x00, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00},
    {0x15, 0xF1, 0xE5, 0x01, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00},
    {0x15, 0xF1, 0xE6, 0x00, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00},
    {0x15, 0xF1, 0xE6, 0x02, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00},
    {0x15, 0xF1, 0xE6, 0x01, 0xA2, 0x77, 0x43, 0xFC, 0xA4, 0x84, 0xDD, 0x39, 0xEF, 0x8A, 0x91, 0x00}
};
static char g_hilink_cccd_uuid[UUID16_LEN] = {0x29, 0x02};

typedef struct {
    int conn_id;
    int attr_handle;  /* The handle of the attribute to be read */
    SleServiceRead read;
    SleServiceWrite write;
    SleServiceIndicate indicate;
} hilink_sle_func;
hilink_sle_func g_charas_func[10] = {{0}};     /* 设置最大Service数量10 */
static uint8_t g_chara_cnt = 0;

static void hilnk_group_add(void)
{
    g_hilink_group_cnt++;
}


static uint8_t sle_uuid_server_init(void)
{
    IOTC_LOGE("sle ext begin create server");
    uint8_t ret = IOTC_OK;
    IotcSleUuidAddr app_uuid = {0};
    char uuid[] = {0x12, 0x34};
    app_uuid.len = sizeof(uuid);
    ret = memcpy_s(app_uuid.uuid, app_uuid.len, uuid, sizeof(uuid));
    if (ret != IOTC_OK) {
        IOTC_LOGE("%s sle ext uuid cpy fail:%d", ret);
        return -1;
    }
    ret = IotcAddSsapServer(&app_uuid, &g_server_id);
    if (ret != IOTC_OK) {
        IOTC_LOGE("%s sle ext create server fail:%d", ret);
        return ret;
    }
    sleep(10);
    IOTC_LOGE("sle ext create server success");
    return ret;
}

static void reverse_uuid(uint8_t *input, char *output, int len)
{
    for (int i = 0; i < len; i++) {
        output[i] = input[len - i - 1];
    }
}

static void convert_uuid(uint8_t *uuid_input, SleUuidType type, IotcSleUuid *uuid_output)
{
    uint8_t temp_uuid[OHOS_SLE_UUID_MAX_LEN] = {0};
    int ret = 0;

    switch (type) {
        case OHOS_SLE_UUID_TYPE_16_BIT:
            uuid_output->uuidLen = UUID16_LEN;
            break;
        case OHOS_SLE_UUID_TYPE_32_BIT:
            uuid_output->uuidLen = UUID32_LEN;
            break;
        case OHOS_SLE_UUID_TYPE_128_BIT:
            uuid_output->uuidLen = UUID128_LEN;
            break;
        default:
            uuid_output->uuidLen = 0;
            break;
    }

    uuid_output->uuid = (char *)uuid_input;
    ret = memcpy_s(temp_uuid, OHOS_SLE_UUID_MAX_LEN, g_hilink_group_uuid[g_hilink_group_cnt], OHOS_SLE_UUID_MAX_LEN);
    if (ret != 0) {
        IOTC_LOGE("%s sle ext convert uuid cpy fail:%d", ret);
        return;
    }
    reverse_uuid(temp_uuid, uuid_output->uuid, uuid_output->uuidLen);
    return;
}

static uint32_t perm_bt_to_bluez(uint32_t permissions)
{

    uint32_t perm = 0;
    if (permissions & IOTC_SLE_SSAP_PERMISSION_READ) {
        perm |= IOTC_SLE_SSAP_PERMISSION_READ;
    }
    if (permissions & IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED) {
        perm |= (IOTC_SLE_SSAP_PERMISSION_READ | IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM);
    }
    if (permissions & IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM) {
        perm |= (IOTC_SLE_SSAP_PERMISSION_READ | 
            IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM | IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED);
    }
    if (permissions & IOTC_SLE_SSAP_PERMISSION_WRITE) {
        perm |= IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED;
    }
    if (permissions & IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED) {
        perm |= (IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED | IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM);
    }
    if (permissions & IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED_MITM) {
        perm |= (IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED |
            IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM | IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED);
    }
    IOTC_LOGI("perm_bt_to_bluez %d to %d\n", permissions, perm);
    return perm;
}

static void set_chara_func(SleAttr *attr, uint8_t is_indicate)
{
    g_charas_func[g_chara_cnt].conn_id = 0;
    if (is_indicate == 0) {
        if (attr->attrType == OHOS_SLE_ATTRIB_TYPE_CHAR) {
            g_charas_func[g_chara_cnt].attr_handle = g_cb_chara_handle;
        } else {
            g_charas_func[g_chara_cnt].attr_handle = g_cb_desc_handle;
        }
        g_charas_func[g_chara_cnt].read       = attr->func.read;
        g_charas_func[g_chara_cnt].write      = attr->func.write;
        g_charas_func[g_chara_cnt].indicate   = attr->func.indicate;
    } else {
        g_charas_func[g_chara_cnt].attr_handle = g_cb_desc_handle;
    }
    g_chara_cnt++;
}

int32_t  IotcSleStartServiceEx(int *srvcHandle, SleService *srvcInfo)
{
    IOTC_LOGI(" IotcSleStartServiceEx enter srvHandle:%d.\n", *srvcHandle);
    int32_t ret = IOTC_OK;
    uint8_t is_indicate = 0;
    IotcSleUuid sle_uuid = {0};
    uint16_t service_handle = 0;
    uint16_t desc_handle = 0;
    uint16_t props_handle = 0;
    uint16_t  properties = 0;
    if (g_server_id == INVALID_SERVER_ID) {
        uint8_t ret = sle_uuid_server_init();
        if (ret != IOTC_OK) {
            IOTC_LOGE("%s sle ext create server fail:%d", ret);
            return -1;
        }
    }

    for (unsigned int i = 0; i < srvcInfo->attrNum; i++) {
        SleAttr *attr = &(srvcInfo->attrList[i]);
        convert_uuid(attr->uuid, attr->uuidType, &sle_uuid);
        switch (attr->attrType) {
            case OHOS_SLE_ATTRIB_TYPE_SERVICE:
                {
                IotcSleUuidAddr service_uuid = {0};
                service_uuid.len = sle_uuid.uuidLen;
                (void)memcpy_s(service_uuid.uuid, sle_uuid.uuidLen, (uint8_t *)sle_uuid.uuid, sle_uuid.uuidLen);
                IOTC_LOGE("sle ext begin create service");
                ret = IotcSsapsAddService(g_server_id, &service_uuid, true, &service_handle);
                if (ret != IOTC_OK) {
                    IOTC_LOGE("sle ext add service fail:%d", ret);
                }
                g_srvc_handle = service_handle;
                hilnk_group_add();
                 IOTC_LOGE("sle ext create service success");
                }
               
                break;
            case OHOS_SLE_ATTRIB_TYPE_CHAR:
                {
                IotcSleUuidAddr service_uuid = {0};
                service_uuid.len = sle_uuid.uuidLen;
                (void)memcpy_s(service_uuid.uuid, sle_uuid.uuidLen, (uint8_t *)sle_uuid.uuid, sle_uuid.uuidLen);
                properties = attr->properties;
                IotcAdptSleSsapsPropertyInfo propertyParam = {0};
                propertyParam.uuid = service_uuid;
                propertyParam.permissions = perm_bt_to_bluez(attr->permission);
                propertyParam.operateIndication = attr->properties;
                propertyParam.valueLen = sizeof(g_chara_val);
                propertyParam.value = g_chara_val;
                 IOTC_LOGE("sle ext begin add props %d",attr->properties);
                ret = IotcSsapsAddProperty(g_server_id, g_srvc_handle,
                    &propertyParam, &props_handle);
                if (ret != IOTC_OK) {
                    IOTC_LOGE("sle ext add property fail:%d", ret);
                }
                hilnk_group_add();
                }

                break;
            case OHOS_SLE_ATTRIB_TYPE_CHAR_VALUE:
                break;
            case OHOS_SLE_ATTRIB_TYPE_CHAR_CLIENT_CONFIG:
                break;
            case OHOS_SLE_ATTRIB_TYPE_CHAR_USER_DESCR:
                {
                IotcSleUuidAddr service_uuid = {0};
                service_uuid.len = sle_uuid.uuidLen;
                (void)memcpy_s(service_uuid.uuid, sle_uuid.uuidLen, (uint8_t *)sle_uuid.uuid, sle_uuid.uuidLen);
               IotcAdptSleSsapsDescInfo descriptor={0};
               descriptor.uuid = service_uuid;
               descriptor.permissions = perm_bt_to_bluez(attr->permission);
               descriptor.valueLen = sizeof(g_desc_val);
               descriptor.value = g_desc_val;
               IOTC_LOGI("sle ext begin add desc");
               ret = IotcSsapsAddDescriptor(g_server_id, g_srvc_handle,props_handle, &descriptor, &desc_handle);
                if (ret != IOTC_OK) {
                    IOTC_LOGE("sle ext add descriptor fail:%d", ret);
                }
                g_cb_desc_handle = desc_handle;
                hilnk_group_add();
                }

                break;
            default:
                IOTC_LOGE("sle ext unknow");
        }

        if ((attr->attrType == OHOS_SLE_ATTRIB_TYPE_CHAR_USER_DESCR) || (attr->attrType == OHOS_SLE_ATTRIB_TYPE_CHAR)) {
            set_chara_func(attr, 0);
        }

        if ((attr->properties & IOTC_ADPT_SLE_CHAR_PROP_INDICATE) ||
            (attr->properties &IOTC_ADPT_SLE_CHAR_PROP_NOTIFY)) {
            is_indicate = 1;
            g_indicate_handle = g_cb_chara_handle;
        }
    }

    if (is_indicate) {
        IOTC_LOGI("sle ext indicate");
        sle_uuid.uuid = g_hilink_cccd_uuid;
        sle_uuid.uuidLen = sizeof(g_hilink_cccd_uuid);
        IotcSleUuidAddr service_uuid = {0};
        service_uuid.len = sle_uuid.uuidLen;
        (void)memcpy_s(service_uuid.uuid, sle_uuid.uuidLen, (uint8_t *)sle_uuid.uuid, sle_uuid.uuidLen);
        IotcAdptSleSsapsDescInfo descriptor={0};
        
        descriptor.uuid = service_uuid;
        descriptor.permissions = perm_bt_to_bluez(IOTC_SLE_SSAP_PERMISSION_READ|IOTC_SLE_SSAP_PERMISSION_WRITE);
        descriptor.operateInd = properties;
        descriptor.valueLen = sizeof(g_desc_val);
        descriptor.value = g_desc_val;
        ret = IotcSsapsAddDescriptor(g_server_id, g_srvc_handle,props_handle, &descriptor, &desc_handle);
        if (ret == IOTC_OK) {
            set_chara_func(NULL, is_indicate);
            g_cb_desc_handle = desc_handle;
        }
    }

    if (g_srvc_handle != 0) {
        IOTC_LOGE("sle ext begin start service");
        ret = IotcSleSsapsStartService(g_server_id, g_srvc_handle);
        *srvcHandle = g_srvc_handle;
    }

    return 0;
}
