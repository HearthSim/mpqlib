#include "mpqlib-stdint.h"

#if defined(__cplusplus)
extern "C" {
#endif

    void __mpqlib_init_cryptography(void);

    const uint32_t *__mpqlib_get_cryptography_table(void);

    void __mpqlib_encrypt(void *data, uint32_t length, uint32_t key,
                                 char disable_input_swapping);
    void __mpqlib_decrypt(void *data, uint32_t length, uint32_t key,
                                 char disable_output_swapping);

    uint32_t __mpqlib_hash_cstring(const char *string, uint32_t type);
    uint32_t __mpqlib_hash_data(const char *data, uint32_t length, uint32_t type);

#if defined(__cplusplus)
}
#endif
