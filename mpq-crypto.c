#include <assert.h>
#include <string.h>

#include "mpq-crypto.h"

static int crypt_table_initialized = 0;
static uint32_t crypt_table[0x500];

/****TODO****/
/* Re-implement various endianess fixes. */

const uint32_t *__mpqlib_get_cryptography_table()
{
    assert(crypt_table_initialized);
    return crypt_table;
}

void __mpqlib_init_cryptography()
{
   // Prepare crypt_table
    uint32_t seed = 0x00100001;
    uint32_t index1 = 0;
    uint32_t index2 = 0;
    int32_t i;

    if (!crypt_table_initialized) {
        crypt_table_initialized = 1;

        for (index1 = 0; index1 < 0x100; index1++) {
            for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100) {
                uint32_t temp1, temp2;

                seed = (seed * 125 + 3) % 0x2AAAAB;
                temp1 = (seed & 0xFFFF) << 0x10;

                seed = (seed * 125 + 3) % 0x2AAAAB;
                temp2 = (seed & 0xFFFF);

                crypt_table[index2] = (temp1 | temp2);
            }
        }
    }
}

void __mpqlib_encrypt(void *_data, uint32_t length, uint32_t key, char disable_input_swapping)
{
    char * data = (char *) _data;
    uint32_t *buffer32 = (uint32_t *) data;
    uint32_t seed = 0xEEEEEEEE;
    uint32_t ch;
    
    assert(crypt_table_initialized);
    assert(data);

   // Round to 4 bytes
    length = length / 4;

   // We duplicate the loop to avoid costly branches
    if (disable_input_swapping) {
        while (length-- > 0) {
            seed += crypt_table[0x400 + (key & 0xFF)];
            ch = *buffer32 ^ (key + seed);

            key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
            seed = *buffer32 + seed + (seed << 5) + 3;

            *buffer32++ = ch;
        }
    } else {
        while (length-- > 0) {
            seed += crypt_table[0x400 + (key & 0xFF)];
            ch = *buffer32 ^ (key + seed);

            key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
            seed = *buffer32 + seed + (seed << 5) + 3;

            *buffer32++ = ch;
        }
    }
}

void __mpqlib_decrypt(void *_data, uint32_t length, uint32_t key, char disable_output_swapping)
{
    char * data = (char *) _data;
    uint32_t *buffer32 = (uint32_t *) data;
    uint32_t seed = 0xEEEEEEEE;
    uint32_t ch;

    assert(crypt_table_initialized);
    assert(data);

   // Round to 4 bytes
    length = length / 4;

    if (disable_output_swapping) {
        while (length-- > 0) {
            ch = *buffer32;

            seed += crypt_table[0x400 + (key & 0xFF)];
            ch = ch ^ (key + seed);

            key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
            seed = ch + seed + (seed << 5) + 3;

            *buffer32++ = ch;
        }

    } else {
        while (length-- > 0) {
            ch = *buffer32;

            seed += crypt_table[0x400 + (key & 0xFF)];
            ch = ch ^ (key + seed);

            key = ((~key << 0x15) + 0x11111111) | (key >> 0x0B);
            seed = ch + seed + (seed << 5) + 3;

            *buffer32++ = ch;
        }
    }
}

uint32_t __mpqlib_hash_cstring(const char *string, uint32_t type)
{
    uint32_t seed1 = 0x7FED7FED;
    uint32_t seed2 = 0xEEEEEEEE;
    uint32_t shifted_type = (type << 8);
    int32_t ch;

    assert(crypt_table_initialized);
    assert(string);

    while (*string != 0) {
        ch = *string++;
        if (ch > 0x60 && ch < 0x7b)
            ch -= 0x20;

        seed1 = crypt_table[shifted_type + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }

    return seed1;
}

uint32_t __mpqlib_hash_data(const char *data, uint32_t length, uint32_t type)
{
    uint32_t seed1 = 0x7FED7FED;
    uint32_t seed2 = 0xEEEEEEEE;
    uint32_t shifted_type = (type << 8);
    int32_t ch;

    assert(crypt_table_initialized);
    assert(data);

    while (length > 0) {
        ch = *data++;

        seed1 = crypt_table[shifted_type + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
        length--;
    }

    return seed1;
}
