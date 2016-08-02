#define _LARGEFILE64_SOURCE

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <sys/types.h>
#define lseek64 _lseeki64
#define O_RDONLY _O_RDONLY
#define O_BINARY _O_BINARY
#define O_LARGEFILE 0
#else
#include <unistd.h>
#endif

#ifdef __APPLE__
#define lseek64 lseek
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mpq-crypto.h"
#include "mpq-bios.h"
#include "mpq-errors.h"
#include "mpq-misc.h"
#include "errors.h"

#ifndef MAX
#define MAX(x,y) (((x)<(y))?(y):(x))
#endif

/*
 * MPQ header.
 */
#ifdef _MSC_VER
#pragma pack(1)
#endif
typedef struct {
    /* basic version of the header. */
    char magic[4];
    uint32_t header_size;
    uint32_t archive_size;
    uint16_t format_version;
    uint16_t sector_size;
    
    uint32_t hash_table_offset;
    uint32_t block_table_offset;
    uint32_t hash_table_entries;
    uint32_t block_table_entries;
    
    /* extended version of header - Burning Crusade. */    
    uint64_t extended_block_table_offset;
    uint16_t hash_table_offset_high;
    uint16_t block_table_offset_high;
}
#ifndef _MSC_VER
__attribute__ ((packed))
#endif
mpq_header_t;

/*
 * One hash entry.
 */

typedef struct {
    uint32_t file_path_hasha;
    uint32_t file_path_hashb;
    uint16_t language;
    uint16_t platform;
    uint32_t file_block_index;
}
#ifndef _MSC_VER
__attribute__ ((packed))
#endif
mpq_hash_t;

/*
 * One block entry.
 */

typedef struct {
    uint32_t block_offset;
    uint32_t block_size;
    uint32_t file_size;
    uint32_t flags;
}
#ifndef _MSC_VER
__attribute__ ((packed))
#endif
mpq_block_t;


/*
 * This are our internal structures.
 */

typedef struct {
    uint32_t file_path_hasha;
    uint32_t file_path_hashb;
    uint16_t language;
    uint16_t platform;
    uint32_t file_block_index;
} hash_t;

typedef struct {
    uint64_t block_offset;
    uint32_t block_size;
    uint32_t file_size;
    uint32_t flags;
} block_t;

struct mpq_archive_t {
    int fd;
    int closeit;
    int for_write;
    
    uint64_t last_block;
    
    uint32_t header_size;
    uint32_t archive_size;
    uint16_t format_version;
    uint32_t sector_size;

    uint64_t hash_table_offset;
    uint64_t block_table_offset;
    uint32_t hash_table_entries;
    uint32_t block_table_entries;
    
    uint64_t extended_block_table_offset;
    
    hash_t * hashs;
    block_t * blocks;
};

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

struct mpq_archive_t * mpqlib_open_archive(const char * fname) {
    int fd;
    struct mpq_archive_t * r;
    
    if ((fd = open(fname, O_RDONLY | O_LARGEFILE | O_BINARY)) == -1) {
        __mpqlib_errno = MPQLIB_ERROR_OPEN;
        return NULL;
    }
    
    r = mpqlib_reopen_archive(fd);
    if (r) {
        r->closeit = 1;
        r->for_write = 0;
    } else {
        close(fd);
    }
    
    return r;
}

struct mpq_archive_t * mpqlib_open_archive_for_writing(const char * fname) {
    int fd;
    struct mpq_archive_t * r;
    
    if ((fd = open(fname, O_RDWR | O_LARGEFILE | O_BINARY)) == -1) {
        __mpqlib_errno = MPQLIB_ERROR_OPEN;
        return NULL;
    }
    
    r = mpqlib_reopen_archive(fd);
    if (r) {
        r->closeit = 1;
        r->for_write = 1;
    } else {
        close(fd);
    }
    
    return r;
}

#define STD_HEADER_SIZE 0x20
#define EXT_HEADER_SIZE 0x0C

