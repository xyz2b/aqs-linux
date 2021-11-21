//
// Created by ziya on 2021/9/21.
//

#include "ObjectSynchronizer.h"

#include "../include/common.h"
#include "../oops/InstanceOopDesc.h"
#include "../oops/MarkOopDesc.h"
#include "BasicLock.h"
#include "Atomic.h"
#include "JavaThread.h"
#include "ObjectMonitor.h"

void ObjectSynchronizer::fast_enter(InstanceOopDesc* obj, BasicLock* lock, JavaThread *t) {
    markOop mark = obj->mark();
    if (UseBiasedLocking) {

    }

    slow_enter(obj, lock, t);
}

void ObjectSynchronizer::fast_exit(InstanceOopDesc* obj, BasicLock* lock, JavaThread *t) {

}

/**
 * 进入这个函数的情况：
 *      1、偏向锁未开启
 *      2、延迟偏向期间执行加锁
 *      3、重入
 */
void ObjectSynchronizer::slow_enter(InstanceOopDesc *obj, BasicLock* lock, JavaThread *t) {

}

void ObjectSynchronizer::slow_exit(InstanceOopDesc *obj, BasicLock* lock, JavaThread *t) {
    fast_exit(obj, lock, t);
}

ObjectMonitor *ObjectSynchronizer::inflate(InstanceOopDesc *obj, JavaThread *t) {

}
