#ifndef __MPQ_BIOS_H__
#define __MPQ_BIOS_H__

#include "mpqlib-stdint.h"

struct mpq_archive_t;

#define MPQ_FLAGS_ISFILE      0x80000000
#define MPQ_FLAGS_HAS_META    0x04000000
#define MPQ_FLAGS_UNIT        0x01000000
#define MPQ_FLAGS_ADJUST      0x00020000
#define MPQ_FLAGS_ENCRYPTED   0x00010000
#define MPQ_FLAGS_COMPRESSED  0x00000200
#define MPQ_FLAGS_IMPLODED    0x00000100

enum mpqlib_ioctl_t {
    MPQLIB_IOCTL_NO_ACTION = 0,
    MPQLIB_IOCTL_SEEK,
    MPQLIB_IOCTL_READ,
    MPQLIB_IOCTL_WRITE,
    MPQLIB_IOCTL_ENTRY_EXISTS,
    MPQLIB_IOCTL_GET_FORMAT_VERSION,
    MPQLIB_IOCTL_GET_SECTOR_SIZE,
    MPQLIB_IOCTL_GET_BLOCK_OFFSET,
    MPQLIB_IOCTL_GET_BLOCK_SIZE,
    MPQLIB_IOCTL_GET_FILE_SIZE,
    MPQLIB_IOCTL_GET_FLAGS,
    MPQLIB_IOCTL_SET_BLOCK_OFFSET,
    MPQLIB_IOCTL_SET_BLOCK_SIZE,
    MPQLIB_IOCTL_SET_FILE_SIZE,
    MPQLIB_IOCTL_SET_FLAGS,
};

#ifdef __cplusplus
extern "C" {
#endif

void mpqlib_init();
struct mpq_archive_t * mpqlib_open_archive(const char * fname);
struct mpq_archive_t * mpqlib_open_archive_for_writing(const char * fname);
struct mpq_archive_t * mpqlib_reopen_archive(int fd);
void mpqlib_printtables(struct mpq_archive_t *);
void mpqlib_close_archive(struct mpq_archive_t *);
int mpqlib_update_hash_entry_by_hash(struct mpq_archive_t * mpq_a, uint32_t h, uint32_t hA, uint32_t hB, uint16_t language, uint16_t platform, int entry);
int mpqlib_update_hash_entry_by_name(struct mpq_archive_t * mpq_a, const char * name, uint16_t language, uint16_t platform, int entry);
int mpqlib_add_hash_entry_by_hash(struct mpq_archive_t *, uint32_t h, uint32_t hA, uint32_t hB, uint16_t language, uint16_t, int entry);
int mpqlib_add_hash_entry_by_name(struct mpq_archive_t *, const char * name, uint16_t language, uint16_t platform, int entry);
int mpqlib_find_hash_entry_by_hash(struct mpq_archive_t *, uint32_t h, uint32_t hA, uint32_t hB, uint16_t language, uint16_t platform);
int mpqlib_find_hash_entry_by_name(struct mpq_archive_t *, const char * name, uint16_t language, uint16_t platform);
uint64_t mpqlib_ioctl(struct mpq_archive_t *, enum mpqlib_ioctl_t command, ...);

#ifdef __cplusplus
}
#endif

#endif