#define no_mpq() { \
    __mpqlib_errno = MPQLIB_ERROR_NOT_MPQ_ARCHIVE; \
    free(mpq_a); \
    return NULL; \
}

#define read_error() { \
    __mpqlib_errno = MPQLIB_ERROR_READ; \
    free_archive(mpq_a); \
    free(mpq_hashs); \
    free(mpq_blocks); \
    return NULL; \
}

static void free_archive(struct mpq_archive_t * mpq_a) {
    free(mpq_a->hashs);
    free(mpq_a->blocks);
    free(mpq_a);
}

static int read_data(struct mpq_archive_t * mpq_a, void * buf, size_t l) {
    if (read(mpq_a->fd, buf, l) != l)
        return 0;
    return 1;
}

static int write_data(struct mpq_archive_t * mpq_a, const void * buf, size_t l) {
    if (write(mpq_a->fd, buf, l) != l)
        return 0;
    return 1;
}

void mpqlib_init() {
    __mpqlib_init_cryptography();
}

void mpqlib_close_archive(struct mpq_archive_t * mpq_a) {
    if (mpq_a) {
        if (mpq_a->closeit) {
            close(mpq_a->fd);
        }
        free_archive(mpq_a);
    }
}

struct mpq_archive_t * mpqlib_reopen_archive(int fd) {
    struct mpq_archive_t * mpq_a;
    mpq_header_t mpq_h;
    mpq_hash_t * mpq_hashs;
    mpq_block_t * mpq_blocks;
    uint16_t * mpq_extblocks = NULL;
    int i;
    uint64_t last_block = 0;
#ifndef WIN32
    int flags;
#endif
    
    __mpqlib_errno = MPQLIB_ERROR_NO_ERROR;
    
    /****TODO****/
    /* Implement endianess a bit everywhere */
    
    if (!(mpq_a = (struct mpq_archive_t *) malloc(sizeof(struct mpq_archive_t)))) {
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        return NULL;
    }
    
    mpq_a->closeit = 0;
    mpq_a->fd = fd;
    mpq_a->for_write = 0;
#ifndef WIN32
    flags = fcntl(fd, F_GETFL);
    if ((flags != -1) && (flags & O_RDWR)) {
        mpq_a->for_write = 1;
    }
#endif
    
    /* Reading the main header, and doing basic checks. */
    if (read(fd, &mpq_h, STD_HEADER_SIZE) != STD_HEADER_SIZE)
        no_mpq();
    
    if (strncmp(mpq_h.magic, "MPQ\x1a", 4))
        no_mpq();

    /* format_version can only be 0 or 1 - the latter beeing for Burning Crusade */
    if ((mpq_h.format_version | 1) != 1)
        no_mpq();
    
    /* If the format is extended, let's read the extra data, otherwise let's fill it with zeroes */
    if (mpq_h.format_version == 1) {
        if (!read_data(mpq_a, ((char *)(&mpq_h)) + STD_HEADER_SIZE, EXT_HEADER_SIZE))
            no_mpq();
    } else {
        mpq_h.extended_block_table_offset = 0;
        mpq_h.hash_table_offset_high = 0;
        mpq_h.block_table_offset_high = 0;
    }

    /* Let's start copying / interpreting / rebuilding basic header data */
    mpq_a->header_size = mpq_h.header_size;
    mpq_a->archive_size = mpq_h.archive_size;
    mpq_a->format_version = mpq_h.format_version;
    mpq_a->sector_size = 0x200 << mpq_h.sector_size;
    
    mpq_a->hash_table_offset = (uint64_t) mpq_h.hash_table_offset + (((uint64_t) mpq_h.hash_table_offset_high) << 32);
    mpq_a->block_table_offset = (uint64_t) mpq_h.block_table_offset + (((uint64_t) mpq_h.block_table_offset_high) << 32);
    mpq_a->hash_table_entries = mpq_h.hash_table_entries;
    mpq_a->block_table_entries = mpq_h.block_table_entries;
    mpq_a->extended_block_table_offset = mpq_h.extended_block_table_offset;
    
