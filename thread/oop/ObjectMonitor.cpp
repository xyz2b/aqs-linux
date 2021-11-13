//
// Created by ziya on 2021/11/12.
//

#include "ObjectMonitor.h"

#include "../../include/common.h"
#include "JavaThread.h"
#include "Atomic.h"
#include "ObjectWaiter.h"

void ObjectMonitor::enter(JavaThread *thread) {
    void* ret = NULL;

    // 如果当前没有线程此有锁，即_owner为NULL，就可以直接抢锁，抢锁成功返回原owner的值
    // 原owner为NULL代表抢锁成功，如果不为NULL，代表已有有线程持有锁了，即返回持有锁的线程
    if ((ret = Atomic::cmpxchg_ptr(thread, &_owner, NULL)) == NULL) {
        INFO_PRINT("[%s] 抢到锁", thread->_name.c_str());
        return;
    }

    // 如果抢锁的线程和持有锁的线程是同一个线程，可以重入
    if (thread == ret) {
        // 重入次数加一
        _recursions++;
        return;
    }

    INFO_PRINT("[%s] 抢锁失败，加入队列", thread->_name.c_str());

    // 加入队列
    ObjectWaiter* node = new ObjectWaiter(thread);
    for (;;) {
        if (_tail == NULL) { // 队列中还没有线程
            if (Atomic::cmpxchg_ptr(node, &_tail, NULL) == NULL) {
                _head = node;

//                node._next = node._prev = &node;

                INFO_PRINT( "[%s] 第一个，加入队列成功", thread->_name.c_str());
                break;
            } else {
                INFO_PRINT( "[%s] 第一个，加入队列失败", thread->_name.c_str());
            }
        } else { // 队列中有线程
            ObjectWaiter* tail = const_cast<ObjectWaiter *>(_tail);

            node->_prev = tail;

            if (Atomic::cmpxchg_ptr(node, &_tail, tail) == tail) {
                tail->_next = node;

                INFO_PRINT( "[%s] 不是第一个，加入队列成功", thread->_name.c_str());
                break;
            } else {
                INFO_PRINT( "[%s] 不是第一个，加入队列失败", thread->_name.c_str());
            }
        }
    }

    // 阻塞等待唤醒，当持有锁的线程，释放锁时唤醒，让队列中的线程进行抢锁
    pthread_mutex_lock(thread->_startThread_lock);
    INFO_PRINT("[%s] 线程阻塞，等待唤醒", thread->_name.c_str());
    pthread_cond_wait(thread->_cond, thread->_startThread_lock);
    pthread_mutex_unlock(thread->_startThread_lock);

    INFO_PRINT("[%s] 被唤醒运行", thread->_name.c_str());
    // 抢锁
    enter(thread);
}

void ObjectMonitor::exit(JavaThread *thread) {
    // 处理重入
    // 如果重入次数不为0，就重入次数减一
    // 如果重入次数为0，就释放锁
    if (_owner == thread && 0 != _recursions) {
        _recursions--;
        return;
    }

    // 释放锁，并唤醒队列中的一个线程，根据是公平锁或非公平锁选择下一个抢锁的线程

    // 清除锁标志，释放锁
    INFO_PRINT("[%s] 释放锁", thread->_name.c_str());
    _owner = NULL;

    // 取出队列中第一个元素
    ObjectWaiter* head = const_cast<ObjectWaiter *>(_head);

    // 如果队列中没有元素了，即没有等待的线程了，就不需要再唤醒了，直接退出即可
    if (head == NULL) {
        return;
    }

    JavaThread* head_thread = const_cast<JavaThread *>(head->_thread);
    INFO_PRINT("[%s] 释放锁，唤醒 [%s]", thread->_name.c_str(), head_thread->_name.c_str());

    if (head->_next != NULL) {
        _head = head->_next;

        // 唤醒线程
        pthread_cond_signal(head_thread->_cond);
    } else {
        // 唤醒线程
        pthread_cond_signal(head_thread->_cond);
    }
}


