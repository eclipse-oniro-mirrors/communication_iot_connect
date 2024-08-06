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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "iotc_oh_wifi.h"
#include "iotc_oh_ble.h"
#include "iotc_oh_connect.h"
#include "iotc_oh_device.h"
#include "securec.h"
#include "cJSON.h"
#include "iotc_conf.h"

#define DEMO_LOG(...) do { \
        printf("DEMO[%s:%u]", __func__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\r\n"); \
    } while (0)

static const IotcDeviceInfo DEV_INFO = {
    .sn = "12345678",
#if IOTC_CONF_AILIFE_SUPPORT
    .prodId = "2F6R",
#else
    .prodId = "2F6R0",
#endif
    .subProdId = "",
    .model = "DL-01W",
#if IOTC_CONF_AILIFE_SUPPORT
    .devTypeId = "046",
#else
    .devTypeId = "0460",
#endif
    .devTypeName = "Table Lamp",
    .manuId = "17C",
    .manuName = "DALEN",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_WIFI,
};

static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},
};

const char *PIN_CODE = "01234567";

const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 0x36, 0x32, 0x50, 0x3C, 0x49, 0x39, 0x62, 0x38,
    0x72, 0xCB, 0x6D, 0xC5, 0xAE, 0xE5, 0x4A, 0x82, 0xD3, 0xE5, 0x6D, 0xF5, 0x36, 0x82, 0x62, 0xEB,
    0x89, 0x30, 0x6C, 0x88, 0x32, 0x56, 0x23, 0xFD, 0xB8, 0x67, 0x90, 0xA7, 0x7B, 0x61, 0x1E, 0xAE
};

static bool g_switch = false;

static int32_t SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        DEMO_LOG("parse error");
        return -1;
    }

    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        DEMO_LOG("get on error");
        return -1;
    }

    int32_t on = cJSON_GetNumberValue(item);
    DEMO_LOG("switch on put %d=>%d", g_switch, on);
    g_switch = on == 0 ? false : true;

    cJSON_Delete(json);
    return 0;
}

static int32_t SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    if (data == NULL || *data != NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        DEMO_LOG("create obj error");
        return -1;
    }

    if (cJSON_AddNumberToObject(json, "on", g_switch) == NULL) {
        cJSON_Delete(json);
        DEMO_LOG("add num error");
        return -1;
    }

    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (*data == NULL) {
        DEMO_LOG("json print error");
        return -1;
    }
    DEMO_LOG("switch on get %d", g_switch);
    *len = strlen(*data);
    return 0;
}

const struct SvcMap {
    const IotcServiceInfo *svc;
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SVC_MAP[] = {
    {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState},
};

static int32_t PutCharState(const IotcCharState state[], uint32_t num)
{
    if (state == NULL || num == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = 0;
    for (uint32_t i = 0; i < num; ++i) {
        for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
            DEMO_LOG("put char sid:%s data:%s", state[i].svcId, state[i].data);
            if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].putCharState == NULL) {
                continue;
            }
            int32_t curRet = SVC_MAP[j].putCharState(SVC_MAP[j].svc, state[i].data, state[i].len);
            if (curRet != 0) {
                ret = curRet;
                DEMO_LOG("put char sid:%s error %d", state[i].svcId, ret);
            }
        }
    }
    return ret;
}

static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
    if (state == NULL || num == 0 || out == NULL || len == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = 0;
    for (uint32_t i = 0; i < num; ++i) {
        for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
            DEMO_LOG("get char sid:%s", state[i].svcId);
            if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].getCharState == NULL) {
                continue;
            }
            int32_t curRet = SVC_MAP[j].getCharState(SVC_MAP[j].svc, &out[i], &len[i]);
            if (curRet != 0) {
                ret = curRet;
                DEMO_LOG("get char sid:%s error %d", state[i].svcId, ret);
            }
        }
    }

    return ret;
}