    /* Allocation and reading of our hash and block tables */    
    if (!(mpq_a->hashs = (hash_t *) malloc(mpq_a->hash_table_entries * sizeof(hash_t)))) {
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        free(mpq_a);
        return NULL;
    }
    
    if (!(mpq_a->blocks = (block_t *) malloc(mpq_a->block_table_entries * sizeof(block_t)))) {
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        free(mpq_a->hashs);
        free(mpq_a);
        return NULL;
    }
    
    if (!(mpq_hashs = (mpq_hash_t *) malloc(mpq_a->hash_table_entries * sizeof(mpq_hash_t)))) {
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        free_archive(mpq_a);
        return NULL;
    }
    
    if (!(mpq_blocks = (mpq_block_t *) malloc(mpq_a->block_table_entries * sizeof(mpq_block_t)))) {
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        free(mpq_hashs);
        free_archive(mpq_a);
        return NULL;
    }
    
    if (lseek64(fd, mpq_a->hash_table_offset, SEEK_SET) != mpq_a->hash_table_offset)
        read_error();
    if (!read_data(mpq_a, mpq_hashs, mpq_a->hash_table_entries * sizeof(mpq_hash_t)))
        read_error();

    if (lseek64(fd, mpq_a->block_table_offset, SEEK_SET) != mpq_a->block_table_offset)
        read_error();
    if (!read_data(mpq_a, mpq_blocks, mpq_a->block_table_entries * sizeof(mpq_block_t)))
        read_error();
    
    if (mpq_a->extended_block_table_offset) {
        if (lseek64(fd, mpq_a->block_table_offset, SEEK_SET))
            read_error();
        
        if (!(mpq_extblocks = (uint16_t *) malloc(mpq_a->block_table_entries * sizeof(uint16_t)))) {
            __mpqlib_errno = MPQLIB_ERROR_MEMORY;
            free_archive(mpq_a);
            free(mpq_hashs);
            free(mpq_blocks);
            return NULL;
        }
        
        if (!read_data(mpq_a, mpq_blocks, mpq_a->block_table_entries * sizeof(mpq_block_t))) {
            free(mpq_extblocks);
            read_error();
        }
    }
    
    /* Hash and block table are (easily) encrypted - let's decrypt them before going on any further */
    __mpqlib_decrypt(mpq_hashs, mpq_a->hash_table_entries * sizeof(mpq_block_t), __mpqlib_hash_cstring("(hash table)", 3), 1);
    __mpqlib_decrypt(mpq_blocks, mpq_a->block_table_entries * sizeof(mpq_block_t), __mpqlib_hash_cstring("(block table)", 3), 1);
    
    /* Copying / interpreting / rebuilding of the hash table. */
    for (i = 0; i < mpq_a->hash_table_entries; i++) {
        /****TODO****/
        /* Implement various checks of the hash table. */
        mpq_a->hashs[i].file_path_hasha = mpq_hashs[i].file_path_hasha;
        mpq_a->hashs[i].file_path_hashb = mpq_hashs[i].file_path_hashb;
        mpq_a->hashs[i].language = mpq_hashs[i].language;
        mpq_a->hashs[i].platform = mpq_hashs[i].platform;
        mpq_a->hashs[i].file_block_index = mpq_hashs[i].file_block_index;
    }
    
    /* Copying / interpreting / rebuilding of the block table. */
    for (i = 0; i < mpq_a->block_table_entries; i++) {
        /****TODO****/
        /* Implement various checks of the block table. */
        mpq_a->blocks[i].block_offset = mpq_blocks[i].block_offset + (mpq_extblocks ? (((uint64_t) mpq_extblocks[i]) << 32): 0);
        mpq_a->blocks[i].block_size = mpq_blocks[i].block_size;
        mpq_a->blocks[i].file_size = mpq_blocks[i].file_size;
        mpq_a->blocks[i].flags = mpq_blocks[i].flags;
        last_block = MAX(last_block, mpq_a->blocks[i].block_offset + mpq_a->blocks[i].block_size);
    }
    
