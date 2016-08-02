#ifndef __STD_INT_H_
#define __STD_INT_H_

#ifdef _MSC_VER
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef signed long int int32_t;
typedef unsigned long int uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#endif
