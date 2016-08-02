#ifndef __MPQ_FILE_H__
#define __MPQ_FILE_H__

#include "mpqlib-stdint.h"
struct mpq_file_t;

enum mpqlib_file_seek_t {
    MPQLIB_SEEK_SET,
    MPQLIB_SEEK_CUR,
    MPQLIB_SEEK_END,
};

#ifdef __cplusplus
extern "C" {
#endif

struct mpq_file_t * mpqlib_open_file(struct mpq_archive_t * mpq_a, int entry);
struct mpq_file_t * mpqlib_open_filename(struct mpq_archive_t * mpq_a, const char * fname);
void mpqlib_close(struct mpq_file_t * mpq_f);
uint32_t mpqlib_read(struct mpq_file_t * mpq_f, void * buffer, uint32_t size);
uint32_t mpqlib_seek(struct mpq_file_t * mpq_f, signed long int offset, enum mpqlib_file_seek_t);

#ifdef __cplusplus
}
#endif

#endif
