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

    t1->run();
    t2->run();
    t3->run();
    t4->run();
    t5->run();

    t1->join();
    t2->join();
    t3->join();
    t4->join();
    t5->join();

    if (val != 50000) {
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