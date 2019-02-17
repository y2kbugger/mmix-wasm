#ifndef _UINT64_H_
#define _UINT64_H_

#ifdef WIN32
#include <windows.h>
typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;
typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
#else
#include <stdint.h>
#endif

extern void uint64_to_hex(uint64_t u, char *c);
extern uint64_t hex_to_uint64(char *str);
extern uint64_t strtouint64(char *arg);

#endif