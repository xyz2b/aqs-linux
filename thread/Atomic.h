//
// Created by ziya on 2021/11/12.
//

#ifndef LINUX_ATOMIC_H
#define LINUX_ATOMIC_H

#include "../include/common.h"

class Atomic {
public:
    /**
     *
     * @param exchange_value
     * @param dest
     * @return
     *
    int exchange_value = 10;

    int *dest = new int;
    *dest = 1;

    运行后的结果exchange_value=1，*dest=10
     */
    static int xchg(int exchange_value, volatile int* dest) {
        __asm__ volatile (
        "xchgl (%2),%0"
        : "=r" (exchange_value)
        : "0" (exchange_value), "r" (dest)
        : "memory");

        return exchange_value;
    }

    /**
     * cmpxchgl exchange_value, (dest)
     *
     * 比较eax(compare_value)与dest中的值
     *  相等：*dest\exchange_value的值发生交换,返回交换前*dest
     *  不相等: exchange_value = *dest,返回原exchange_value
     */
    static int cmpxchg(int exchange_value, volatile int* dest, int compare_value) {
        __asm__ volatile ("cmpxchgl %1,(%3)"
        : "=a" (exchange_value)
        : "r" (exchange_value), "a" (compare_value), "r" (dest)
        : "cc", "memory");

        return exchange_value;
    }

    /**
     * =============================================================
     */
    long _cmpxchg(long exchange_value, long* dest, long compare_value) {
        int ret = *dest;

        // 如果compare_value=*dest，*dest的值与exchange_value的值发生交换
        // 返回交换后的exchange_value,即原*dest
        if (*dest == compare_value) {
            *dest = exchange_value;
            exchange_value = ret;
        }

        // 如果compare_value!=*dest，返回*dest
        return ret;
    }

    // 本段代码的执行效果类似上面的方法的效果,不过这段代码是原子性的
    /**
     * 比较*dest == compare_value
     *      如果相等，*dest = exchange_value，返回*dest的原值
     *      如果不相等，则直接返回*dest的值
     * */
    static long cmpxchg(long exchange_value, volatile long* dest, long compare_value) {
        INFO_PRINT("exchange_value: %d, dest: %d, compare_value, %d", exchange_value, *dest, compare_value);
        __asm__ __volatile__ ("lock; cmpxchgq %1,(%3)"
        : "=a" (exchange_value)
        : "r" (exchange_value), "a" (compare_value), "r" (dest)
        : "cc", "memory");

        return exchange_value;
    }

    static void* cmpxchg_ptr(void* exchange_value, void* dest, void* compare_value) {
        // 这里是将dest的指针转成了long类型的指针，所以取值时，取得是long类型的值，如果本身dest是bool类型的指针，那这样取就会取多，取到其他的内存的值，可能和原来的值就不相同了，所以传入的dest需要是一个指向64位数据长度的指针
        return (void*)cmpxchg((long)exchange_value, (long*)dest, (long)compare_value);
    }
};


#endif //LINUX_ATOMIC_H