static int32_t ReportAll(void)
{
    IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])] = {0};
    int32_t ret;
    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        reportInfo[i].svcId = SVC_MAP[i].svc->svcId;
        ret = SVC_MAP[i].getCharState(SVC_MAP[i].svc, (char **)&reportInfo[i].data, &reportInfo[i].len);
        if (ret != 0) {
            DEMO_LOG("get char sid:%s error %d", reportInfo[i].svcId, ret);
            break;
        }
    }
    if (ret == 0) {
        ret = IotcOhDevReportCharState(reportInfo, sizeof(reportInfo) / sizeof(reportInfo[0]));
    }

    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        if (reportInfo[i].data != NULL) {
            cJSON_free((char *)reportInfo[i].data);
            reportInfo[i].data = NULL;
        }
    }
    return ret;
}

static int32_t GetPincode(uint8_t *buf, uint32_t bufLen)
{
    if (buf == NULL || bufLen > IOTC_PINCODE_LEN) {
        DEMO_LOG("param invalid");
        return -1;
    }

#if IOTC_CONF_AILIFE_SUPPORT
    unsigned char pskBuf[16] = {0};
    extern void HiLinkGetPsk(unsigned char *psk, unsigned short len);
    HiLinkGetPsk(pskBuf, 16);

    int32_t ret = memcpy_s(buf, bufLen, pskBuf, 8);
#else
    int32_t ret = memcpy_s(buf, bufLen, PIN_CODE, strlen(PIN_CODE));
#endif
    if (ret != EOK) {
        return -1;
    }
    return 0;
}

static int32_t GetAcKey(uint8_t *buf, uint32_t bufLen)
{
    if (buf == NULL || bufLen > IOTC_AC_KEY_LEN) {
        DEMO_LOG("param invalid");
        return -1;
    }
    int32_t ret = memcpy_s(buf, bufLen, AC_KEY, sizeof(AC_KEY));
    if (ret != EOK) {
        return -1;
    }
    return 0;
}

