//
// Created by ziya on 2021/11/20.
//

#ifndef AQS_BASICLOCK_H
#define AQS_BASICLOCK_H

#include "../include/common.h"

class MarkOopDesc;
class JavaThread;

// 轻量级锁
class BasicLock {
private:
    // 存储膨胀成轻量级锁之前对象的markOop
    markOop    _displaced_header;
    JavaThread*     _owner;

public:
    markOop displaced_header() const {
        return _displaced_header;
    }

    void set_displaced_header(markOop header)   {
        _displaced_header = header;
    }

    JavaThread* owner() {
        return _owner;
    }

    void set_owner(JavaThread* owner) {
        _owner = owner;
    }
};


#endif //AQS_BASICLOCK_H
