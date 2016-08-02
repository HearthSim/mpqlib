#include <stdlib.h>
#include <memory.h>
#include <zlib.h>
#include "mpq-bios.h"
#include "mpq-file.h"
#include "mpq-errors.h"
#include "errors.h"

#define MPQ_BUFSIZ 16384

struct mpq_file_t {
    struct mpq_archive_t * mpq_a;
    int entry;
    int number_of_sectors;
    int remaining_bytes;
    int current_sector;
    uint32_t cursor;
    uint64_t offset;
    uint32_t file_size;
    uint32_t sector_size;

    char * buffer;
    uint64_t * offsets;
    uint32_t * sizes;
};

struct mpq_file_t * mpqlib_open_file(struct mpq_archive_t * mpq_a, int entry) {
    struct mpq_file_t * r;
    uint32_t o;
    uint32_t flags;
    int i;
    
    __mpqlib_errno = MPQLIB_ERROR_NO_ERROR;
    
    /* Basic checks of the entry. */
    
    if (mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_ENTRY_EXISTS, entry)) {
        __mpqlib_errno = MPQLIB_ERROR_FILE_NOT_FOUND;
        return NULL;
    }
    
    flags = mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_GET_FLAGS, entry);
    
    if (!(flags & MPQ_FLAGS_ISFILE)) {
        __mpqlib_errno = MPQLIB_ERROR_NOT_A_FILE;
        return NULL;
    }
    
    if (flags & MPQ_FLAGS_IMPLODED) {
        __mpqlib_errno = MPQLIB_ERROR_NOT_SUPPORTED;
        return NULL;
    }
    
    // We won't support anything else than compressed files yet.
    if (!(flags & MPQ_FLAGS_COMPRESSED)) {
        __mpqlib_errno = MPQLIB_ERROR_NOT_COMPRESSED;
        return NULL;
    }
    
    if (!(r = (struct mpq_file_t *) malloc(sizeof(struct mpq_file_t)))) {
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        return NULL;
    }
    
    r->sector_size = mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_GET_SECTOR_SIZE);
    
    /* Allocating and filling up the mpq_file_t structure. */
    if (!(r->buffer = malloc(r->sector_size))) {
        free(r);
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        return NULL;
    }
    
    r->entry = entry;
    r->mpq_a = mpq_a;
    r->file_size = mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_GET_FILE_SIZE, entry);
    r->offset = mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_GET_BLOCK_OFFSET, entry);
    r->number_of_sectors = r->file_size / r->sector_size;
    if ((r->remaining_bytes = (r->file_size % r->sector_size))) {
        r->number_of_sectors++;
    } else {
        r->remaining_bytes = r->sector_size;
    }
    r->current_sector = -1;
    r->cursor = 0;
    
    if (!(r->offsets = (uint64_t *) malloc((r->number_of_sectors + 1) * sizeof(uint64_t)))) {
        free(r->buffer);
        free(r);
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        return NULL;
    }
    
    if (!(r->sizes = (uint32_t *) malloc(r->number_of_sectors * sizeof(uint32_t)))) {
        free(r->offsets);
        free(r->buffer);
        free(r);
        __mpqlib_errno = MPQLIB_ERROR_MEMORY;
        return NULL;
    }
    
    /* Reading the offset table and building the size table */
    mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_SEEK, r->offset);
    for (i = 0; i <= r->number_of_sectors; i++) {
        mpqlib_ioctl(mpq_a, MPQLIB_IOCTL_READ, &o, 4);
        r->offsets[i] = r->offset + o;
    }
    
    for (i = 0; i < r->number_of_sectors; i++) {
        r->sizes[i] = r->offsets[i + 1] - r->offsets[i];
    }
    
    return r;
}

struct mpq_file_t * mpqlib_open_filename(struct mpq_archive_t * mpq_a, const char * fname) {
    int e;
    
    __mpqlib_errno = MPQLIB_ERROR_NO_ERROR;

    if ((e = mpqlib_find_hash_entry_by_name(mpq_a, fname, 0, 0)) < 0) {
        __mpqlib_errno = MPQLIB_ERROR_FILE_NOT_FOUND;
        return NULL;
    }

    return mpqlib_open_file(mpq_a, e);
}

void mpqlib_close(struct mpq_file_t * mpq_f) {
    free(mpq_f->buffer);
    free(mpq_f->offsets);
    free(mpq_f->sizes);
    free(mpq_f);
}