#if IOTC_CONF_AILIFE_SUPPORT
#define HILINK_HTTPS_CERT1 \
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIIFZDCCA0ygAwIBAgIIYsLLTehAXpYwDQYJKoZIhvcNAQELBQAwUDELMAkGA1UE\r\n" \
    "BhMCQ04xDzANBgNVBAoMBkh1YXdlaTETMBEGA1UECwwKSHVhd2VpIENCRzEbMBkG\r\n" \
    "A1UEAwwSSHVhd2VpIENCRyBSb290IENBMB4XDTE3MDgyMTEwNTYyN1oXDTQyMDgx\r\n" \
    "NTEwNTYyN1owUDELMAkGA1UEBhMCQ04xDzANBgNVBAoMBkh1YXdlaTETMBEGA1UE\r\n" \
    "CwwKSHVhd2VpIENCRzEbMBkGA1UEAwwSSHVhd2VpIENCRyBSb290IENBMIICIjAN\r\n" \
    "BgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA1OyKm3Ig/6eibB7Uz2o93UqGk2M7\r\n" \
    "84WdfF8mvffvu218d61G5M3Px54E3kefUTk5Ky1ywHvw7Rp9KDuYv7ktaHkk+yr5\r\n" \
    "9Ihseu3a7iM/C6SnMSGt+LfB/Bcob9Abw95EigXQ4yQddX9hbNrin3AwZw8wMjEI\r\n" \
    "SYYDo5GuYDL0NbAiYg2Y5GpfYIqRzoi6GqDz+evLrsl20kJeCEPgJZN4Jg00Iq9k\r\n" \
    "++EKOZ5Jc/Zx22ZUgKpdwKABkvzshEgG6WWUPB+gosOiLv++inu/9blDpEzQZhjZ\r\n" \
    "9WVHpURHDK1YlCvubVAMhDpnbqNHZ0AxlPletdoyugrH/OLKl5inhMXNj3Re7Hl8\r\n" \
    "WsBWLUKp6sXFf0dvSFzqnr2jkhicS+K2IYZnjghC9cOBRO8fnkonh0EBt0evjUIK\r\n" \
    "r5ClbCKioBX8JU+d4ldtWOpp2FlxeFTLreDJ5ZBU4//bQpTwYMt7gwMK+MO5Wtok\r\n" \
    "Ux3UF98Z6GdUgbl6nBjBe82c7oIQXhHGHPnURQO7DDPgyVnNOnTPIkmiHJh/e3vk\r\n" \
    "VhiZNHFCCLTip6GoJVrLxwb9i4q+d0thw4doxVJ5NB9OfDMV64/ybJgpf7m3Ld2y\r\n" \
    "E0gsf1prrRlDFDXjlYyqqpf1l9Y0u3ctXo7UpXMgbyDEpUQhq3a7txZQO/17luTD\r\n" \
    "oA6Tz1ADavvBwHkCAwEAAaNCMEAwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQF\r\n" \
    "MAMBAf8wHQYDVR0OBBYEFKrE03lH6G4ja+/wqWwicz16GWmhMA0GCSqGSIb3DQEB\r\n" \
    "CwUAA4ICAQC1d3TMB+VHZdGrWJbfaBShFNiCTN/MceSHOpzBn6JumQP4N7mxCOwd\r\n" \
    "RSsGKQxV2NPH7LTXWNhUvUw5Sek96FWx/+Oa7jsj3WNAVtmS3zKpCQ5iGb08WIRO\r\n" \
    "cFnx3oUQ5rcO8r/lUk7Q2cN0E+rF4xsdQrH9k2cd3kAXZXBjfxfKPJTdPy1XnZR/\r\n" \
    "h8H5EwEK5DWjSzK1wKd3G/Fxdm3E23pcr4FZgdYdOlFSiqW2TJ3Qe6lF4GOKOOyd\r\n" \
    "WHkpu54ieTsqoYcuMKnKMjT2SLNNgv9Gu5ipaG8Olz6g9C7Htp943lmK/1Vtnhgg\r\n" \
    "pL3rDTsFX/+ehk7OtxuNzRMD9lXUtEfok7f8XB0dcL4ZjnEhDmp5QZqC1kMubHQt\r\n" \
    "QnTauEiv0YkSGOwJAUZpK1PIff5GgxXYfaHfBC6Op4q02ppl5Q3URl7XIjYLjvs9\r\n" \
    "t4S9xPe8tb6416V2fe1dZ62vOXMMKHkZjVihh+IceYpJYHuyfKoYJyahLOQXZykG\r\n" \
    "K5iPAEEtq3HPfMVF43RKHOwfhrAH5KwelUA/0EkcR4Gzth1MKEqojdnYNemkkSy7\r\n" \
    "aNPPT4LEm5R7sV6vG1CjwbgvQrWCgc4nMb8ngdfnVF7Ydqjqi9SAqUzIk4+Uf0ZY\r\n" \
    "+6RY5IcHdCaiPaWIE1xURQ8B0DRUURsQwXdjZhgLN/DKJpCl5aCCxg==\r\n" \
    "-----END CERTIFICATE-----\r\n"

#define HILINK_HTTPS_CERT14 \
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\r\n" \
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\r\n" \
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\r\n" \
    "MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\r\n" \
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\r\n" \
    "hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\r\n" \
    "RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\r\n" \
    "gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\r\n" \
    "KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\r\n" \
    "QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\r\n" \
    "XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\r\n" \
    "DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\r\n" \
    "LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\r\n" \
    "RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\r\n" \
    "jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\r\n" \
    "6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\r\n" \
    "mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\r\n" \
    "Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\r\n" \
    "WD9f\r\n" \
    "-----END CERTIFICATE-----\r\n"

