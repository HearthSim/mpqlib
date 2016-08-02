#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lookupa.h"
#include "hashtab.h"
#include "mpq-fs.h"
#include "mpq-misc.h"
#include "mpq-errors.h"
#include "errors.h"
#include "stalloc.h"

#define MAX_FNAME 2048

#define INITIAL_HASH_SIZE 10

typedef struct hash_entry_t {
    struct mpq_archive_t * mpq_a;
    int entry;
} hash_entry;

static char * strnormalize(char * _str) {
    char * str = _str;
    while (*str) {
        *str = toupper(*str);
        if (*str == '/')
            *str = '\\';
        str++;
    }
    return _str;
}

htab * hash_table = 0;
struct st_alloc_t * st = 0;

static void add_file(struct mpq_archive_t * mpq_a, char * fname) {
    int entry;
    char * nfname;

    if ((entry = mpqlib_find_hash_entry_by_name(mpq_a, fname, 0, 0)) < 0)
        return;
    
    if (!hash_table) {
        hash_table = hcreate(INITIAL_HASH_SIZE);
        st = st_init();
    }

    nfname = strnormalize(st_strdup(st, fname));
    hadd(hash_table, (uint8_t *) nfname, strlen(nfname), NULL);
    if (!hstuff(hash_table)) {
        hstuff(hash_table) = st_alloc(st, sizeof(hash_entry));
    }
    ((hash_entry *) hstuff(hash_table))->mpq_a = mpq_a;
    ((hash_entry *) hstuff(hash_table))->entry = entry;
}

/*
 * Adds an opened archive file into the system, and try to automagically import the list file.
 */
void mpqlib_fs_add_archive(struct mpq_archive_t * mpq_a) {
    struct mpq_file_t * listfile;
    char * buffer;
    int s;
    
    if (!(listfile = mpqlib_open_filename(mpq_a, "(listfile)")))
        return;
    
    s = mpqlib_seek(listfile, 0, MPQLIB_SEEK_END);
    mpqlib_seek(listfile, 0, MPQLIB_SEEK_SET);

    if (!(buffer = (char *) malloc(s + 1))) {
        mpqlib_close(listfile);
        return;
    }
    
    buffer[s] = 0;
    mpqlib_read(listfile, buffer, s);
    mpqlib_fs_attach_listfile(mpq_a, buffer);
    
    free(buffer);
    mpqlib_close(listfile);
}

/*
 * Generalistic function to add an archive to the directory structure using a custom listfile.
 */
void mpqlib_fs_attach_listfile(struct mpq_archive_t * mpq_a, const char * listfile) {
    char fname[MAX_FNAME];
    const char * p;
    char * fnp;
    
    for (p = listfile, fnp = fname; *p; p++) {
        switch (*p) {
        /* Each entry in the listfile may be separated by CR, LF, and/or ';'. */
        case '\r':
        case '\n':
        case ';':
            *fnp = 0;
            if (fnp != fname)
                add_file(mpq_a, fname);
            fnp = fname;
            break;
        default:
            *(fnp++) = *p;
            break;
        }
    }
}

/*
 * Recursively find a file.
 */
static hash_entry * find_file(char * fname) {
    if (!hash_table)
        return NULL;
    
    if (!hfind(hash_table, (uint8_t *) fname, strlen(fname)))
        return NULL;
    
    return (hash_entry *) hstuff(hash_table);
}

struct mpq_file_t * mpqlib_fs_open(const char * _fname) {
    char * fname = strnormalize(strdup(_fname));
    hash_entry * entry;
    
    entry = find_file(fname);
    __mpqlib_errno = MPQLIB_ERROR_NO_ERROR;
    
    free(fname);

    if (entry) {
        return mpqlib_open_file(entry->mpq_a, entry->entry);
    }

    __mpqlib_errno = MPQLIB_ERROR_FILE_NOT_FOUND;
    
    return NULL;
}

#define FALSE 0
#define TRUE !FALSE

int mpqlib_fs_filelist(char * buffer) {
    int r_size = 0, l;
    char * p = buffer;

    if (!hash_table)
        return 0;

    if (hfirst(hash_table)) do {
        l = strlen((char *)hkey(hash_table));
        r_size += l + 1;
        if (p) {
            strcpy(p, (char *)hkey(hash_table));
            p += l;
            *(p++) = '\n';
        }
    } while (hnext(hash_table));
    
    if (p)
        *p = 0;

    return r_size;
}

void mpqlib_fs_shutdown() {
    if (!hash_table)
        return;

    st_destroy(st);
    hdestroy(hash_table);
    hash_table = 0;
}
