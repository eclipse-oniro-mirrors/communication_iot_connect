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

#include "seq_num_utils.h"
#include "iotc_errcode.h"

using namespace testing::ext;

class SeqNumUtilsTest : public testing::Test {
public:
    SeqNumUtilsTest()
    {}
    ~SeqNumUtilsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void SeqNumUtilsTest::SetUpTestCase(void)
{
}

void SeqNumUtilsTest::TearDownTestCase(void)
{
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest001, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(100, 200, 0, &isSmall, &delta);
    EXPECT_EQ(ret, false);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest002, TestSize.Level1)
{
    bool ret = SeqNumCheck(100, 200, 10, NULL, NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest003, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(100, 100, 10, &isSmall, &delta);
    EXPECT_EQ(ret, false);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest004, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(100, 200, 10, &isSmall, &delta);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(isSmall, false);
    EXPECT_EQ(delta, 100);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest005, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(200, 100, 10, &isSmall, &delta);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(isSmall, true);
    EXPECT_EQ(delta, 100);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest006, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(5, 200, 10, &isSmall, &delta);
    EXPECT_EQ(ret, true);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest007, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(UINT32_MAX - 5, 100, 10, &isSmall, &delta);
    EXPECT_EQ(ret, true);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest008, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(100, 500, 10, &isSmall, &delta);
    EXPECT_EQ(ret, false);
}

HWTEST_F(SeqNumUtilsTest, SeqNumCheckTest009, TestSize.Level1)
{
    bool isSmall = false;
    uint32_t delta = 0;
    bool ret = SeqNumCheck(100, 200, SEQ_NUM_WINDOW_MAX + 1, &isSmall, &delta);
    EXPECT_EQ(ret, false);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest001, TestSize.Level1)
{
    uint32_t ret = CoapSeqToNum(NULL, 0);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest002, TestSize.Level1)
{
    uint8_t data = 0x12;
    uint32_t ret = CoapSeqToNum(&data, 0);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest003, TestSize.Level1)
{
    uint8_t data = 0x12;
    uint32_t ret = CoapSeqToNum(&data, sizeof(uint8_t));
    EXPECT_EQ(ret, 0x12);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest004, TestSize.Level1)
{
    uint8_t data[2] = { 0x12, 0x34 };
    uint32_t ret = CoapSeqToNum(data, sizeof(uint16_t));
    EXPECT_EQ(ret, 0x3412);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest005, TestSize.Level1)
{
    uint8_t data[4] = { 0x12, 0x34, 0x56, 0x78 };
    uint32_t ret = CoapSeqToNum(data, sizeof(uint32_t));
    EXPECT_EQ(ret, 0x78563412);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest006, TestSize.Level1)
{
    uint8_t data[3] = { 0x12, 0x34, 0x56 };
    uint32_t ret = CoapSeqToNum(data, 3);
    EXPECT_EQ(ret, 0x123456);
}

HWTEST_F(SeqNumUtilsTest, CoapSeqToNumTest007, TestSize.Level1)
{
    uint8_t data[5] = { 0x12, 0x34, 0x56, 0x78, 0x9A };
    uint32_t ret = CoapSeqToNum(data, 5);
    EXPECT_EQ(ret, 0);
}