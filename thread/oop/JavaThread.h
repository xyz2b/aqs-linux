//
// Created by xyzjiao on 11/9/21.
//

#ifndef LINUX_JAVATHREAD_H
#define LINUX_JAVATHREAD_H

#include "../../include/common.h"

enum ThreadState {
    ALLOCATED,                    // Memory has been allocated but not initialized
    INITIALIZED,                  // The thread has been initialized but yet started
    RUNNABLE,                     // Has been started and is runnable, but not necessarily running
    MONITOR_WAIT,                 // Waiting on a contended monitor lock
    CONDVAR_WAIT,                 // Waiting on a condition variable
    OBJECT_WAIT,                  // Waiting on an Object.wait() call
    BREAKPOINTED,                 // Suspended at breakpoint
    SLEEPING,                     // Thread.sleep()
    ZOMBIE                        // All done, but not reclaimed yet
};

class JavaThread {
private:


public:
    pthread_t _tid[1];
    string _name;

    pthread_mutex_t _startThread_lock[1];
    pthread_cond_t _cond[1];

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
