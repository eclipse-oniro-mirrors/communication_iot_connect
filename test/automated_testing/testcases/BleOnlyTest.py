#!/usr/bin/env python3
# coding=utf-8

#
# Copyright (c) 2020-2022 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# -*- coding: utf-8 -*-
from aosp.aw.AOSP.Common import  click
from  devicetest.core.test_case import  TestCase,Step
import hypium import UiDriver, BY, Rect
from django.template.defaultfilters import center


class BleOnlyTest(TestCase):

    def __init__(self, configs):
        self.TAG = self.__class__.__name__
        TestCase.__init__(self, self.TAG, configs)
        self.driver = UiDriver(self.device1)
        self.driver_width,self.driver_height = self.driver.get_window_size()

    def setup(self):
        Step("预制条件1xxxx")

    def process(self):
        Step("步骤1、打开通用互联APP")
        self.driver.start_app("com.app.connect")
        self.driver.wait(2)

        Step("步骤2、点击设备")
        self.driver.find_component(BY.text("设备")).click()
        self.driver.wait(1)

        Step("步骤3、点到点本地控")
        hi_3863  = self.driver.find_component(BY.image("hi_3863.jpeg"))
        center = self.driver.find_component(BY.image("Center.jpeg"))
        # self.driver.capture_screen("Center.jpeg", True,area=Rect(980. 1127, 1429, 1580))
        self.driver.wait(3)

        if hi_3863:
            self.driver.drag(hi_3863,center,press_time=3, drag_time=1)
            assert self.driver.wait_for_component(BY.text("已连接，点击查看设备详情"),timeout=5)
            self.driver.touch(BY.text("已连接，点击查看设备详情"))
            kaiguan = self.driver.find_component(BY.text("Toggle").isAfter(BY.text("开关")))
            for i in range(604800):
                kaiguan.click()
                self.driver.wait(3)
        else:
            self.driver.log_info("未找到hi3863")

     def teardown(self):
         Step("收尾工作xxxx")
         self.driver.stop_app("com.app.connect")