    mpq_a->last_block = last_block;
    
    /* All done, let's clean up and exit. */
    if (mpq_extblocks)
        free(mpq_extblocks);
    
    free(mpq_blocks);
    free(mpq_hashs);
    
    return mpq_a;
}

/*
 * Verbose display of an archive.
 */

void mpqlib_printtables(struct mpq_archive_t * mpq_a) {
    int i;
    
    printf("Archive details:\n");
    printf("Header size:    %10u\n", mpq_a->header_size);
    printf("Archive size:   %10u\n", mpq_a->archive_size);
    printf("Format version: %10u\n", mpq_a->format_version);
    printf("Sector size:    %10u\n\n", mpq_a->sector_size);
    
    printf("Hash table offset:   %016llX\n", mpq_a->hash_table_offset);
    printf("Block table offset:  %016llX\n", mpq_a->block_table_offset);
    printf("Hash table entries:  %8u\n", mpq_a->hash_table_entries);
    printf("Block table entries: %8u\n", mpq_a->block_table_entries);

    printf("Extended block table offset:  %016llX\n\n\n", mpq_a->extended_block_table_offset);

    printf("Hash table dump.\n");
    printf("HashA      HashB      language   platform   index\n");
    printf("--------   --------   --------   --------   -----\n");
    for (i = 0; i < mpq_a->hash_table_entries; i++) {
        printf("%08X   %08X   %08X   %08X   %d\n", mpq_a->hashs[i].file_path_hasha, mpq_a->hashs[i].file_path_hashb, mpq_a->hashs[i].language, mpq_a->hashs[i].platform, mpq_a->hashs[i].file_block_index);
    }

    printf("\n\nBlock table dump.\n");
    printf("Entry      Block offset       size       filesize    flags\n");
    printf("--------   ----------------   --------   --------   --------\n");
    for (i = 0; i < mpq_a->block_table_entries; i++) {
        printf("%8d   %016llX   %8d   %8d   %08X\n", i, mpq_a->blocks[i].block_offset, mpq_a->blocks[i].block_size, mpq_a->blocks[i].file_size, mpq_a->blocks[i].flags);
    }
}

static hash_t * mpqlib_locate_hash_entry(struct mpq_archive_t * mpq_a, uint32_t h, uint32_t hA, uint32_t hB, uint32_t language, uint32_t platform) {
    uint32_t i;

    /* The hash table is a true one, pre-built. We can access it as-it, speeding up the searches drastically. */
    for (i = h & (mpq_a->hash_table_entries - 1); i < mpq_a->hash_table_entries; i++) {
        if ((mpq_a->hashs[i].file_path_hasha == hA) &&
            (mpq_a->hashs[i].file_path_hashb == hB) &&
            (mpq_a->hashs[i].language == language) &&
            (mpq_a->hashs[i].platform == platform)) {
            return mpq_a->hashs + i;
        }
        if (mpq_a->hashs[i].file_block_index == 0xffffffff)
            return NULL;
    }
    
    return NULL;
}

