//
// Created by xyzjiao on 11/9/21.
//

#include "JavaThread.h"
#include "ObjectMonitor.h"
#include "Atomic.h"
#include "ObjectWaiter.h"

extern int val;
extern ObjectMonitor objectMonitor;

void* thread_do(void* arg) {
    JavaThread* Self = (JavaThread*) arg;

    pthread_mutex_lock(Self->_startThread_lock);
    Self->_state = INITIALIZED;
    INFO_PRINT("[%s] 线程阻塞，等待唤醒", Self->_name.c_str());
    pthread_cond_wait(Self->_cond, Self->_startThread_lock);
    pthread_mutex_unlock(Self->_startThread_lock);

    // 开始执行业务逻辑，包括加锁的逻辑
    Self->_state = RUNNABLE;

    objectMonitor.enter(Self);

    for (int j = 0; j < 10000; j++) {
        val++;
    }

    // 业务逻辑执行完成，不包括解锁的逻辑
    Self->_state = FINISHED;
    objectMonitor.exit(Self);

    // 业务逻辑执行完成，包括解锁的逻辑
    Self->_state = ZOMBIE;

    return 0;
}

JavaThread::JavaThread(string name) {
    _name = name;

    pthread_mutex_init(_startThread_lock, NULL);
    pthread_cond_init(_cond, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    _state = ALLOCATED;

    // 自旋，将所创建的线程到objectMonitor
    for (;;) {
        if ((bool)(Atomic::cmpxchg_ptr(reinterpret_cast<void *>(true), (void *) &objectMonitor._waiterSetLock,
                                       reinterpret_cast<void *>(false))) == false) {
            objectMonitor._waiters++;
            INFO_PRINT("[%s] 加入waiter set", this->_name.c_str());
            ObjectWaiter* node = new ObjectWaiter(this);
            if (objectMonitor._waiterSet == NULL) {
                objectMonitor._waiterSet = node;
            } else {
                node->_next = objectMonitor._waiterSet;
                objectMonitor._waiterSet->_prev = node;
                objectMonitor._waiterSet = node;
            }
            objectMonitor._waiterSetLock = false;
            break;
        }
        sleep(1);
    }

    pthread_create(_tid, &attr, thread_do, this);

    pthread_attr_destroy(&attr);
}

// 主线程run方法中的唤醒逻辑 可能会比 业务线程thread_do方法中的阻塞逻辑先运行，不确定，所以会造成先唤醒再阻塞的情况
// 解决办法：使用业务线程状态来判断
void JavaThread::run() {
    while (true) {
        if (_state == INITIALIZED) {
            INFO_PRINT("唤醒 [%s]", this->_name.c_str());
            pthread_mutex_lock(_startThread_lock);
            pthread_cond_signal(_cond);
            pthread_mutex_unlock(_startThread_lock);
            break;
        }

        sleep(1);
    }

}

void JavaThread::join() {
    while (true) {
        if (_state == ZOMBIE) {
            break;
        } else {
        }

        sleep(1);
    }
}


JavaThread::JavaThread(int thread_num) {
    stringstream ss;
    ss << "t";
    ss << thread_num;
    _name = ss.str();

    pthread_mutex_init(_startThread_lock, NULL);
    pthread_cond_init(_cond, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    _state = ALLOCATED;

    pthread_create(_tid, &attr, thread_do, this);

    pthread_attr_destroy(&attr);
}