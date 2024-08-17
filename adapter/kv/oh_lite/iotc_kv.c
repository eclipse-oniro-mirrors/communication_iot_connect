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
#include "iotc_kv.h"
#include <stdbool.h>
#include <stddef.h>
#include "securec.h"
#include "utils_file.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

#define MAX_KEY_LEN 128
#define MAX_TAG_LEN 128
#define MAX_FILE_NUMS 64
#define MAX_FILE_PATH_LEN 512
#define FILE_LEN_INVALID 0

#define CHECK_IS_INIT \
    do { \
        if (!g_isInit) { \
            IOTC_LOGE("kv not init"); \
            return IOTC_ADAPTER_KV_ERR_INIT; \
        } \
    } while (0)

static char g_tag[MAX_TAG_LEN + 1] = {0};
static bool g_isInit = false;

int32_t IotcKvInit(const char *tag)
{
    if (tag == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    if (g_isInit) {
        IOTC_LOGE("kv reinit");
        return IOTC_ADAPTER_KV_ERR_INIT;
    }

    uint32_t tagLen = strlen(tag);
    if (tagLen > MAX_TAG_LEN) {
        IOTC_LOGE("tagLen[%u] out of range", tagLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    if (strcpy_s(g_tag, MAX_TAG_LEN, tag) != EOK) {
        IOTC_LOGE("strcpy tag err");
        return IOTC_ERR_SECUREC_STRCPY;
    }
    g_isInit = true;

    return IOTC_OK;
}

static int32_t CreateFile(const char *filePath)
{
    IOTC_LOGD("create %s", filePath);
    int32_t fd = UtilsFileOpen(filePath, O_RDWR_FS | O_CREAT_FS, 0);
    if (fd < 0) {
        IOTC_LOGE("open or create error");
        return IOTC_ADAPTER_KV_ERR_CREATE_FILE;
    }
    UtilsFileClose(fd);
    return IOTC_OK;
}

static bool CheckFileIsExisit(const char *filePath)
{
    int32_t fd = UtilsFileOpen(filePath, O_RDONLY_FS, 0);
    if (fd < 0) {
        return false;
    } else {
        UtilsFileClose(fd);
        return true;
    }
}

static int32_t KvSetValue(const char *filePath, const uint8_t *buf, uint32_t len)
{
    IOTC_LOGD("write %s", filePath);
    int32_t fd = UtilsFileOpen(filePath, O_RDWR_FS | O_CREAT_FS, 0);
    if (fd < 0) {
        IOTC_LOGE("open file[%s] error", filePath);
        return IOTC_ADAPTER_KV_ERR_SET_ITEM;
    }

    int32_t ret = UtilsFileWrite(fd, (uint8_t *)buf, len);
    if (ret < 0) {
        IOTC_LOGE("write file[%s] error", filePath);
        UtilsFileClose(fd);
        return IOTC_ADAPTER_KV_ERR_SET_ITEM;
    }
    UtilsFileClose(fd);
    return IOTC_OK;
}

int32_t IotcKvSetValue(const char *key, const uint8_t *value, uint32_t len)
{
    if ((key == NULL) || (value == NULL) || (len == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    CHECK_IS_INIT;

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        IOTC_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (!CheckFileIsExisit((const char *)filePath)) {
        if (CreateFile((const char *)filePath) != IOTC_OK) {
            return IOTC_ADAPTER_KV_ERR_CREATE_FILE;
        }
    }
    return KvSetValue((const char *)filePath, value, len);
}

static int32_t KvGetValue(const char *filePath, uint8_t *buf, uint32_t *len)
{
    IOTC_LOGD("read %s", filePath);
    int32_t fd = UtilsFileOpen(filePath, O_RDWR_FS | O_CREAT_FS, 0);
    if (fd < 0) {
        IOTC_LOGE("open file[%s] error", filePath);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }

    uint32_t fileLen = 0;
    int32_t ret = UtilsFileStat(filePath, &fileLen);
    if (ret != 0) {
        IOTC_LOGE("stat file[%s] error", filePath);
        UtilsFileClose(fd);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }
    if (fileLen > *len) {
        IOTC_LOGW("buffer not enough %u/%u", fileLen, *len);
        UtilsFileClose(fd);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }
    *len = fileLen;
    ret = UtilsFileRead(fd, buf, *len);
    if (ret < 0) {
        IOTC_LOGE("read file[%s] error", filePath);
        UtilsFileClose(fd);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }
    UtilsFileClose(fd);
    return IOTC_OK;
}

int32_t IotcKvGetValue(const char *key, uint8_t *buf, uint32_t *len)
{
    if ((key == NULL) || (key[0] == '\0') || (buf == NULL) || len == NULL || (*len == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    CHECK_IS_INIT;

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        IOTC_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    return KvGetValue((const char *)filePath, buf, len);
}

static uint32_t GetFileLenByName(const char *filePath, uint32_t *len)
{
    int32_t fd = UtilsFileOpen(filePath, O_RDWR_FS | O_CREAT_FS, 0);
    if (fd < 0) {
        IOTC_LOGE("open file[%s] error", filePath);
        return IOTC_ADAPTER_KV_ERR_OPEN_FILE;
    }

    int32_t ret = UtilsFileStat(filePath, len);
    if (ret != 0) {
        IOTC_LOGE("stat file[%s] error", filePath);
        UtilsFileClose(fd);
        *len = 0;
        return IOTC_ADAPTER_KV_ERR_STAT_FILE;
    }
    UtilsFileClose(fd);
    return IOTC_OK;
}

int32_t IotcKvGetLen(const char *key, uint32_t *len)
{
    if ((key == NULL) || (key[0] == '\0')) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    CHECK_IS_INIT;

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        IOTC_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    int32_t ret = GetFileLenByName((const char *)filePath, len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get fileLen err");
    }
    return IOTC_OK;
}

static int32_t KvDelValue(const char *filePath)
{
    IOTC_LOGD("delete %s", filePath);
    int32_t ret = UtilsFileDelete(filePath);
    if (ret != 0) {
        IOTC_LOGE("delete error %d", ret);
        return IOTC_ADAPTER_KV_ERR_DEL_FILE;
    }
    return IOTC_OK;
}

int32_t IotcKvDelValue(const char *key)
{
    if ((key == NULL) || (key[0] == '\0')) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!g_isInit) {
        IOTC_LOGE("kv not init");
        return IOTC_ADAPTER_KV_ERR_INIT;
    }

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        IOTC_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    return KvDelValue((const char *)filePath);
}

int32_t IotcKvDeInit(void)
{
    CHECK_IS_INIT;
    (void)memset_s(g_tag, sizeof(g_tag), 0, sizeof(g_tag));
    g_isInit = false;
    return IOTC_OK;
}