int mpqlib_add_hash_entry_by_hash(struct mpq_archive_t * mpq_a, uint32_t h, uint32_t hA, uint32_t hB, uint16_t language, uint16_t platform, int entry) {
    uint32_t i;

    if (!mpq_a->for_write) {
        __mpqlib_errno = MPQLIB_ERROR_READONLY;
        return -1;
    }

    /* The hash table is a true one, pre-built. We can access it as-it, speeding up the searches drastically. */
    for (i = h & (mpq_a->hash_table_entries - 1); i < mpq_a->hash_table_entries; i++) {
        if ((mpq_a->hashs[i].file_path_hasha == hA) &&
            (mpq_a->hashs[i].file_path_hashb == hB) &&
            (mpq_a->hashs[i].language == language) &&
            (mpq_a->hashs[i].platform == platform)) {
            /* Full collision ?! */
            __mpqlib_errno = MPQLIB_ERROR_UNKNOWN;
            return -1;
        }
        if (mpq_a->hashs[i].file_block_index == 0xffffffff) {
            mpq_a->hashs[i].file_path_hasha = hA;
            mpq_a->hashs[i].file_path_hashb = hB;
            mpq_a->hashs[i].language = language;
            mpq_a->hashs[i].platform = platform;
            mpq_a->hashs[i].file_block_index = entry;
            return 0;
        }
    }
    
    __mpqlib_errno = MPQLIB_ERROR_TOO_MANY_FILES;
    return -1;
}

int mpqlib_add_hash_entry_by_name(struct mpq_archive_t * mpq_a, const char * name, uint16_t language, uint16_t platform, int entry) {
    uint32_t h, hA, hB;
    
    if (!mpq_a->for_write) {
        __mpqlib_errno = MPQLIB_ERROR_READONLY;
        return -1;
    }

    h = mpqlib_hash_filename(name);
    hA = mpqlib_hashA_filename(name);
    hB = mpqlib_hashB_filename(name);
    
    return mpqlib_add_hash_entry_by_hash(mpq_a, h, hA, hB, language, platform, entry);
}

int mpqlib_update_hash_entry_by_hash(struct mpq_archive_t * mpq_a, uint32_t h, uint32_t hA, uint32_t hB, uint16_t language, uint16_t platform, int entry) {
    hash_t * hash = mpqlib_locate_hash_entry(mpq_a, h, hA, hB, language, platform);
    
    if (!mpq_a->for_write) {
        __mpqlib_errno = MPQLIB_ERROR_READONLY;
        return -1;
    }
    
    if (hash) {
        hash->file_block_index = entry;
        return 0;
    }
    return -1;
}

int mpqlib_update_hash_entry_by_name(struct mpq_archive_t * mpq_a, const char * name, uint16_t language, uint16_t platform, int entry) {
    uint32_t h, hA, hB;
    
    h = mpqlib_hash_filename(name);
    hA = mpqlib_hashA_filename(name);
    hB = mpqlib_hashB_filename(name);
    
    return mpqlib_update_hash_entry_by_hash(mpq_a, h, hA, hB, language, platform, entry);
}

int mpqlib_find_hash_entry_by_hash(struct mpq_archive_t * mpq_a, uint32_t h, uint32_t hA, uint32_t hB, uint16_t language, uint16_t platform) {
    hash_t * hash = mpqlib_locate_hash_entry(mpq_a, h, hA, hB, language, platform);
    
    if (hash) {
        return hash->file_block_index;
    }
    return -1;
}

int mpqlib_find_hash_entry_by_name(struct mpq_archive_t * mpq_a, const char * name, uint16_t language, uint16_t platform) {
    uint32_t h, hA, hB;
    
    h = mpqlib_hash_filename(name);
    hA = mpqlib_hashA_filename(name);
    hB = mpqlib_hashB_filename(name);
    
    return mpqlib_find_hash_entry_by_hash(mpq_a, h, hA, hB, language, platform);
}