static int cache_sector(struct mpq_file_t * mpq_f, int sector) {
    static char in_buf[MPQ_BUFSIZ];
    int r = 1;
    
    /* current_sector should be initialized to -1 */
    if (mpq_f->current_sector != sector) {
        int il, ol;
        mpqlib_ioctl(mpq_f->mpq_a, MPQLIB_IOCTL_SEEK, mpq_f->offsets[sector]);
        mpqlib_ioctl(mpq_f->mpq_a, MPQLIB_IOCTL_READ, in_buf, mpq_f->sizes[sector]);
        mpq_f->current_sector = sector;
        
        il = ol = sector == (mpq_f->number_of_sectors - 1) ? mpq_f->remaining_bytes : mpq_f->sector_size;
        
        if (ol == mpq_f->sizes[sector]) {
            memcpy(mpq_f->buffer, in_buf, ol);
        } else {
            char * cursor = in_buf;
            r = 0;
            if ((*cursor++) == 2) {
                z_stream z;
                z.next_in = (Bytef *) cursor;
                z.avail_in = mpq_f->sizes[sector];
                z.total_in = mpq_f->sizes[sector];
                z.next_out = (Bytef *) mpq_f->buffer;
                z.avail_out = ol;
                z.total_out = 0;
                z.zalloc = NULL;
                z.zfree = NULL;
                if (inflateInit(&z) == 0) {
                    r = inflate(&z, Z_FINISH) >= 0 ? 1 : 0;
                    ol = z.total_out;
                    inflateEnd(&z);
                }
            }
        }
        
        if (il != ol)
            r = 0;
    }
    
    return r;
}

/*
 * The read function will iterate several times, in order to split the calls across the sectors.
 */

uint32_t mpqlib_read(struct mpq_file_t * mpq_f, void * _buffer, uint32_t size) {
    int return_size = 0;

    while (1) {
        char * buffer = (char *) _buffer;
        int sector_begin, sector_end;
        uint32_t cl_size;
        uint32_t offset_begin, offset_end;
    
        uint32_t first_sector_begins;
        uint32_t last_sector_ends;

        __mpqlib_errno = MPQLIB_ERROR_NO_ERROR;
        
        /* Computing various cursors and stuff. */
        if ((size + mpq_f->cursor) >= mpq_f->file_size)
                size = mpq_f->file_size - mpq_f->cursor;

        offset_begin = mpq_f->cursor;
        sector_begin = offset_begin / mpq_f->sector_size;
        first_sector_begins = offset_begin % mpq_f->sector_size;
    
        offset_end = size + mpq_f->cursor;
        sector_end = offset_end / mpq_f->sector_size;
        last_sector_ends = offset_end % mpq_f->sector_size;
    
        if (!(offset_end % mpq_f->sector_size)) {
            sector_end--;
            last_sector_ends = mpq_f->sector_size;
        }
    
        /* Let's ask for the first sector */
        if (!cache_sector(mpq_f, sector_begin)) {
            __mpqlib_errno = MPQLIB_ERROR_COMPRESSION;
            return 0;
        }
    
        /* If we've hit the end sector, let's just copy that over and exit. */
        if (sector_begin == sector_end) {
                memcpy(buffer, mpq_f->buffer + first_sector_begins, size);
            mpq_f->cursor += size;
            return_size += size;
            break;
        }
    
        /* Else, let's compute the cluster size, copy that chunk, and reiterate. */
        cl_size = mpq_f->sector_size - first_sector_begins;
        memcpy(buffer, mpq_f->buffer + first_sector_begins, cl_size);
        mpq_f->cursor += cl_size;
        
        return_size += cl_size;
        _buffer = buffer + cl_size;
        size  = size - cl_size;
    }
    
    return return_size;
}

uint32_t mpqlib_seek(struct mpq_file_t * mpq_f, signed long int offset, enum mpqlib_file_seek_t wheel) {
    switch (wheel) {
    case MPQLIB_SEEK_SET:
        if (offset < 0)
            offset = 0;
        mpq_f->cursor = offset;
        break;
    case MPQLIB_SEEK_CUR:
        if ((mpq_f->cursor + offset) < 0)
            offset = -mpq_f->cursor;
        mpq_f->cursor += offset;
        break;
    case MPQLIB_SEEK_END:
        if (offset > 0)
            offset = 0;
        if ((mpq_f->file_size + offset) < 0)
            offset = -mpq_f->cursor;
        mpq_f->cursor = mpq_f->file_size + offset;
        break;
    default:
        __mpqlib_errno = MPQLIB_ERROR_UNKNOWN;
        return 0xffffffff;
    }
    if (mpq_f->cursor >= mpq_f->file_size)
        mpq_f->cursor = mpq_f->file_size;
    return mpq_f->cursor;
}
