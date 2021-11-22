//
// Created by ziya on 2021/11/12.
//

#ifndef LINUX_OBJECTMONITOR_H
#define LINUX_OBJECTMONITOR_H

#include "../include/common.h"

// 在.h文件中使用class定义，在.c文件中再include，避免循环引用的问题
class JavaThread;
class ObjectWaiter;

class ObjectMonitor {
private:
    // 存储膨胀成重量级锁之前对象头的markOop
    volatile markOop   _header;
    void* volatile _object;
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
    volatile long _entryListLock;
public:
    ObjectMonitor() {
        _recursions = 0;
        _owner = NULL;
        _tail = _head = NULL;
        _entryListLength = 0;
        _entryList = NULL;
        _entryListLock = false;
    }

    markOop header() const {
        return _header;
    }

    inline void set_header(markOop hdr) {
        _header = hdr;
    }

    inline void* object() const {
        return _object;
    }

    inline void* object_addr() {
        return (void *)(&_object);
    }

    inline void set_object(void* obj) {
        _object = obj;
    }

    void enter(JavaThread* thread);
    void exit(JavaThread* thread);

    void print(string name);
};


#endif //LINUX_OBJECTMONITOR_H