#define  HILINK_HTTPS_CERT28 \
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIIFjTCCA3WgAwIBAgIEGErM1jANBgkqhkiG9w0BAQsFADBWMQswCQYDVQQGEwJD\r\n" \
    "TjEwMC4GA1UECgwnQ2hpbmEgRmluYW5jaWFsIENlcnRpZmljYXRpb24gQXV0aG9y\r\n" \
    "aXR5MRUwEwYDVQQDDAxDRkNBIEVWIFJPT1QwHhcNMTIwODA4MDMwNzAxWhcNMjkx\r\n" \
    "MjMxMDMwNzAxWjBWMQswCQYDVQQGEwJDTjEwMC4GA1UECgwnQ2hpbmEgRmluYW5j\r\n" \
    "aWFsIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MRUwEwYDVQQDDAxDRkNBIEVWIFJP\r\n" \
    "T1QwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDXXWvNED8fBVnVBU03\r\n" \
    "sQ7smCuOFR36k0sXgiFxEFLXUWRwFsJVaU2OFW2fvwwbwuCjZ9YMrM8irq93VCpL\r\n" \
    "TIpTUnrD7i7es3ElweldPe6hL6P3KjzJIx1qqx2hp/Hz7KDVRM8Vz3IvHWOX6Jn5\r\n" \
    "/ZOkVIBMUtRSqy5J35DNuF++P96hyk0g1CXohClTt7GIH//62pCfCqktQT+x8Rgp\r\n" \
    "7hZZLDRJGqgG16iI0gNyejLi6mhNbiyWZXvKWfry4t3uMCz7zEasxGPrb382KzRz\r\n" \
    "EpR/38wmnvFyXVBlWY9ps4deMm/DGIq1lY+wejfeWkU7xzbh72fROdOXW3NiGUgt\r\n" \
    "hxwG+3SYIElz8AXSG7Ggo7cbcNOIabla1jj0Ytwli3i/+Oh+uFzJlU9fpy25IGvP\r\n" \
    "a931DfSCt/SyZi4QKPaXWnuWFo8BGS1sbn85WAZkgwGDg8NNkt0yxoekN+kWzqot\r\n" \
    "aK8KgWU6cMGbrU1tVMoqLUuFG7OA5nBFDWteNfB/O7ic5ARwiRIlk9oKmSJgamNg\r\n" \
    "TnYGmE69g60dWIolhdLHZR4tjsbftsbhf4oEIRUpdPA+nJCdDC7xij5aqgwJHsfV\r\n" \
    "PKPtl8MeNPo4+QgO48BdK4PRVmrJtqhUUy54Mmc9gn900PvhtgVguXDbjgv5E1hv\r\n" \
    "cWAQUhC5wUEJ73IfZzF4/5YFjQIDAQABo2MwYTAfBgNVHSMEGDAWgBTj/i39KNAL\r\n" \
    "tbq2osS/BqoFjJP7LzAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjAd\r\n" \
    "BgNVHQ4EFgQU4/4t/SjQC7W6tqLEvwaqBYyT+y8wDQYJKoZIhvcNAQELBQADggIB\r\n" \
    "ACXGumvrh8vegjmWPfBEp2uEcwPenStPuiB/vHiyz5ewG5zz13ku9Ui20vsXiObT\r\n" \
    "ej/tUxPQ4i9qecsAIyjmHjdXNYmEwnZPNDatZ8POQQaIxffu2Bq41gt/UP+TqhdL\r\n" \
    "jOztUmCypAbqTuv0axn96/Ua4CUqmtzHQTb3yHQFhDmVOdYLO6Qn+gjYXB74BGBS\r\n" \
    "ESgoA//vU2YApUo0FmZ8/Qmkrp5nGm9BC2sGE5uPhnEFtC+NiWYzKXZUmhH4J/qy\r\n" \
    "P5Hgzg0b8zAarb8iXRvTvyUFTeGSGn+ZnzxEk8rUQElsgIfXBDrDMlI1Dlb4pd19\r\n" \
    "xIsNER9Tyx6yF7Zod1rg1MvIB671Oi6ON7fQAUtDKXeMOZePglr4UeWJoBjnaH9d\r\n" \
    "Ci77o0cOPaYjesYBx4/IXr9tgFa+iiS6M+qf4TIRnvHST4D2G0CvOJ4RUHlzEhLN\r\n" \
    "5mydLIhyPDCBBpEi6lmt2hkuIsKNuYyH4Ga8cyNfIWRjgEj1oDwYPZTISEEdQLpe\r\n" \
    "/v5WOaHIz16eGWRGENoXkbcFgKyLmZJ956LYBws2J+dIeWCKw9cTXPhyQN9Ky8+Z\r\n" \
    "AAoACxGV2lZFA4gKn2fQ1XmxqI1AbQ3CekD6819kR5LLU7m7Wc5P/dAVUwHY3+vZ\r\n" \
    "5nbv0CO7O6l5s9UCKc2Jo5YPSjXnTkLAdc0Hz+Ys63su\r\n" \
    "-----END CERTIFICATE-----\r\n"
