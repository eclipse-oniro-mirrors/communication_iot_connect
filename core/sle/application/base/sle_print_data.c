#include "sle_print_data.h"
#include <stddef.h>
#include <stdio.h>
#include "iotc_log.h"
#include "securec.h"
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