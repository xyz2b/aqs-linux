//
// Created by hongyujiao on 7/12/21.
//

#ifndef MARK_CLEAR_COMMON_H
#define MARK_CLEAR_COMMON_H

#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <memory.h>
#include <string>
#include <pthread.h>
#include <map>
#include <sstream>
#include <iostream>

using namespace std;

typedef void *              pvoid;
typedef unsigned char       byte;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;

typedef union {
    long        l_dummy;
    double      d_dummy;
    void *      v_dummy;
} Align;

#define ALIGN_SIZE (sizeof(Align))

typedef enum {
    GC_MARK_CLEAN,
    GC_MARK_COLLECT,
    GC_MARK_COPY,
    GC_G1,
} GC_Type;

#define UseParallelGC false
#define UseG1GC false
#define UseSerialGC false
#define UseConcMarkSweepGC false
#define UseAdaptiveSizePolicy false

// gc算法
#define DEFAULT_GC_TYPE GC_MARK_COPY

/* ==============================
 * customize print
 * ============================== */
#define DEBUG           0
#define INFO            1
#define WARNING         2
#define ERROR           3

#define LOG_LEVEL         INFO

#define PRINT(level, msg, ...) do{ \
    printf("[%s] (%s:%d->%s): " msg"\n", level, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    fflush(stdout); \
}while(0)

#define DEBUG_PRINT(msg, ...) if (DEBUG >= LOG_LEVEL)  { \
    PRINT("DEBUG", msg, ##__VA_ARGS__);    \
}

#define INFO_PRINT(msg, ...) if (INFO >= LOG_LEVEL)  { \
    PRINT("INFO", msg, ##__VA_ARGS__);    \
}

#define WARNING_PRINT(msg, ...) if (WARNING >= LOG_LEVEL)  { \
    PRINT("WARNING", msg, ##__VA_ARGS__);    \
}

#define ERROR_PRINT(msg, ...) if (ERROR >= LOG_LEVEL)  { \
    PRINT("ERROR", msg, ##__VA_ARGS__);    \
}

#define NULL_POINTER(ptr) if (ptr == NULL) { \
    PRINT("ERROR", "null pointer", ##__VA_ARGS__);                                            \
}

#endif //MARK_CLEAR_COMMON_H
