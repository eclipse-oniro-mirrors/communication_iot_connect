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

#include "wifi_net_info.h"
#include "iotc_errcode.h"

using namespace testing::ext;

class WifiNetInfoTest : public testing::Test {
public:
    WifiNetInfoTest()
    {}
    ~WifiNetInfoTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void WifiNetInfoTest::SetUpTestCase(void)
{
}

void WifiNetInfoTest::TearDownTestCase(void)
{
}

HWTEST_F(WifiNetInfoTest, IsNetworkConnectedTest001, TestSize.Level1)
{
    bool ret = IsNetworkConnected();
    EXPECT_EQ(ret, false);
}

HWTEST_F(WifiNetInfoTest, IsWifiNetInfoExistTest001, TestSize.Level1)
{
    bool ret = IsWifiNetInfoExist();
    EXPECT_EQ(ret, false);
}

HWTEST_F(WifiNetInfoTest, GetWifiMacAddrStrTest001, TestSize.Level1)
{
    int32_t ret = GetWifiMacAddrStr(NULL, 0);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(WifiNetInfoTest, GetWifiMacAddrStrTest002, TestSize.Level1)
{
    char buf[10];
    int32_t ret = GetWifiMacAddrStr(buf, 10);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}

HWTEST_F(WifiNetInfoTest, GetWifiMacAddrStrTest003, TestSize.Level1)
{
    char buf[MAC_ADDR_STR_LEN + 1];
    (void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
    int32_t ret = GetWifiMacAddrStr(buf, MAC_ADDR_STR_LEN);
    EXPECT_EQ(ret, IOTC_ERR_PARAM_INVALID);
}