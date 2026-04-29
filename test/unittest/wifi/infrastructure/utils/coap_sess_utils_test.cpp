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

#include "coap_sess_utils.h"
#include "iotc_errcode.h"
#include "coap_codec_def.h"

using namespace testing::ext;

class CoapSessUtilsTest : public testing::Test {
public:
    CoapSessUtilsTest()
    {}
    ~CoapSessUtilsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void CoapSessUtilsTest::SetUpTestCase(void)
{
}

void CoapSessUtilsTest::TearDownTestCase(void)
{
}

HWTEST_F(CoapSessUtilsTest, CoapUriWhiteListMatchTest001, TestSize.Level1)
{
    bool ret = CoapUriWhiteListMatch(NULL, NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CoapSessUtilsTest, CoapUriWhiteListMatchTest002, TestSize.Level1)
{
    CoapPacket packet;
    (void)memset_s(&packet, sizeof(CoapPacket), 0, sizeof(CoapPacket));
    const char *whiteList[] = { "/test", NULL };
    bool ret = CoapUriWhiteListMatch(&packet, whiteList);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CoapSessUtilsTest, CoapUriWhiteListMatchTest003, TestSize.Level1)
{
    CoapPacket packet;
    (void)memset_s(&packet, sizeof(CoapPacket), 0, sizeof(CoapPacket));
    packet.header.code = 0;
    bool ret = CoapUriWhiteListMatch(&packet, NULL);
    EXPECT_EQ(ret, false);
}

HWTEST_F(CoapSessUtilsTest, CoapUriWhiteListMatchTest004, TestSize.Level1)
{
    CoapPacket packet;
    (void)memset_s(&packet, sizeof(CoapPacket), 0, sizeof(CoapPacket));
    packet.header.code = COAP_CODE_CLASS_SUCC_RESP;
    const char *whiteList[] = { "/test", NULL };
    bool ret = CoapUriWhiteListMatch(&packet, whiteList);
    EXPECT_EQ(ret, false);
}