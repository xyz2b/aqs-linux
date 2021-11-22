//
// Created by ziya on 2021/9/21.
//

#include "ObjectSynchronizer.h"
#include "../include/common.h"
#include "../oops/InstanceOopDesc.h"
#include "../oops/MarkOopDesc.h"
#include "BasicLock.h"
#include "Atomic.h"
#include "JavaThread.h"
#include "ObjectMonitor.h"
#include "ObjectWaiter.h"
#include "ParkEvent.h"

extern BasicLock lock;

void ObjectSynchronizer::fast_enter(InstanceOopDesc* obj, BasicLock* lock, JavaThread *t) {
    markOop mark = obj->mark();
    if (UseBiasedLocking) {

    }

    slow_enter(obj, lock, t);
}

void ObjectSynchronizer::fast_exit(InstanceOopDesc* obj, BasicLock* lock, JavaThread *t) {
    markOop dhw = lock->displaced_header();
    markOop mark;

    if (NULL == dhw) {
        // 处理重入，逻辑未写
        return;
    }

    INFO_PRINT("[%s] 开始解锁", t->_name.c_str());

    mark = obj->mark();

    // TODO: 轻量级锁膨胀成重量级锁之后，mark就变了，抢到轻量级锁解锁时，就不会走这个逻辑
    // 轻量级锁 解锁时什么都不做，就是还让mark的值为轻量级锁，后面的线程都会膨胀成重量级锁
    if (mark == (markOop)lock && lock->owner() == t) {
//        if ((markOop) Atomic::cmpxchg_ptr(lock, obj->mark_addr(), mark) == mark) {
            INFO_PRINT("[%s] 轻量级锁解锁成功", t->_name.c_str());

            return;
//        }
    }

    INFO_PRINT("[%s] 轻量级锁解锁失败，膨胀成重量级锁", t->_name.c_str());
    // 重量级锁解锁
    // 先膨胀成重量级锁，获取重量级锁对象，然后再解锁重量级锁
    ObjectSynchronizer::inflate(obj, t, true)->exit(t);
}

/**
 * 进入这个函数的情况：
 *      1、偏向锁未开启
 *      2、延迟偏向期间执行加锁
 *      3、重入
 */
void ObjectSynchronizer::slow_enter(InstanceOopDesc *obj, BasicLock* lock, JavaThread *t) {
    markOop mark = obj->mark();

    // 只有无锁才能变成轻量级锁
    // TODO: 如果已经膨胀成重量级锁了，之前轻量级锁的线程释放了轻量级锁，别的线程还能获取不到轻量级锁了，回退不了
    if(mark->is_neutral()) {
        // 保存上一个锁状态的对象头，这里就是将无锁的对象头保存起来
        lock->set_displaced_header(mark);

        // 抢轻量级锁
        if (mark == (markOop) Atomic::cmpxchg_ptr(lock, obj->mark_addr(), mark)) {
            INFO_PRINT("[%s] 上轻量级锁成功", t->_name.c_str());

            // 这个hotspot源码中没有，这里是为了判断重入加的
            lock->set_owner(t);
            return;
        }
    } else { // 轻量级锁重入
        // 重入的逻辑不是同一把锁

        // 是轻量级锁 且 持有轻量级锁的线程是当前线程
        if (mark && mark->has_locker() && t == mark->locker()->owner()) {
            assert(lock != mark->locker(), "must not re-lock the same lock");
            assert(lock != (BasicLock*)obj->mark(), "don't relock with same BasicLock");

            // 置为NULL是为了后续解锁时判断是不是重入时使用
            lock->set_displaced_header(NULL);
            return;
        }
    }

    // 某个线程抢轻量级锁失败，执行到膨胀逻辑之前，另一个抢到锁的线程就已经释放了轻量级锁，此时对象头中mark是无锁状态
    // 那这个抢轻量级锁进入膨胀逻辑时，判断mark是无锁状态，返回的重量级对象为NULL
    // 先行发生
    INFO_PRINT("[%s] 轻量级锁抢锁失败", t->_name.c_str());

    // Hotspot中的这句代码，没太明白意图
    lock->set_displaced_header(MarkOopDesc::unused_mark());
    // 膨胀成重量级锁
    ObjectMonitor* monitor = ObjectSynchronizer::inflate(obj, t, false);

    // TODO: 膨胀成重量级锁之后，之前拿到轻量级锁的线程需要阻塞等待
    // 等待抢到轻量级锁的线程执行完成
    while (true) {
        if (lock->owner() != NULL && lock->owner()->_state == ZOMBIE) {
            break;
        }
    }

    monitor->enter(t);

}

void ObjectSynchronizer::slow_exit(InstanceOopDesc *obj, BasicLock* lock, JavaThread *t) {
    fast_exit(obj, lock, t);
}

