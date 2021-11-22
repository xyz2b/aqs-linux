//
// Created by ziya on 2021/9/21.
//

#ifndef ZIYA_AQS_CPP_OBJECTSYNCHRONIZER_H
#define ZIYA_AQS_CPP_OBJECTSYNCHRONIZER_H

class JavaThread;
class InstanceOopDesc;
class BasicLock;
class ObjectMonitor;

class ObjectSynchronizer {

public:
    static void fast_enter(InstanceOopDesc* obj, BasicLock* lock, JavaThread* t);
    static void fast_exit(InstanceOopDesc* obj, BasicLock* lock, JavaThread* t);

    static void slow_enter(InstanceOopDesc* obj, BasicLock* lock, JavaThread* t);
    static void slow_exit(InstanceOopDesc* obj, BasicLock* lock, JavaThread* t);

    static ObjectMonitor* inflate(InstanceOopDesc* obj, JavaThread* t, bool exit);
};


#endif //ZIYA_AQS_CPP_OBJECTSYNCHRONIZER_H
