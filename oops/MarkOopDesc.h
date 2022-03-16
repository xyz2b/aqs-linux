//
// Created by xyzjiao on 11/19/21.
//

#ifndef AQS_MARKOOPDESC_H
#define AQS_MARKOOPDESC_H

#include "../include/common.h"
#include "../include/GlobalDefinitions.h"
#include "../thread/JavaThread.h"
#include "../thread/ObjectMonitor.h"
#include "../thread/BasicLock.h"

class MarkOopDesc {
private:
    // Conversion
    uintptr_t value() const {
        // 返回该对象的this指针
        // 该类虽然没有属性，创建出来的对象是个空对象，但是在C++的机制中，空对象也是会占用内存的，32位操作系统占用4字节，64位操作系统占用8字节
        // HotSpot就用空对象所占用的内存来存储MarkWord(64位操作系统下也是8字节)
        // 创建对象时使用这样的语法：MarkOop mark = MarkOop(value);      // typedef MarkOopDesc* markOop;
        // 这里创建对象的方式比较特殊，这样创建出来的对象，如果直接打印指向该对象的指针(mark)，打印出来的是创建对象时传入的值(value)
        // 所以value方法返回的该对象的this指针，其对应的值其实是使用上面的语法创建该对象时传入的值，而不是指向该对象的指针地址
        return (uintptr_t) this;
    }

public:
    // Constants
    enum {
        // 分代年龄所占用的bit数
        age_bits                 = 4,
        // 锁标志位所占的bit数
        lock_bits                = 2,
        // 偏向锁位所占的bit数
        biased_lock_bits         = 1,
        // hashCode所能占用的最大位数
        max_hash_bits            = 64 - age_bits - lock_bits - biased_lock_bits,
        // hashCode所占用的bit数（31bit）
        hash_bits                = max_hash_bits > 31 ? 31 : max_hash_bits,
        cms_bits                 = 1,   // 这个有啥用
        // epoch所占用的bit数
        epoch_bits               = 2
    };

    // The biased locking code currently requires that the age bits be
    // contiguous to the lock bits.
    enum {
        // 锁标志位的偏移量
        lock_shift               = 0,
        // 偏向锁位的偏移量
        biased_lock_shift        = lock_bits, // 2
        // 分代年龄的偏移量
        age_shift                = lock_bits + biased_lock_bits, // 3
        cms_shift                = age_shift + age_bits, // 7
        // hashCode的偏移量
        hash_shift               = cms_shift + cms_bits, // 8
        // epoch的偏移量
        epoch_shift              = hash_shift // 8
    };


    enum {
        lock_mask                = right_n_bits(lock_bits),     // 011
        lock_mask_in_place       = lock_mask << lock_shift,     // 011
        biased_lock_mask         = right_n_bits(lock_bits + biased_lock_bits),  // 111
        biased_lock_mask_in_place= biased_lock_mask << lock_shift,  // 111
        biased_lock_bit_in_place = 1 << biased_lock_shift,          // 100
        age_mask                 = right_n_bits(age_bits),          // 1111
        age_mask_in_place        = age_mask << age_shift,           // 0111 1000
        epoch_mask               = right_n_bits(epoch_bits),        // 011
        epoch_mask_in_place      = epoch_mask << epoch_shift,       // 0011 0000 0000
        cms_mask                 = right_n_bits(cms_bits),          // 001
        cms_mask_in_place        = cms_mask << cms_shift            // 1000 0000
#ifndef _WIN64
        ,hash_mask               = right_n_bits(hash_bits),         // 01111111111111111111111111111111
        hash_mask_in_place       = (address_word)hash_mask << hash_shift // 0111111111111111111111111111111100000000
#endif
    };

    // Alignment of JavaThread pointers encoded in object header required by biased locking
    enum {
        biased_lock_alignment    = 2 << (epoch_shift + epoch_bits)
    };

#ifdef _WIN64
    // These values are too big for Win64
    const static uintptr_t hash_mask = right_n_bits(hash_bits);
    const static uintptr_t hash_mask_in_place  =
                            (address_word)hash_mask << hash_shift;
#endif

    enum {
        // 轻量级锁
        locked_value             = 0,
        // 无锁
        unlocked_value           = 1,
        // 重量级锁
        monitor_value            = 2,
        // GC
        marked_value             = 3,
        // 偏向锁
        biased_lock_pattern      = 5
    };

    enum {
        no_hash                  = 0
    };  // no hash value assigned

