//
// Created by xyzjiao on 11/19/21.
//

#include "InstanceOopDesc.h"

#include "MarkOopDesc.h"

InstanceOopDesc::InstanceOopDesc() {
    // 创建InstanceOop对象时，默认初始化为无锁状态，即对象头为1
    _mark = MarkOopDesc::prototype();
}