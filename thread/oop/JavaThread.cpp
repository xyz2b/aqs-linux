//
// Created by xyzjiao on 11/9/21.
//

#include "JavaThread.h"
#include "ObjectMonitor.h"

extern int val;
extern ObjectMonitor objectMonitor;

void* thread_do(void* arg) {
    JavaThread* Self = (JavaThread*) arg;

    pthread_mutex_lock(Self->_startThread_lock);
    Self->_state = INITIALIZED;
    pthread_cond_wait(Self->_cond, Self->_startThread_lock);
    pthread_mutex_unlock(Self->_startThread_lock);

    objectMonitor.enter(Self);

    Self->_state = RUNNABLE;

    for (int j = 0; j < 10000; j++) {
        val++;
    }

    objectMonitor.exit(Self);

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

    pthread_create(_tid, &attr, thread_do, this);

    pthread_attr_destroy(&attr);
}

// 主线程run方法中的唤醒逻辑 可能会比 业务线程thread_do方法中的阻塞逻辑先运行，不确定，所以会造成先唤醒再阻塞的情况
// 解决办法：使用业务线程状态来判断
void JavaThread::run() {
    while (true) {
        if (_state == INITIALIZED) {
            pthread_cond_signal(_cond);
            break;
        }

        sleep(1);
    }

}

void JavaThread::join() {
    while (true) {
        if (_state == ZOMBIE) {
            INFO_PRINT("[%s] 执行完毕", _name.c_str());
            break;
        } else {
            INFO_PRINT("[%s]:[%X] 线程状态: %d", _name.c_str(), pthread_self(), _state);
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