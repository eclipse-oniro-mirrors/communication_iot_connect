#include "sle_print_data.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iotc_log.h"
#include "securec.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_conf.h"
#include "iotc_errcode.h"
static void IotcSlePrintData(const uint16_t valueLen,const uint8_t *value) {
    size_t buf_len = (size_t)valueLen * 3 + 1;
    char *hex_str = malloc(buf_len);
    if (!hex_str) {
        IOTC_LOGI("IotcSlePrintData malloc failed");
        return;
    }
    char *p = hex_str;
    size_t remaining = buf_len;

    for (uint32_t i = 0; i < valueLen ; i++) {
        int n = snprintf(p, remaining, "%02X ", value[i]);
        if (n < 0 || (size_t)n >= remaining) {
            break;
        }
        p += n;
        remaining -= n;
    }
    IOTC_LOGI("IotcSlePrintData First %u bytes: %s", valueLen, hex_str);
    free(hex_str);
}

void SlePrintfData(const uint8_t* data, uint16_t total_len) {
    const uint16_t CHUNK_SIZE = 50;
    uint16_t processed = 0;

    while (processed < total_len) {
        uint16_t remaining = total_len - processed;
        uint16_t chunk_len = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        IotcSlePrintData(chunk_len, data + processed);

        processed += chunk_len;
    }
}

int32_t SleJsonGetString(const IotcJson *json, const char *key, const char **out_str)
{
    CHECK_RETURN(json   != NULL &&
    key    != NULL &&
    out_str!= NULL,
    IOTC_ERR_PARAM_INVALID);

    const char *src = IotcJsonGetStr(IotcJsonGetObj(json, key));
    if (src == NULL) {
        IOTC_LOGW("get json str error %s", key);
        return IOTC_ADAPTER_JSON_ERR_GET_STRING;
    }

    if (*out_str != NULL && strcmp(*out_str, src) == 0) {
        IOTC_LOGD("string content is the same, no need to reallocate");
        return IOTC_OK;
    }

    if (*out_str != NULL) {
        IotcFree((char *)*out_str);
        IOTC_LOGW("free old string %s", key);
    }

    char *dup = strdup(src);
    if (dup == NULL) {
        IOTC_LOGE("strdup(%s) failed", key);
        *out_str = NULL;
        return IOTC_ERROR;
    }

    *out_str = dup;
    return IOTC_OK;
}