    enum {
        // 0
        no_hash_in_place         = (address_word)no_hash << hash_shift,
        // 1
        no_lock_in_place         = unlocked_value
    };

    enum {
        max_age                  = age_mask
    };

    enum {
        max_bias_epoch           = epoch_mask
    };

    bool has_bias_pattern() const {
        /**
         *  value() 返回对象头的内容
         *  biased_lock_mask_in_place   0111
         *  biased_lock_pattern         0101    支持偏向的偏向锁
         *
         *  判断是否是偏向锁
         *  (value() & biased_lock_mask_in_place) == biased_lock_pattern
         */
        return (mask_bits(value(), biased_lock_mask_in_place) == biased_lock_pattern);
    }

    JavaThread* biased_locker() const {
        assert(has_bias_pattern(), "should not call this otherwise");
        /**
         *  value() 返回对象头的内容
         *  biased_lock_mask_in_place              0111
         *  age_mask_in_place                 0111 1000
         *  epoch_mask_in_place          0011 0000 0000
         *  ~(biased_lock_mask_in_place | age_mask_in_place | epoch_mask_in_place) ...1111 1100 1000 0000，取最高的54位，即当前线程指针(JavaThread*)
         *
         *  获取偏向锁中存储的当前线程指针（JavaThread*）
         */
        return (JavaThread*) ((intptr_t) (mask_bits(value(),
                                                ~(biased_lock_mask_in_place | age_mask_in_place | epoch_mask_in_place))));
    }

    /**
     * 该函数的作用:
     *  1. 判断某个对象头是否已支持偏向锁
     *  2. 判断某个对象头是否还未偏向任何线程
     * @return
     */
    bool is_biased_anonymously() const {
        return (has_bias_pattern() && (biased_locker() == NULL));
    }

    /**
     * 从对象头中取出epoch的值
     * @return
     */
    int bias_epoch() const {
        assert(has_bias_pattern(), "should not call this otherwise");
        /**
         *  epoch_mask_in_place    0011 0000 0000
         *  epoch_shift            8
         */
        return (mask_bits(value(), epoch_mask_in_place) >> epoch_shift);
    }

    /**
     * 修改对象头中epoch的值
     * @param epoch
     * @return
     */
    markOop set_bias_epoch(int epoch) {
        // 这个判断是为了判断传入的参数epoch是否越界 epoch只占两位,取值范围为0-3
        assert((epoch & (~epoch_mask)) == 0, "epoch overflow");

        /**
         * mask_bits(value(), ~epoch_mask_in_place) 将epoch位置0
         * (epoch << epoch_shift) 将新的epoch值移动到对应位置
         * markOop(mask_bits(value(), ~epoch_mask_in_place) | (epoch << epoch_shift))  设置epoch值
         * */
        return markOop(mask_bits(value(), ~epoch_mask_in_place) | (epoch << epoch_shift));
    }

    // epoch自增1
    markOop incr_bias_epoch() {
        return set_bias_epoch((1 + bias_epoch()) & epoch_mask);
    }

    // 返回初始化偏向锁之后的对象头，即低3位为101
    static markOop biased_locking_prototype() {
        return markOop( biased_lock_pattern );
    }

    /**
     * mark的最后2位 != 01
     * 言外之意就是轻量级锁\重量级锁时返回true
     * @return
     */
    bool is_locked()   const {
        /**
         * lock_mask_in_place   0011
         * unlocked_value       0001
         */
        return (mask_bits(value(), lock_mask_in_place) != unlocked_value);
    }

    /**
     * mark的最后3位 == 001
     * 言外之意就是无锁时返回true
     * @return
     */
    bool is_unlocked() const {
        /**
         *  biased_lock_mask_in_place    0111
         */
        return (mask_bits(value(), biased_lock_mask_in_place) == unlocked_value);
    }

    /**
     * mark的最后2位 == 11
     * @return
     */
    bool is_marked()   const {
        /**
         * lock_mask_in_place     0011
         * marked_value            011
         */
        return (mask_bits(value(), lock_mask_in_place) == marked_value);
    }

    /**
     * mark的最后3位 == 001
     * 言外之意就是无锁时返回true
     * 与is_unlocked一模一样
     * @return
     */
    bool is_neutral()  const {
        /**
         *  biased_lock_mask_in_place       0111
         *  unlocked_value                  0001
         */
        return (mask_bits(value(), biased_lock_mask_in_place) == unlocked_value);
    }

