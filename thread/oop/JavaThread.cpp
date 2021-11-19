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

    // 进入临界区
    objectMonitor.enter(Self);

    objectMonitor.enter(Self);
    INFO_PRINT("测试重入")
    objectMonitor.exit(Self);

    // 开始执行业务逻辑，不包括加锁的逻辑
    // 进入enter时，状态必须是小于等于INITIALIZED，所以这里是在enter执行完毕（即抢到锁之后）再改变状态为RUNNABLE
    // 如果在进入enter之前状态大于INITIALIZED，会导致释放锁的线程判断最后一个未抢到锁未被执行的线程（包括：被阻塞的线程、唤醒了还未抢到锁的线程）时，
    //      会漏掉这个刚进入enter逻辑之后还未执行到抢锁逻辑的线程，因为判断条件是小于等于INITIALIZED，
    //      此时释放锁的线程就会认为所有线程都抢到了锁都被执行了，没有未抢到锁未被执行的线程了，释放锁的线程就会把队列置为null，
    //      那这个刚进入enter还未执行到抢锁逻辑的线程，如果进入抢锁逻辑时，抢锁失败，同时这个线程加入队列的时机 在 释放锁的线程把队列置为null之前，那就会丢掉这个线程，没人唤醒
    Self->_state = RUNNABLE;

    for (int j = 0; j < 10000; j++) {
        val++;
    }

    // 业务逻辑执行完成，不包括解锁的逻辑
    Self->_state = FINISHED;
    // 退出临界区
    objectMonitor.exit(Self);

    // 执行完成，包括解锁的逻辑
    Self->_state = ZOMBIE;

    return 0;
}

JavaThread::JavaThread(string name) {
    _name = name;

    pthread_mutex_init(_startThread_lock, NULL);
    pthread_cond_init(_cond, NULL);

//    pthread_mutex_init(_sync_lock, NULL);
//    pthread_cond_init(_sync_cond, NULL);
    _sync_park_event = new ParkEvent(this);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    _state = ALLOCATED;

    // 自旋，将所创建的线程到objectMonitor
    for (;;) {
        if ((bool)(Atomic::cmpxchg_ptr(reinterpret_cast<void *>(true), (void *) &objectMonitor._entryListLock,
                                       reinterpret_cast<void *>(false))) == false) {
            objectMonitor._entryListLength++;
            INFO_PRINT("[%s] 加入waiter set", this->_name.c_str());
            ObjectWaiter* node = new ObjectWaiter(this);
            if (objectMonitor._entryList == NULL) {
                objectMonitor._entryList = node;
            } else {
                node->_next = objectMonitor._entryList;
                objectMonitor._entryList->_prev = node;
                objectMonitor._entryList = node;
            }
            objectMonitor._entryListLock = false;
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

//    pthread_mutex_init(_sync_lock, NULL);
//    pthread_cond_init(_sync_cond, NULL);
    _sync_park_event = new ParkEvent(this);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    _state = ALLOCATED;

    pthread_create(_tid, &attr, thread_do, this);

    pthread_attr_destroy(&attr);
}