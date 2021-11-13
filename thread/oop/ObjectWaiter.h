//
// Created by ziya on 2021/11/12.
//

#ifndef LINUX_OBJECTWAITER_H
#define LINUX_OBJECTWAITER_H

#include "../../include/common.h"

class JavaThread;

class ObjectWaiter {
public:
    volatile ObjectWaiter* _prev;
    volatile ObjectWaiter* _next;
    volatile JavaThread* _thread;
public:
    ObjectWaiter(JavaThread* thread) {
        _thread = thread;
        _prev = NULL;
        _next = NULL;
    }
};


#endif //LINUX_OBJECTWAITER_H
