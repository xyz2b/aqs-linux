#include "./include/common.h"
#include "./thread/oop/ObjectMonitor.h"
#include "./thread/oop/JavaThread.h"


int val = 0;
ObjectMonitor objectMonitor;

int test() {
    JavaThread* t1 = new JavaThread("t1");
    JavaThread* t2 = new JavaThread("t2");
    JavaThread* t3 = new JavaThread("t3");
    JavaThread* t4 = new JavaThread("t4");
    JavaThread* t5 = new JavaThread("t5");
    JavaThread* t6 = new JavaThread("t6");
    JavaThread* t7 = new JavaThread("t7");
    JavaThread* t8 = new JavaThread("t8");
    JavaThread* t9 = new JavaThread("t9");
    JavaThread* t10 = new JavaThread("t10");

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

    for (int i = 0; i < 10000; i++) {
        INFO_PRINT("run: %d", i);
        test();
    }

    return 0;
}