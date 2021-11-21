#include "./include/common.h"
#include "./thread/ObjectMonitor.h"
#include "./thread/JavaThread.h"
#include "./include/GlobalDefinitions.h"
#include "./thread/BasicLock.h"
#include "./thread/ObjectSynchronizer.h"
#include "./oops/InstanceOopDesc.h"

int val = 0;
ObjectMonitor objectMonitor;
BasicLock lock;
InstanceOopDesc obj;

void* thread_do_1(void* arg) {
    JavaThread* Self = static_cast<JavaThread *>(arg);

    // 开始执行业务逻辑，包括加锁的逻辑
    // 如果进入enter之前状态大于SYNC_WAIT，会导致目前要释放锁的线程判断最后一个未抢到锁未被执行的线程（包括：被阻塞的线程、唤醒了还未抢到锁的线程）时，
    //      会漏掉这个刚进入enter还未执行到抢锁逻辑的线程，因为判断条件是小于等于SYNC_WAIT，
    //      此时目前要释放锁的线程就会认为所有线程都抢到了锁都被执行了，没有未抢到锁未被执行的线程了，目前要释放锁的线程就会把队列置为null，
    //      那这个刚进入enter还未执行到抢锁逻辑的线程，如果进入抢锁逻辑时，抢锁失败，同时这个线程加入队列的时机 在 释放锁的线程把队列置为null之前，那就会丢掉这个线程，没人唤醒
    Self->_state = RUNNABLE;

    // 进入临界区
    ObjectSynchronizer::fast_enter(&obj, &lock, Self);

    for (int j = 0; j < 10000; j++) {
        val++;
    }

    // 业务逻辑执行完成，不包括解锁的逻辑
    Self->_state = FINISHED;

    // 退出临界区
    ObjectSynchronizer::fast_exit(&obj, &lock, Self);

    return 0;
}

int test() {
    JavaThread* t1 = new JavaThread(thread_do_1, NULL, "t1");
    JavaThread* t2 = new JavaThread(thread_do_1, NULL, "t2");
    JavaThread* t3 = new JavaThread(thread_do_1, NULL, "t3");
    JavaThread* t4 = new JavaThread(thread_do_1, NULL, "t4");
    JavaThread* t5 = new JavaThread(thread_do_1, NULL, "t5");
    JavaThread* t6 = new JavaThread(thread_do_1, NULL, "t6");
    JavaThread* t7 = new JavaThread(thread_do_1, NULL, "t7");
    JavaThread* t8 = new JavaThread(thread_do_1, NULL, "t8");
    JavaThread* t9 = new JavaThread(thread_do_1, NULL, "t9");
    JavaThread* t10 = new JavaThread(thread_do_1, NULL, "t10");

    t1->run();
    t2->run();
    t3->run();
    t4->run();
    t5->run();
    t6->run();
    t7->run();
    t8->run();
    t9->run();
    t10->run();

    t1->join();
    t2->join();
    t3->join();
    t4->join();
    t5->join();
    t6->join();
    t7->join();
    t8->join();
    t9->join();
    t10->join();

    if (val != 100000) {
        exit(-1);
    } else {
        INFO_PRINT("val: %d", val);
        val = 0;
    }
}

int main() {
    test();
//    for (int i = 0; i < 10000; i++) {
//        INFO_PRINT("run: %d", i);
//        test();
//    }

    // 直接将对象指针当成一个long类型的整数来用，传入的值就是这个整数的值，然后返回的也是传入的整数值
//    markOop o = markOop(1);
//    INFO_PRINT("%X", o);
//
//    INFO_PRINT("%ld", right_n_bits(31) << 8);

    return 0;
}