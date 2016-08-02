#ifndef __MPQ_FS_H__
#define __MPQ_FS_H__

#include "mpq-bios.h"
#include "mpq-file.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpqlib_fs_add_archive(struct mpq_archive_t *);
void mpqlib_fs_attach_listfile(struct mpq_archive_t *, const char *);

struct mpq_file_t * mpqlib_fs_open(const char *);

int mpqlib_fs_filelist(char * buffer);

void mpqlib_fs_shutdown();

#ifdef __cplusplus
}
#endif

#endif