#endif

static int32_t GetRootCA(const char **ca[], uint32_t *num)
{
    if (ca == NULL || num == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }
    /* 填充证书 */
#if IOTC_CONF_AILIFE_SUPPORT
    static const char *CA_CERT[] = {HILINK_HTTPS_CERT1, HILINK_HTTPS_CERT14, HILINK_HTTPS_CERT28};
#else
    static const char *CA_CERT[] = {NULL};
#endif

    *ca = CA_CERT;
    *num = sizeof(CA_CERT) / sizeof(CA_CERT[0]);
    return 0;
}

static int32_t NoticeReboot(IotcRebootReason res)
{
    DEMO_LOG("notice reboot res %d", res);
    return 0;
}

static int32_t IotcOhDemoEntry(void)
{
    static const IotcOhProfCallback PROF_CALLBACK = {
        .onPutCharState = PutCharState,
        .onGetCharState = GetCharState,
        .onReportAll = ReportAll,
        .onGetPincode = GetPincode,
        .onGetAcKey = GetAcKey,
        .onProfFree = cJSON_free,
    };
    static const IotcOhDevCallback DEVICE_CALLBACK = {
        .onReboot = NoticeReboot,
        .onTrng = NULL,
    };
    static const IotcOhDevInitParam DEVICE_INIT_PARAM = {
        .devInfo = &DEV_INFO,
        .svcInfo = SVC_INFO,
        .svcNum = sizeof(SVC_INFO) / sizeof(SVC_INFO[0]),
        .profCb = &PROF_CALLBACK,
        .devCb = &DEVICE_CALLBACK,
    };
    int32_t ret = IotcOhDevInit(&DEVICE_INIT_PARAM);
    if (ret != 0) {
        DEMO_LOG("init device error %d", ret);
        return ret;
    }

    static const IotcOhNetCfgParam NETCFG_PARAM = {
        .mode = IOTC_NET_CONFIG_MODE_BLE_SUP,
        .timeout = (10 * 60 * 1000),
    };

    static const IotcOhWifiExCallback WIFI_EX_CALLBACK = {
        .onGetCaCert = GetRootCA,
    };

    static const IotcOhWifiInitParam WIFI_INIT_PARAM = {
        .netcfg = &NETCFG_PARAM,
        .buffer = NULL,
        .exCallback = &WIFI_EX_CALLBACK,
    };

    ret = IotcOhWifiEnable(&WIFI_INIT_PARAM);
    if (ret != 0) {
        DEMO_LOG("enable wifi connect error %d", ret);
        return ret;
    }

    ret = IotcOhBleEnable(NULL);
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
        return ret;
    }

    ret = IotcOhMain();
    if (ret != 0) {
        DEMO_LOG("iotc oh main error %d", ret);
        return ret;
    }
    DEMO_LOG("iotc oh main success");
    return ret;
}

static void IotcOhDemoExit(void)
{
    int32_t ret = IotcOhStop();
    if (ret != 0) {
        DEMO_LOG("iotc stop error %d", ret);
    }

    ret = IotcOhWifiDisable();
    if (ret != 0) {
        DEMO_LOG("iotc wifi disable error %d", ret);
    }

    ret = IotcOhBleDisable();
    if (ret != 0) {
        DEMO_LOG("iotc ble disable error %d", ret);
    }

    ret = IotcOhDevDeinit();
    if (ret != 0) {
        DEMO_LOG("iotc dev info deinit error %d", ret);
    }
}

static void PressButton1(void)
{
    DEMO_LOG("button 1 press");
    IotcOhRestore();
}

void PressButton2(void)
{
    DEMO_LOG("button 2 press");
    IotcOhDemoExit();
}