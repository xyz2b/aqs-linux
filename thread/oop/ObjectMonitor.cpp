//
// Created by ziya on 2021/11/12.
//

#include "ObjectMonitor.h"

#include "../../include/common.h"
#include "JavaThread.h"
#include "Atomic.h"
#include "ObjectWaiter.h"

void ObjectMonitor::print(string name) {
    ObjectWaiter* next = const_cast<ObjectWaiter *>(_head);

    stringstream ss;
    ss << name << " queue: [";
    while (next != NULL) {
        JavaThread* next_thread = const_cast<JavaThread *>(next->_thread);

        ss << next_thread->_name << ", ";
        next = const_cast<ObjectWaiter *>(next->_next);
    }
    ss << "]";
    INFO_PRINT("%s", ss.str().c_str());
}

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
    // 死循环是确保线程成功入队，没有入队之后就没办法被唤醒了
    for (;;) {
        if (_tail == NULL) { // 队列中还没有线程
            if (Atomic::cmpxchg_ptr(node, &_tail, NULL) == NULL) {
                _head = node;

//                node._next = node._prev = &node;

                INFO_PRINT( "[%s] 第一个，加入队列成功", thread->_name.c_str());
                print(thread->_name);
                break;
            } else {
                INFO_PRINT( "[%s] 第一个，加入队列失败", thread->_name.c_str());
                print(thread->_name);
            }
        } else { // 队列中有线程
            ObjectWaiter* tail = const_cast<ObjectWaiter *>(_tail);

            node->_prev = tail;

            if (Atomic::cmpxchg_ptr(node, &_tail, tail) == tail) {
                tail->_next = node;

                INFO_PRINT( "[%s] 不是第一个，加入队列成功", thread->_name.c_str());
                print(thread->_name);
                break;
            } else {
                INFO_PRINT( "[%s] 不是第一个，加入队列失败", thread->_name.c_str());
                print(thread->_name);
            }
        }
    }

    // 阻塞等待唤醒，当持有锁的线程，释放锁时唤醒，让队列中的线程进行抢锁
    pthread_mutex_lock(thread->_startThread_lock);
    thread->_state = MONITOR_WAIT;
    INFO_PRINT("[%s] 线程阻塞，等待唤醒", thread->_name.c_str());
    pthread_cond_wait(thread->_cond, thread->_startThread_lock);
    pthread_mutex_unlock(thread->_startThread_lock);

    thread->_state = RUNNABLE;
    INFO_PRINT("[%s] 被唤醒运行", thread->_name.c_str());
    // 抢锁
    enter(thread);
}

void ObjectMonitor::exit(JavaThread *thread) {
    // TODO: JavaThread是否相等，需要重写JavaThread的operator ==，根据两个JavaThread的tid是否相等进行判断（pthread_equal）
    if (_owner != thread) {
        INFO_PRINT("[%s] 非持锁线程不得释放锁", thread->_name.c_str());
        return;
    }

    // 处理重入
    // 如果重入次数不为0，就重入次数减一
    // 如果重入次数为0，就释放锁
    if (0 != _recursions) {
        _recursions--;
        return;
    }

    // 释放锁，并唤醒队列中的一个线程，根据是公平锁或非公平锁选择下一个抢锁的线程

    // 最后一个等待线程还没有成功入队，但是该线程执行完了，之后没有人唤醒最后一个等待线程了
    // 如何判定所有线程都执行完毕了？
    // 自旋，判断所保存的所有线程状态，都执行完毕之后退出，没有执行完毕就等待
    while (true) {
        ObjectWaiter* next = _waiterSet;
        bool all_done = true;
        for (int i = 0; i < _waiters; i++) {
            JavaThread* next_thread = const_cast<JavaThread *>(next->_thread);
            if (next_thread->_state < FINISHED) {
                all_done = false;
                break;
            }
            next = const_cast<ObjectWaiter *>(next->_next);
        }

        if (all_done) {
            INFO_PRINT("[%s] all done", thread->_name.c_str());
            // 清除锁标志，释放锁
            INFO_PRINT("[%s] 释放锁", thread->_name.c_str());
            _owner = NULL;
            print(thread->_name);
            return;
        } else {
            INFO_PRINT("[%s] not all done", thread->_name.c_str());
        }

        if (_head != NULL) {
            INFO_PRINT("[%s]释放锁，head不为null", thread->_name.c_str());
            print(thread->_name);
            break;
        } else {
            INFO_PRINT("[%s]释放锁，head为null", thread->_name.c_str());
            print(thread->_name);
        }

        sleep(1);
    }

    // 取出队列中第一个元素
    ObjectWaiter* head = const_cast<ObjectWaiter *>(_head);

    JavaThread* head_thread = const_cast<JavaThread *>(head->_thread);
    INFO_PRINT("[%s] head thread [%s]", thread->_name.c_str(), head_thread->_name.c_str());

    while (true) {
        if (head->_next != NULL) {
            _head = head->_next;
            print(thread->_name);
            break;
        } else {
            // 将head置为NULL之前head->_next可能被赋值，然后再将head置为NULL，就把next丢了
            // head->_next什么时候应该为NULL？当前要被唤醒的线程是最后一个未被唤醒的线程
            ObjectWaiter* next = _waiterSet;
            int unrun = 0;
            for (int i = 0; i < _waiters; i++) {
                JavaThread* next_thread = const_cast<JavaThread *>(next->_thread);
                if (next_thread->_state <= MONITOR_WAIT) {
                    unrun++;
                }
                next = const_cast<ObjectWaiter *>(next->_next);
            }

            if (unrun == 1) {
                _head = _tail = NULL;
                print(thread->_name);
                break;
            }
        }
    }

    // 清除锁标志，释放锁
    INFO_PRINT("[%s] 释放锁", thread->_name.c_str());
    _owner = NULL;

    // 唤醒等待线程
    while (true) {
        // 这里的逻辑是为了防止加入的队列中的线程还未执行到阻塞逻辑，就被释放锁的线程唤醒了
        // 两个线程执行的快慢问题，有阻塞逻辑那个线程执行完设置状态之后，
        // 还没来得及执行阻塞逻辑，另一个有唤醒逻辑的线程，就把判断状态以及唤醒逻辑执行完了，造成了先唤醒后阻塞
        // 所以这里在唤醒时加锁
        INFO_PRINT("[%s] head thread %s state %d", thread->_name.c_str(), head_thread->_name.c_str(), head_thread->_state);
        if (head_thread->_state == MONITOR_WAIT) {
            INFO_PRINT("[%s] 释放锁，唤醒 [%s]", thread->_name.c_str(), head_thread->_name.c_str());
            pthread_mutex_lock(head_thread->_startThread_lock);
            pthread_cond_signal(head_thread->_cond);
            pthread_mutex_unlock(head_thread->_startThread_lock);
            break;
        }
        sleep(1);
    }
}
