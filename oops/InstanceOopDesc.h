//
// Created by xyzjiao on 11/19/21.
//

#ifndef AQS_INSTANCEOOPDESC_H
#define AQS_INSTANCEOOPDESC_H

#include "../include/common.h"

class InstanceOopDesc {
public:
    // 看成一个8字节的值
    markOop _mark;

    InstanceOopDesc();

    markOop mark() {
        return _mark;
    }

    markOop* mark_addr() const    {
        return (markOop*) &_mark;
    }

    void set_mark(volatile markOop m)      {
        _mark = m;
    }
};


#endif //AQS_INSTANCEOOPDESC_H
