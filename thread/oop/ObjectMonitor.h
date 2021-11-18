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

    // number of all threads
    volatile int _entryListLength;
    // 保存所有线程的列表，主要需要看所有线程的状态，这个字段的名字起得不太对，后续需要改，_waiterSet令有其用
    // _waiterSet是重量级锁中的一个队列，调用wait方法阻塞的线程队列
    ObjectWaiter* volatile _entryList;
    // protects entryList，简单的自旋锁
    volatile bool _entryListLock;
public:
    ObjectMonitor() {
        _recursions = 0;
        _owner = NULL;
        _tail = _head = NULL;
        _entryListLength = 0;
        _entryList = NULL;
        _entryListLock = false;
    }
    void enter(JavaThread* thread);
    void exit(JavaThread* thread);

    void print(string name);
};


#endif //LINUX_OBJECTMONITOR_H
