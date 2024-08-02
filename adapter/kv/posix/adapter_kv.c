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
#include "adapter_kv.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "securec.h"
#include "iotc_errcode.h"
#include "adapter_log.h"

#define MAX_KEY_LEN 128
#define MAX_TAG_LEN 128
#define MAX_FILE_NUMS 64
#define MAX_FILE_PATH_LEN 512

static char g_tag[MAX_TAG_LEN + 1] = {0};

static int32_t CreateConfigPath(const char *dir)
{
    /* 创建目录，0660表示创建目录的权限 */
    int32_t ret = mkdir(dir, 0660);
    if (ret != 0) {
        struct stat state;
        (void)memset_s(&state, sizeof(struct stat), 0, sizeof(struct stat));
        ret = stat(dir, &state);
        if ((ret != 0) || ((state.st_mode & S_IFDIR) == 0)) {
            ADAPTER_LOGE("create %s err %d", dir, ret);
            return IOTC_ADAPTER_KV_ERR_CREATE_DIR;
        }
    }
    return IOTC_OK;
}

int32_t AdapterKvInit(const char *tag)
{
    if (tag == NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t tagLen = strlen(tag);
    if (tagLen > MAX_TAG_LEN) {
        ADAPTER_LOGE("tagLen[%u] out of range", tagLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = CreateConfigPath(tag);
    if (ret != IOTC_OK) {
        return ret;
    }

    if (strcpy_s(g_tag, MAX_TAG_LEN, tag) != EOK) {
        ADAPTER_LOGE("strcpy tag err");
        return IOTC_ERR_SECUREC_STRCPY;
    }

    return IOTC_OK;
}

static int32_t CreateFile(const char *filePath)
{
    ADAPTER_LOGD("create %s", filePath);

    FILE *fp = fopen(filePath, "ab+");
    if (fp == NULL) {
        ADAPTER_LOGE("open or create %s error, errno: %d", filePath, errno);
        return IOTC_ADAPTER_KV_ERR_CREATE_FILE;
    }

    (void)fclose(fp);
    fp = NULL;
    if (chmod(filePath, S_IRUSR | S_IWUSR) != 0) {
        ADAPTER_LOGE("chmod error, errno:%d", errno);
        return IOTC_ADAPTER_KV_ERR_CREATE_FILE;
    }

    return IOTC_OK;
}

static bool CheckFileIsExisit(const char *filePath)
{
    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) {
        return false;
    }
    (void)fclose(fp);
    return true;
}

static int32_t KvSetValue(const char *filePath, const uint8_t *buf, uint32_t len)
{
    ADAPTER_LOGD("write %s", filePath);
    FILE *fp = fopen(filePath, "wb+");
    if (fp == NULL) {
        ADAPTER_LOGE("open file[%s] error, errno:%d", filePath, errno);
        return IOTC_ADAPTER_KV_ERR_SET_ITEM;
    }

    /* 写入1块，长度为len的数据 */
    if (fwrite(buf, 1, len, fp) != len) {
        ADAPTER_LOGE("write file[%s] error", filePath);
        (void)fclose(fp);
        return IOTC_ADAPTER_KV_ERR_SET_ITEM;
    }

    /* 数据从用户空间刷入内核缓冲区，不关心返回值 */
    (void)fflush(fp);
    int32_t fd = fileno(fp);
    if (fd != -1) {
        (void)fsync(fd);
    }

    (void)fclose(fp);
    return IOTC_OK;
}

int32_t AdapterKvSetValue(const char *key, const uint8_t *value, uint32_t len)
{
    if ((key == NULL) || (value == NULL) || (len == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        ADAPTER_LOGW("sprintf err");
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
    ADAPTER_LOGD("read %s", filePath);
    FILE *fp = fopen(filePath, "rb+");
    if (fp == NULL) {
        ADAPTER_LOGE("open file[%s] error, errno:%d", filePath, errno);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }

    uint32_t fileLen = 0;
    struct stat st;
    int32_t ret = stat(filePath, &st);
    if (ret != 0) {
        ADAPTER_LOGE("stat file[%s] error, errno:%d, ret:%d", filePath, errno, ret);
        (void)fclose(fp);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }
    fileLen = (uint32_t)st.st_size;
    if (fileLen > *len) {
        ADAPTER_LOGW("buffer not enough %u/%u", fileLen, *len);
        (void)fclose(fp);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }
    *len = fileLen;
    if (fread(buf, 1, *len, fp) != *len) {
        ADAPTER_LOGE("read file[%s] error", filePath);
        (void)fclose(fp);
        return IOTC_ADAPTER_KV_ERR_GET_ITEM;
    }

    (void)fclose(fp);
    return IOTC_OK;
}

int32_t AdapterKvGetValue(const char *key, uint8_t *buf, uint32_t *len)
{
    if ((key == NULL) || (key[0] == '\0') || (buf == NULL) || len == NULL || (*len == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        ADAPTER_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    return KvGetValue((const char *)filePath, buf, len);
}

static uint32_t GetFileLenByName(const char *filePath, uint32_t *len)
{
    struct stat st;
    int32_t ret = stat(filePath, &st);
    if (ret != 0) {
        ADAPTER_LOGD("stat file[%s] error, errno:%d, ret:%d", filePath, errno, ret);
        *len = 0;
        return (errno == ENOENT) ? IOTC_OK : IOTC_ADAPTER_KV_ERR_STAT_FILE;
    }
    *len = (uint32_t)st.st_size;
    return IOTC_OK;
}

int32_t AdapterKvGetLen(const char *key, uint32_t *len)
{
    if ((key == NULL) || (key[0] == '\0') || (len == NULL)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        ADAPTER_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    return GetFileLenByName((const char *)filePath, len);
}

static int32_t KvDelValue(const char *filePath)
{
    ADAPTER_LOGD("delete %s", filePath);
    int32_t ret = remove(filePath);
    if (ret != 0) {
        ADAPTER_LOGE("delete error %d", ret);
        return IOTC_ADAPTER_KV_ERR_DEL_FILE;
    }
    return IOTC_OK;
}

int32_t AdapterKvDelValue(const char *key)
{
    if ((key == NULL) || (key[0] == '\0')) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    char filePath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", g_tag, key) < 0) {
        ADAPTER_LOGW("sprintf err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    return KvDelValue((const char *)filePath);
}

int32_t AdapterKvDeInit(void)
{
    (void)memset_s(g_tag, sizeof(g_tag), 0, sizeof(g_tag));
    return IOTC_OK;
}