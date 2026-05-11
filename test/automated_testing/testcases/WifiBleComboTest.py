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
from pydoc import text

from  devicetest.core.test_case import  TestCase,Step
import hypium import UiDriver, BY, Rect
import time

class WifiBleComboTest(TestCase):

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

        Step("步骤3、点到右上角+号")
        self.driver.find_component(BY.xpath \
        ("/root/ContainerModal/Column/Stack/Column/Navigation/NavigationContent/NavDestination \
        /NavDestinationContent/Column/Tabs/Swiper/TabContent/Column/Fles/Image")).click()
        self.driver.wait(3)

        Step("步骤4、点击添加")
        self.driver.find_component(BY.text("添加").type("button")).click()
        self.driver.wait(1)

        Step("步骤5、点击输入wifi名")
        self.driver.find_component(BY.xpath \
        ("/root/ContainerModal/Column/Stack/Column/Navigation/NavigationContent/NavDestination \
        /NavDestinationContent/Column/Column[1]/Row[0]/TextInput"))
        self.driver.input_text(BY.type("TextInput"), text:"Mate")

        Step("步骤6、点击输入密码")
        # 根据条件点击控件  点击密码框
        self.driver.touch(BY.isAfter(BY.type('Divider')).isBefore(BY.type('Blank')).type('TextInput'))
        self.driver.wait(0.5)
        # 输入密码
        self.driver.input_password(BY.type('Divider')).isBefore(BY.type('Blank')).type('TextInput'), password:"12345678")
        self.driver.wait(2)
        # 根据条件点击控件  点击键盘下一步
        self.driver.go_back()
        self.driver.wait(0.5)

        Step("步骤7、点击下一步")
        self.driver.find_component(BY.text("下一步")).click()
        self.driver.wait(60)

        Step("步骤7、点击开关")
        total_duration = 86400
        start_time = time.time()
        while True:
            CURRENT_TIME = time.time()
            if CURRENT_TIME - start_time > total_duration:
                break
            self.driver.touch(BY.type("Toggle"))
            time.sleep(10)


     def teardown(self):
         Step("收尾工作 退出应用")
         self.driver.quit("com.app.connect")