//
// Created by xyzjiao on 11/9/21.
//

#ifndef LINUX_JAVATHREAD_H
#define LINUX_JAVATHREAD_H

#include "../include/common.h"

class ParkEvent;

enum ThreadState {
    ALLOCATED,                    // Memory has been allocated but not initialized
    INITIALIZED,                  // The thread has been initialized but yet started
    RUNNABLE,                     // Has been started and is runnable, but not necessarily running
    SYNC_WAIT,
    MONITOR_WAIT,                 // Waiting on a contended monitor lock
    CONDVAR_WAIT,                 // Waiting on a condition variable
    OBJECT_WAIT,                  // Waiting on an Object.wait() call
    BREAKPOINTED,                 // Suspended at breakpoint
    SLEEPING,                     // Thread.sleep()
    FINISHED,
    ZOMBIE                        // All done, but not reclaimed yet
};

class JavaThread {
private:


public:
    // 线程的tid
    pthread_t _tid[1];
    // 线程的名字
    string _name;

    // 用于锁以及阻塞线程
    pthread_mutex_t _startThread_lock[1];
    pthread_cond_t _cond[1];

    // 用于synchronized，阻塞唤醒线程
    ParkEvent* _sync_park_event;

    // 线程的状态
    ThreadState _state;
    // 中断标记位，JavaThread标识为中断，当其被唤醒之后会自己退出（在执行业务逻辑前）
    bool _interrupted;
public:
    JavaThread(string name);
    JavaThread(int index);
public:
    void run();
    void join();
};


#endif //LINUX_JAVATHREAD_H
