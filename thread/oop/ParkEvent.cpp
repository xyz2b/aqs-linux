//
// Created by xyzjiao on 11/18/21.
//

#include "ParkEvent.h"

#include "JavaThread.h"

void ParkEvent::park() {
    pthread_mutex_lock(_lock);
    _owner->_state = INITIALIZED;
    INFO_PRINT("[%s] 线程阻塞，等待唤醒", _owner->_name.c_str());
    pthread_cond_wait(_cond, _lock);
    pthread_mutex_unlock(_lock);
}

void ParkEvent::unpark() {
    pthread_mutex_lock(_lock);
    pthread_cond_signal(_cond);
    pthread_mutex_unlock(_lock);
}
