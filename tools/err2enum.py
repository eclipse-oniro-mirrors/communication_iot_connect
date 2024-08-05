# Copyright (c) 2024-2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import os
import sys


def find_errcode_by_module(module):
    f = open(os.path.dirname(os.path.abspath(__file__)) + '/../interfaces/kits/common/iotc_def.h', 'r', encoding='utf8')

    begin_line = -1
    end_line = -1
    lines = f.readlines()
    for i in range(len(lines)):
        if lines[i].find("typedef enum") != -1:
            begin_line = i
        elif lines[i].find("IotcSubModule") != -1:
            end_line = i
            break
    if begin_line == -1 or end_line == -1:
        f.close()
        return 1

    module_index = -1
    for i in range(begin_line + 1, end_line):
        if lines[i].find("=") == -1:
            module_index += 1
        else:
            module_index = int(lines[i].split("=")[1].split(",")[0])

        if lines[i].find(module) != -1:
            f.close()
            return - module_index * (2 ** 16)
    
    f.close()
    return 1



def find_enum(errcode):
    f = open(os.path.dirname(os.path.abspath(__file__)) + '/../interfaces/kits/common/iotc_errcode.h', 'r', encoding='utf8')
    line = f.readline()
    while line and line.find("enum IotcErrcode") == -1:
        line = f.readline()
    if line.find("enum IotcErrcode") == -1:
        f.close()
        return "find enum def err"

    cur_errcode = 0
    while line.find("}") == -1:
        line = f.readline()

        if line.find("=") == -1:
            if line.find(",") != -1:
                cur_errcode += 1
        elif line.find("ERR_CODE_BASE") != -1:
            module = line.split("ERR_CODE_BASE(")[1].split(")")[0]
            cur_errcode = find_errcode_by_module(module)
        else:
            cur_errcode = int(line.split("=")[1].split(",")[0])

        if cur_errcode == int(errcode):
            f.close()
            return line.split("=")[0].split(",")[0].replace(" ", "")
        elif cur_errcode > 0:
            break

    f.close()
    return "errcode not found"


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("param invalid")
    else:
        print(find_enum(sys.argv[1]))