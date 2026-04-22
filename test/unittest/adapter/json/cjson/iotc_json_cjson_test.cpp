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

#include <gtest/gtest.h>
#include "iotc_json.h"
#include "iotc_errcode.h"

using namespace testing::ext;

class IotcJsonCjsonTest : public testing::Test {
public:
    IotcJsonCjsonTest() {}
    ~IotcJsonCjsonTest() {}
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(IotcJsonCjsonTest, IotcDuplicateJsonTest001, TestSize.Level1)
{
    IotcJson *ret = IotcDuplicateJson(NULL, true);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonParseTest001, TestSize.Level1)
{
    IotcJson *ret = IotcJsonParse(NULL);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonParseWithLenTest001, TestSize.Level1)
{
    IotcJson *ret = IotcJsonParseWithLen(NULL, 10);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonParseWithLenTest002, TestSize.Level1)
{
    const char *str = "{\"key\":\"value\"}";
    IotcJson *ret = IotcJsonParseWithLen(str, 0);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonDeleteTest001, TestSize.Level1)
{
    IotcJsonDelete(NULL);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonPrintTest001, TestSize.Level1)
{
    char *ret = IotcJsonPrint(NULL);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonFreePrintTest001, TestSize.Level1)
{
    IotcJsonFreePrint(NULL);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonPrint2BufferTest001, TestSize.Level1)
{
    uint32_t len = 256;
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonPrint2Buffer(json, NULL, &len);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    (void)len;
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonPrint2BufferTest002, TestSize.Level1)
{
    char buffer[256] = {0};
    uint32_t len = 256;
    int32_t ret = IotcJsonPrint2Buffer(NULL, buffer, &len);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonPrint2BufferTest003, TestSize.Level1)
{
    char buffer[256] = {0};
    uint32_t len = 0;
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonPrint2Buffer(json, buffer, &len);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonPrint2BufferTest004, TestSize.Level1)
{
    char buffer[256] = {0};
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonPrint2Buffer(json, buffer, NULL);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    (void)buffer;
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonPrint2BufferTest005, TestSize.Level1)
{
    char buffer[10] = {0};
    uint32_t len = 10;
    IotcJson *json = IotcJsonCreate();
    IotcJsonAddStr2Obj(json, "key", "very_long_value_that_exceeds_buffer");
    int32_t ret = IotcJsonPrint2Buffer(json, buffer, &len);
    EXPECT_EQ(ret, IOTC_ADAPTER_JSON_ERR_PRINT);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetObjTest001, TestSize.Level1)
{
    IotcJson *ret = IotcJsonGetObj(NULL, "name");
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetObjTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    IotcJson *ret = IotcJsonGetObj(json, NULL);
    EXPECT_EQ(ret, nullptr);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetArrayItemTest001, TestSize.Level1)
{
    IotcJson *ret = IotcJsonGetArrayItem(NULL, 0);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddItem2ArrayTest001, TestSize.Level1)
{
    IotcJson *array = IotcJsonCreateArray();
    int32_t ret = IotcJsonAddItem2Array(NULL, array);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(array);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddItem2ArrayTest002, TestSize.Level1)
{
    IotcJson *item = IotcJsonCreateStr("test");
    int32_t ret = IotcJsonAddItem2Array(NULL, item);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(item);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddItem2ObjTest001, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonAddItem2Obj(NULL, "name", json);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddItem2ObjTest002, TestSize.Level1)
{
    IotcJson *item = IotcJsonCreateStr("test");
    int32_t ret = IotcJsonAddItem2Obj(NULL, "name", item);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(item);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddItem2ObjTest003, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    IotcJson *item = IotcJsonCreateStr("test");
    int32_t ret = IotcJsonAddItem2Obj(json, NULL, item);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddFloat2ObjTest001, TestSize.Level1)
{
    int32_t ret = IotcJsonAddFloat2Obj(NULL, "name", 1.5);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddFloat2ObjTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonAddFloat2Obj(json, NULL, 1.5);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddBool2ObjTest001, TestSize.Level1)
{
    int32_t ret = IotcJsonAddBool2Obj(NULL, "name", true);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddBool2ObjTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonAddBool2Obj(json, NULL, true);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddStr2ObjTest001, TestSize.Level1)
{
    int32_t ret = IotcJsonAddStr2Obj(NULL, "name", "value");
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddStr2ObjTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonAddStr2Obj(json, NULL, "value");
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonAddStr2ObjTest003, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonAddStr2Obj(json, "name", NULL);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetStrTest001, TestSize.Level1)
{
    const char *ret = IotcJsonGetStr(NULL);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetNumTest001, TestSize.Level1)
{
    int64_t val = 0;
    int32_t ret = IotcJsonGetNum(NULL, &val);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetNumTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    int32_t ret = IotcJsonGetNum(json, NULL);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetNumTest003, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    int64_t val = 0;
    int32_t ret = IotcJsonGetNum(json, &val);
    EXPECT_EQ(ret, IOTC_ADAPTER_JSON_ERR_TYPE);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetFloatTest001, TestSize.Level1)
{
    double val = 0.0;
    int32_t ret = IotcJsonGetFloat(NULL, &val);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetFloatTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    int32_t ret = IotcJsonGetFloat(json, NULL);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetFloatTest003, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    double val = 0.0;
    int32_t ret = IotcJsonGetFloat(json, &val);
    EXPECT_EQ(ret, IOTC_ADAPTER_JSON_ERR_TYPE);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetBoolTest001, TestSize.Level1)
{
    bool val = false;
    int32_t ret = IotcJsonGetBool(NULL, &val);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetBoolTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    int32_t ret = IotcJsonGetBool(json, NULL);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetBoolTest003, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    bool val = false;
    int32_t ret = IotcJsonGetBool(json, &val);
    EXPECT_EQ(ret, IOTC_ADAPTER_JSON_ERR_TYPE);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonIsStrTest001, TestSize.Level1)
{
    bool ret = IotcJsonIsStr(NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonIsNumTest001, TestSize.Level1)
{
    bool ret = IotcJsonIsNum(NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonIsFloatTest001, TestSize.Level1)
{
    bool ret = IotcJsonIsFloat(NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonIsBoolTest001, TestSize.Level1)
{
    bool ret = IotcJsonIsBool(NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonIsArrayTest001, TestSize.Level1)
{
    bool ret = IotcJsonIsArray(NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonHasObjTest001, TestSize.Level1)
{
    bool ret = IotcJsonHasObj(NULL, "name");
    EXPECT_EQ(ret, false);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonHasObjTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    bool ret = IotcJsonHasObj(json, NULL);
    EXPECT_EQ(ret, false);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetArraySizeTest001, TestSize.Level1)
{
    uint32_t size = 0;
    int32_t ret = IotcJsonGetArraySize(NULL, &size);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetArraySizeTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    int32_t ret = IotcJsonGetArraySize(json, NULL);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonGetArraySizeTest003, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreateStr("test");
    uint32_t size = 0;
    int32_t ret = IotcJsonGetArraySize(json, &size);
    EXPECT_EQ(ret, IOTC_ADAPTER_JSON_ERR_TYPE);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonDeleteItemTest001, TestSize.Level1)
{
    IotcJsonDeleteItem(NULL, "name");
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonDeleteItemTest002, TestSize.Level1)
{
    IotcJson *json = IotcJsonCreate();
    IotcJsonDeleteItem(json, NULL);
    IotcJsonDelete(json);
}

HWTEST_F(IotcJsonCjsonTest, IotcJsonArrayDeleteItemTest001, TestSize.Level1)
{
    IotcJsonArrayDeleteItem(NULL, 0);
}