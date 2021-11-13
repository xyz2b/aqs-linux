//
// Created by ziya on 2021/11/12.
//

#ifndef LINUX_OBJECTMONITOR_H
#define LINUX_OBJECTMONITOR_H

#include "../../include/common.h"

class JavaThread;
class ObjectWaiter;

class ObjectMonitor {
public:
    // 当前持有锁的线程，即锁标志
    volatile JavaThread* _owner;
    // 重入次数
    volatile int         _recursions;

    // 队列
    volatile ObjectWaiter*       _tail;
    volatile ObjectWaiter*        _head;

    // number of waiting threads
    volatile int _waiters;
    //
    ObjectWaiter* volatile _waiterSet;
    // protects waiterSet，简单的自旋锁
    volatile bool _waiterSetLock;
public:
    ObjectMonitor() {
        _recursions = 0;
        _owner = NULL;
        _tail = _head = NULL;
        _waiters = 0;
        _waiterSet = NULL;
        _waiterSetLock = false;
    }
    void enter(JavaThread* thread);
    void exit(JavaThread* thread);
};


#endif //LINUX_OBJECTMONITOR_H
