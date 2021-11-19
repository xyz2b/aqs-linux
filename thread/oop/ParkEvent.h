//
// Created by xyzjiao on 11/18/21.
//

#ifndef AQS_PARKEVENT_H
#define AQS_PARKEVENT_H

#include "../../include/common.h"
#include "JavaThread.h"

// 对阻塞/唤醒线程的逻辑的封装
class ParkEvent {
public:
    JavaThread* _owner;
    pthread_mutex_t _lock[1];
    pthread_cond_t _cond[1];

public:
    ParkEvent() {
        int status;

        status = pthread_mutex_init(_lock, NULL);
        assert(status == 0, "mutex init fail");

        status = pthread_cond_init(_cond, NULL);
        assert(status == 0, "cond init fail");

    }

    ParkEvent(JavaThread* owner) {
        int status;

        _owner = owner;

        status = pthread_mutex_init(_lock, NULL);
        assert(status == 0, "mutex init fail");

        status = pthread_cond_init(_cond, NULL);
        assert(status == 0, "cond init fail");

    }

    // 在构造函数中加锁，在析构函数中解锁
    // 用的时候，只需要在代码块中创建锁对象即可（OS栈上创建），当退出代码块时，会自动调用代码块中所创建对象的析构函数
    /**
     * {
     *      ParkEvent parkEvent;
     * }
     * */
    ~ParkEvent() {

    }

    void park(ThreadState thread_status);
    void unpark();
};


#endif //AQS_PARKEVENT_H
