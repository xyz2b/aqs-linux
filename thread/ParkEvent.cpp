//
// Created by xyzjiao on 11/18/21.
//

#include "ParkEvent.h"

#include "JavaThread.h"

void ParkEvent::park(ThreadState thread_status) {
    pthread_mutex_lock(_lock);
    _owner->_state = thread_status;
    INFO_PRINT("[%s] 线程阻塞，等待唤醒", _owner->_name.c_str());
    // pthread_cond_wait逻辑：会先解锁，然后进入阻塞状态。之后当被唤醒时会再加锁，然后解除阻塞状态（所以其参数要传入一个lock）
    pthread_cond_wait(_cond, _lock);
    pthread_mutex_unlock(_lock);
}

void ParkEvent::unpark() {
    pthread_mutex_lock(_lock);
    // pthread_cond_signal唤醒单个在对应cond上处于wait状态的线程（单个唤醒）（pthread_cond_broadcast：全部唤醒）
    pthread_cond_signal(_cond);
    pthread_mutex_unlock(_lock);
}
