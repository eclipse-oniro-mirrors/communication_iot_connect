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

#ifndef IOTC_ADPT_SLE_EXT_H
#define IOTC_ADPT_SLE_EXT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#define OHOS_SLE_UUID_MAX_LEN 16

typedef enum {
    OHOS_SLE_ATTRIB_TYPE_SERVICE = 0x00,
    OHOS_SLE_ATTRIB_TYPE_CHAR,
    OHOS_SLE_ATTRIB_TYPE_CHAR_VALUE,
    OHOS_SLE_ATTRIB_TYPE_CHAR_CLIENT_CONFIG,
    OHOS_SLE_ATTRIB_TYPE_CHAR_USER_DESCR,
} SleAttribType;

typedef enum {
    OHOS_SLE_UUID_TYPE_NULL = 0x00,
    OHOS_SLE_UUID_TYPE_16_BIT,
    OHOS_SLE_UUID_TYPE_32_BIT,
    OHOS_SLE_UUID_TYPE_128_BIT,
} SleUuidType;


typedef int (*SleServiceRead)(unsigned char *buff, unsigned int *len);

typedef int (*SleServiceWrite)(unsigned char *buff, unsigned int len);

typedef int (*SleServiceIndicate)(unsigned char *buff, unsigned int len);

typedef struct {
    SleServiceRead read;
    SleServiceWrite write;
    SleServiceIndicate indicate;
} SleOperateFunc;

typedef struct {
    SleAttribType attrType;
    unsigned int permission; /* e.g. (OHOS_GATT_PERMISSION_READ | OHOS_GATT_PERMISSION_WRITE) */
    SleUuidType uuidType;
    unsigned char uuid[OHOS_SLE_UUID_MAX_LEN];
    unsigned char *value;
    unsigned char valLen;
    /* e.g. (OHOS_GATT_CHARACTER_PROPERTY_BIT_BROADCAST | OHOS_GATT_CHARACTER_PROPERTY_BIT_READ) */
    unsigned char properties;
    SleOperateFunc func;
} SleAttr;

typedef struct {
    unsigned int attrNum;
    SleAttr *attrList;
} SleService;

/**
 * @brief Adds a service, its characteristics, and descriptors and starts the service.
 *
 * This function is available for system applications only.
 *
 * @param srvcHandle Indicates the pointer to the handle ID of the service,
 * which is returned by whoever implements this function.
 * @param srvcInfo Indicates the pointer to the service information.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the operation is successful;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
int32_t IotcSleStartServiceEx(int *srvcHandle, SleService *srvcInfo);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_ADPT_SLE_EXT_H*/