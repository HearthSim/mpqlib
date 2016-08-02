#include "mpq-misc.h"
#include "mpq-crypto.h"

/*
 * The various string hashing function.
 */

uint32_t mpqlib_hash_filename(const char * str) {
    return __mpqlib_hash_cstring(str, 0);
}

uint32_t mpqlib_hashA_filename(const char * str) {
    return __mpqlib_hash_cstring(str, 1);
}

uint32_t mpqlib_hashB_filename(const char * str) {
    return __mpqlib_hash_cstring(str, 2);
}