// 膨胀成重量级锁，返回重量级锁
// 膨胀的意思就是获取重量级锁对象（重量级锁实体对象ObjectMonitor）
ObjectMonitor *ObjectSynchronizer::inflate(InstanceOopDesc *obj, JavaThread *t, bool exit) {
    for (;;) {
        markOop mark = obj->mark();
        INFO_PRINT("[%s] 对象头 %X", t->_name.c_str(), mark);

        // 1.对象头中已经是重量级锁，直接返回对应的重量级锁对象，用于加解锁（即已经是重量级锁）
        if (mark->has_monitor()) {
            INFO_PRINT("[%s] 对象头中已经是重量级锁", t->_name.c_str());
            // 获取重量级锁对象头中的重量级锁对象的指针
            ObjectMonitor* inf = mark->monitor();

            // TODO: 存在先行发生，如果重量级锁解锁时，该线程还在加入集合的过程中，就不会判断其状态，可能会漏掉唤醒
            if (!exit) {
                // 自旋，将膨胀到重量级锁的线程到objectMonitor
                for (;;) {
                    if ((bool)(Atomic::cmpxchg_ptr(reinterpret_cast<void *>(true), (void *) &inf->_entryListLock,
                                                   reinterpret_cast<void *>(false))) == false) {
                        INFO_PRINT("[%s] 加入waiter set", t->_name.c_str());
                        ObjectWaiter* node = new ObjectWaiter(t);
                        if (inf->_entryList == NULL) {
                            inf->_entryList = node;
                        } else {
                            node->_next = inf->_entryList;
                            inf->_entryList->_prev = node;
                            inf->_entryList = node;
                        }
                        inf->_entryListLength++;
                        inf->_entryListLock = false;
                        break;
                    }
                    sleep(1);
                }
            }

            return inf;
        }

        // 2.目前对象头处于正在从轻量级锁膨胀成重量级锁的中间状态，即对象头为0，表示已经有线程触发膨胀了，还在膨胀的过程中，就需要等待这个线程的膨胀过程完成
        // 只需要重试即可，判断对象头是否已经是重量级锁了，是就直接返回重量级锁对象
        if (mark == MarkOopDesc::INFLATING()) {
            INFO_PRINT("[%s] 检测到正在膨胀成重量级锁 %d，阻塞等待唤醒", t->_name.c_str(), mark->has_monitor());

            // Hotspot源码的做法是阻塞等待唤醒
//            t->_light_to_monitor_park_event->park(INITIALIZED);
            sleep(1);
            continue;
        }

        // 2.目前对象头中还是轻量级锁（还未膨胀过），需要膨胀成重量级锁，此时还没有重量级锁对象，需要创建
        if (mark->has_locker()) {
            ObjectMonitor* m = new ObjectMonitor;

            // 轻量级锁膨胀到重量级锁时，对象头需要先置为0，表示有线程触发对象头正在从轻量级锁膨胀成重量级锁，一个中间状态
            if (mark != (markOop) Atomic::cmpxchg_ptr(MarkOopDesc::INFLATING(), obj->mark_addr(), mark)) {
                // 置0失败，代表有别的线程已经在膨胀过程中，并且膨胀成重量级锁成功，已经将mark修改了，只需要重试即可，判断对象头是否已经是重量级锁了，是就直接返回重量级锁对象
                INFO_PRINT("[%s] 更正状态为正在膨胀成重量级锁失败", t->_name.c_str());

                continue ;       // Interference -- just retry
            }

            INFO_PRINT("[%s] 更正状态为正在膨胀成重量级锁成功", t->_name.c_str());

            // 设置锁正在膨胀之后
            markOop dmw = mark->displaced_mark_helper();

            m->set_header(dmw);

            m->set_object(obj);

            // 将重量级锁写入对象头markOop中
            // 只有处于从轻量级锁膨胀成重量级锁的状态时（即处于膨胀过程的中间状态，对象头为0时），才能将重量级锁写入对象头markOop中（才能改变对象头为重量级锁）
            mark = MarkOopDesc::INFLATING();
            if (mark == (markOop) Atomic::cmpxchg_ptr(MarkOopDesc::encode(m), obj->mark_addr(), mark)) {
                INFO_PRINT("[%s] 膨胀成重量级锁成功 %X，唤醒其他等待的线程", t->_name.c_str(), mark->has_monitor());
//                t->_light_to_monitor_park_event->unpark();
            } else {
                INFO_PRINT("[%s] 膨胀成重量级锁失败", t->_name.c_str());
                continue;
            }

            if (!exit) {
                // 自旋，将膨胀到重量级锁的线程到objectMonitor
                for (;;) {
                    if ((bool)(Atomic::cmpxchg_ptr(reinterpret_cast<void *>(true), (void *) &m->_entryListLock,
                                                   reinterpret_cast<void *>(false))) == false) {
                        INFO_PRINT("[%s] 加入waiter set", t->_name.c_str());
                        ObjectWaiter* node = new ObjectWaiter(t);
                        if (m->_entryList == NULL) {
                            m->_entryList = node;
                        } else {
                            node->_next = m->_entryList;
                            m->_entryList->_prev = node;
                            m->_entryList = node;
                        }
                        m->_entryListLength++;
                        m->_entryListLock = false;
                        break;
                    }
                    sleep(1);
                }
            }

            return m;
        }

//        // 偏向锁直接膨胀成重量级锁
//        return NULL;
    }
}
