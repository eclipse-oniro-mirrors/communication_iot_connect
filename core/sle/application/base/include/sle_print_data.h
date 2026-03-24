#ifndef SLE_PRINT_DATA_H
#define SLE_PRINT_DATA_H
#include "utils_json.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SlePrintfData(const uint8_t* data, uint16_t total_len);
int32_t SleJsonGetString(const IotcJson *json, const char *key, const char **out_str);
#ifdef __cplusplus
}
#endif

#endif /* SLE_PRINT_DATA_H*/