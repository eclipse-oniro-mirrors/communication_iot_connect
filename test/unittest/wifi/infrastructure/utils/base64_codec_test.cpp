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
#include "securec.h"

#include "base64_codec.h"
#include "iotc_errcode.h"

using namespace testing::ext;

class Base64CodecTest : public testing::Test {
public:
    Base64CodecTest()
    {}
    ~Base64CodecTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void Base64CodecTest::SetUpTestCase(void)
{
}

void Base64CodecTest::TearDownTestCase(void)
{
}

HWTEST_F(Base64CodecTest, GetBase64CodecDataTest001, TestSize.Level1)
{
    uint32_t dataLen = 0;
    uint8_t *result = GetBase64CodecData(NULL, 0, &dataLen, BASE64_CODEC_TYPE_ENCODE, 1024);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(Base64CodecTest, GetBase64CodecDataTest002, TestSize.Level1)
{
    uint8_t input[] = {'t', 'e', 's', 't'};
    uint8_t *result = GetBase64CodecData(input, sizeof(input), NULL, BASE64_CODEC_TYPE_ENCODE, 1024);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(Base64CodecTest, GetBase64CodecDataTest003, TestSize.Level1)
{
    uint8_t input[] = {'t', 'e', 's', 't'};
    uint32_t dataLen = 0;
    uint8_t *result = GetBase64CodecData(input, 0, &dataLen, BASE64_CODEC_TYPE_ENCODE, 1024);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(Base64CodecTest, GetBase64CodecDataTest004, TestSize.Level1)
{
    uint8_t input[] = {'t', 'e', 's', 't'};
    uint32_t dataLen = 0;
    uint8_t *result = GetBase64CodecData(input, sizeof(input), &dataLen, BASE64_CODEC_TYPE_ENCODE, 1);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(Base64CodecTest, GetBase64CodecDataTest005, TestSize.Level1)
{
    uint8_t input[] = {'t', 'e', 's', 't'};
    uint32_t dataLen = 0;
    uint8_t *result = GetBase64CodecData(input, sizeof(input), &dataLen, BASE64_CODEC_TYPE_ENCODE, 1024);
    EXPECT_NE(result, nullptr);
    EXPECT_NE(dataLen, 0U);
    free(result);
}

HWTEST_F(Base64CodecTest, GetBase64CodecDataTest006, TestSize.Level1)
{
    static uint8_t input[] = {'d', 'G', 'V', 'z', 'd', 'A', '=' , '='};
    uint32_t dataLen = 0;
    uint8_t *result = GetBase64CodecData(input, sizeof(input), &dataLen, BASE64_CODEC_TYPE_DECODE, 1024);
    EXPECT_NE(result, nullptr);
    EXPECT_NE(dataLen, 0U);
    free(result);
}