    /**
     * 正在进行膨胀
     * @return
     */
    bool is_being_inflated() const {
        return (value() == 0);
    }

    /**
     * 后两位置为 01，无锁/偏向锁
     * @return
     */
    markOop set_unlocked() const {
        return markOop(value() | unlocked_value);
    }

    /**
     * 后两位为00时返回true
     * 言外之意: 轻量级锁时返回true
     * @return
     */
    bool has_locker() const {
        return ((value() & lock_mask_in_place) == locked_value);
    }

    /**
     * 获取轻量级锁
     * @return
     */
    BasicLock* locker() const {
        assert(has_locker(), "check");
        return (BasicLock*) value();
    }

    /**
     * 判断是不是重量级锁
     * @return
     */
    bool has_monitor() const {
        return ((value() & monitor_value) != 0);
    }

    // 返回重量级锁对象头中指向重量级锁对象的指针
    ObjectMonitor* monitor() const {
        assert(has_monitor(), "check");
        // Use xor instead of &~ to provide one extra tag-bit check.
        return (ObjectMonitor*) (value() ^ monitor_value);
    }


    bool has_displaced_mark_helper() const {
        /**
         * unlocked_value 1
         */
        return ((value() & unlocked_value) == 0);
    }

    markOop displaced_mark_helper() const {
        assert(has_displaced_mark_helper(), "check");
        intptr_t ptr = (value() & ~monitor_value);
        return *(markOop*)ptr;
    }

    void set_displaced_mark_helper(markOop m) const {
        assert(has_displaced_mark_helper(), "check");
        intptr_t ptr = (value() & ~monitor_value);
        *(markOop*)ptr = m;
    }

    markOop copy_set_hash(intptr_t hash) const {
        intptr_t tmp = value() & (~hash_mask_in_place);
        tmp |= ((hash & hash_mask) << hash_shift);
        return (markOop)tmp;
    }

    static markOop unused_mark() {
        return (markOop) marked_value;
    }

    static markOop encode(BasicLock* lock) {
        return (markOop) lock;
    }

    static markOop encode(ObjectMonitor* monitor) {
        intptr_t tmp = (intptr_t) monitor;
        return (markOop) (tmp | monitor_value);
    }

    static markOop encode(JavaThread* thread, uint age, int bias_epoch) {
        intptr_t tmp = (intptr_t) thread;
        assert(UseBiasedLocking && ((tmp & (epoch_mask_in_place | age_mask_in_place | biased_lock_mask_in_place)) == 0), "misaligned JavaThread pointer");
        assert(age <= max_age, "age too large");
        assert(bias_epoch <= max_bias_epoch, "bias epoch too large");
        return (markOop) (tmp | (bias_epoch << epoch_shift) | (age << age_shift) | biased_lock_pattern);
    }

    // used to encode pointers during GC
    markOop clear_lock_bits() {
        return markOop(value() & ~lock_mask_in_place);
    }

    // age operations
    markOop set_marked()   {
        return markOop((value() & ~lock_mask_in_place) | marked_value);
    }

    markOop set_unmarked() {
        return markOop((value() & ~lock_mask_in_place) | unlocked_value);
    }

    uint    age()               const {
        return mask_bits(value() >> age_shift, age_mask);
    }

    markOop set_age(uint v) const {
        assert((v & ~age_mask) == 0, "shouldn't overflow age field");
        return markOop((value() & ~age_mask_in_place) | (((uintptr_t)v & age_mask) << age_shift));
    }

    markOop incr_age()          const {
        return age() == max_age ? markOop(this) : set_age(age() + 1);
    }

    // hash operations
    intptr_t hash() const {
        return mask_bits(value() >> hash_shift, hash_mask);
    }

    bool has_no_hash() const {
        return hash() == no_hash;
    }

    static markOop INFLATING() {
        return (markOop) 0;
    }

public:
    // 无锁的MarkOop，初始化mark时用
    static markOop prototype() {
        // return 1
        // 这里创建对象的方式比较特殊，这样创建出来的对象，如果直接打印指向该对象的指针，打印出来的是创建对象时传入的值，如这里的( no_hash_in_place | no_lock_in_place )
        // 所以上面value方法返回的该对象的this指针，其对应的值其实是创建该对象时传入的值，而不是指向该对象的指针地址
        // 正常创建对象时，直接打印指向该对象的指针，打印出来的是指针地址
        return markOop( no_hash_in_place | no_lock_in_place );
    }
};


#endif //AQS_MARKOOPDESC_H
