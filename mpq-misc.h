#ifndef __MPQ_MISC_H__
#define __MPQ_MISC_H__

#include "mpqlib-stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t mpqlib_hash_filename(const char * str);
uint32_t mpqlib_hashA_filename(const char * str);
uint32_t mpqlib_hashB_filename(const char * str);

#ifdef __cplusplus
}
#endif

#endif
