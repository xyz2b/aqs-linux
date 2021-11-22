//
// Created by xyzjiao on 11/9/21.
//

#include "JavaThread.h"
#include "ObjectMonitor.h"
#include "Atomic.h"
#include "ObjectWaiter.h"
#include "ParkEvent.h"

extern int val;
extern ObjectMonitor objectMonitor;

void* thread_do(void* arg) {
    JavaThread* Self = (JavaThread*) arg;

    pthread_mutex_lock(Self->_startThread_lock);
    Self->_state = INITIALIZED;
    INFO_PRINT("[%s] 线程阻塞，等待唤醒", Self->_name.c_str());
    pthread_cond_wait(Self->_cond, Self->_startThread_lock);
    pthread_mutex_unlock(Self->_startThread_lock);

    // 执行业务逻辑
    Self->_entry_point(Self);

    // 执行完成，包括解锁的逻辑
    Self->_state = ZOMBIE;

    return 0;
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

JavaThread::JavaThread(thread_fun entry_point, void *args, string name) {
    _entry_point = entry_point;
    _args = args;
    _name = name;

    pthread_mutex_init(_startThread_lock, NULL);
    pthread_cond_init(_cond, NULL);

//    pthread_mutex_init(_sync_lock, NULL);
//    pthread_cond_init(_sync_cond, NULL);
    _sync_park_event = new ParkEvent(this);
    _light_to_monitor_park_event = new ParkEvent(this);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    _state = ALLOCATED;

    pthread_create(_tid, &attr, thread_do, this);

    pthread_attr_destroy(&attr);
}
