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

    if (mark == (markOop)lock && lock->owner() == t) {
        if ((markOop) Atomic::cmpxchg_ptr(dhw, obj->mark_addr(), mark) == mark) {
            INFO_PRINT("[%s] 轻量级锁解锁成功", t->_name.c_str());

            return;
        }
    }

    // 重量级锁解锁
    // 先膨胀成重量级锁，获取重量级锁对象，然后再解锁重量级锁
    ObjectSynchronizer::inflate(obj, t)->exit(t);
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

    INFO_PRINT("[%s] 轻量级锁抢锁失败", t->_name.c_str());

    // Hotspot中的这句代码，没太明白意图
    lock->set_displaced_header(MarkOopDesc::unused_mark());
    // 膨胀成重量级锁
    ObjectSynchronizer::inflate(obj, t)->enter(t);
}

void ObjectSynchronizer::slow_exit(InstanceOopDesc *obj, BasicLock* lock, JavaThread *t) {
    fast_exit(obj, lock, t);
}

// 膨胀成重量级锁，返回重量级锁
// 膨胀的意思就是获取重量级锁对象（重量级锁实体对象ObjectMonitor）
ObjectMonitor *ObjectSynchronizer::inflate(InstanceOopDesc *obj, JavaThread *t) {
    for (;;) {
        markOop mark = obj->mark();

        // 1.对象头中已经是重量级锁，直接返回对应的重量级锁对象，用于加解锁（即已经是重量级锁）
        if (mark->has_monitor()) {
            // 获取重量级锁对象头中的重量级锁对象的指针
            ObjectMonitor* inf = mark->monitor();
            return inf;
        }

        // 2.目前对象头处于正在从轻量级锁膨胀成重量级锁的中间状态，即对象头为0，表示已经有线程触发膨胀了，还在膨胀的过程中
        if (mark == MarkOopDesc::INFLATING()) {
            INFO_PRINT("[%s] 检测到正在膨胀成重量级锁 %d", t->_name.c_str(), mark->has_monitor());

            // Hotspot源码的做法是阻塞等待唤醒

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
                INFO_PRINT("[%s] 膨胀成重量级锁成功 %X", t->_name.c_str(), mark->has_monitor());
            } else {
                INFO_PRINT("[%s] 膨胀成重量级锁失败", t->_name.c_str());
            }

            return m;
        }

        // 偏向锁直接膨胀成重量级锁
        return NULL;
    }
}