uint64_t mpqlib_ioctl(struct mpq_archive_t * mpq_a, enum mpqlib_ioctl_t command, ...) {
    uint64_t r = 0;
    uint32_t u32;
    uint64_t u64;
    int entry;
    void * buffer;
    va_list ap;
    va_start(ap, command);

    __mpqlib_errno = MPQLIB_ERROR_NO_ERROR;

    switch(command) {
    case MPQLIB_IOCTL_NO_ACTION:
        break;
    case MPQLIB_IOCTL_SEEK:
        u64 = va_arg(ap, uint64_t);
        if (lseek64(mpq_a->fd, u64, SEEK_SET) != u64) {
            r = -1;
            __mpqlib_errno = MPQLIB_ERROR_SEEK;
        }
        break;
    case MPQLIB_IOCTL_READ:
        buffer = va_arg(ap, void *);
        u32 = va_arg(ap, uint32_t);
        r = read_data(mpq_a, buffer, u32) ? 0 : -1;
        if (r == -1)
            __mpqlib_errno = MPQLIB_ERROR_READ;
        break;
    case MPQLIB_IOCTL_WRITE:
        if (!mpq_a->for_write) {
            __mpqlib_errno = MPQLIB_ERROR_READONLY;
            r = -1;
            break;
        }
        buffer = va_arg(ap, void *);
        u32 = va_arg(ap, uint32_t);
        r = write_data(mpq_a, buffer, u32) ? 0 : -1;
        if (r == -1)
            __mpqlib_errno = MPQLIB_ERROR_WRITE;
        break;
    case MPQLIB_IOCTL_GET_FORMAT_VERSION:
        r = mpq_a->format_version;
        break;
    case MPQLIB_IOCTL_GET_SECTOR_SIZE:
        r = mpq_a->sector_size;
        break;
    case MPQLIB_IOCTL_SET_BLOCK_OFFSET:
    case MPQLIB_IOCTL_SET_BLOCK_SIZE:
    case MPQLIB_IOCTL_SET_FILE_SIZE:
    case MPQLIB_IOCTL_SET_FLAGS:
        if (!mpq_a->for_write) {
            __mpqlib_errno = MPQLIB_ERROR_READONLY;
            r = -1;
            break;
        }
    case MPQLIB_IOCTL_ENTRY_EXISTS:
    case MPQLIB_IOCTL_GET_BLOCK_OFFSET:
    case MPQLIB_IOCTL_GET_BLOCK_SIZE:
    case MPQLIB_IOCTL_GET_FILE_SIZE:
    case MPQLIB_IOCTL_GET_FLAGS:
        entry = va_arg(ap, int);
        if ((entry >= mpq_a->block_table_entries) || (entry < 0)) {
            __mpqlib_errno = MPQLIB_ERROR_IOCTL_INVALID_ENTRY;
            r = -1;
            break;
        }
        switch (command) {
	case MPQLIB_IOCTL_ENTRY_EXISTS:
	    break;
        case MPQLIB_IOCTL_GET_BLOCK_OFFSET:
            r =  mpq_a->blocks[entry].block_offset;
            break;
        case MPQLIB_IOCTL_GET_BLOCK_SIZE:
            r = mpq_a->blocks[entry].block_size;
            break;
        case MPQLIB_IOCTL_GET_FILE_SIZE:
            r = mpq_a->blocks[entry].file_size;
            break;
        case MPQLIB_IOCTL_GET_FLAGS:
            r = mpq_a->blocks[entry].flags;
            break;
        case MPQLIB_IOCTL_SET_BLOCK_OFFSET:
            u64 = va_arg(ap, uint64_t);
            mpq_a->blocks[entry].block_offset = u64;
            break;
        case MPQLIB_IOCTL_SET_BLOCK_SIZE:
            u32 = va_arg(ap, uint32_t);
            mpq_a->blocks[entry].block_size = u32;
            break;
        case MPQLIB_IOCTL_SET_FILE_SIZE:
            u32 = va_arg(ap, uint32_t);
            mpq_a->blocks[entry].file_size = u32;
            break;
        case MPQLIB_IOCTL_SET_FLAGS:
            u32 = va_arg(ap, uint32_t);
            mpq_a->blocks[entry].flags = u32;
            break;
        default:
            // should never get there.
            __mpqlib_errno = MPQLIB_ERROR_UNKNOWN;
            r = -1;
            break;
        }
        break;
    default:
        __mpqlib_errno = MPQLIB_ERROR_INVALID_IOCTL;
        r = -1;
        break;
    }
    
    va_end(ap);
    return r;
}
