#include <stdio.h>
#include <stdlib.h>
#include "mpq-bios.h"
#include "mpq-file.h"
#include "mpq-fs.h"
#include "mpq-errors.h"

int main(int argc, char ** argv) {
    struct mpq_archive_t * t1;
    char * fname = "test.mpq";
    struct mpq_file_t * f1;

    mpqlib_init();

    if (argc == 2)
        fname = argv[1];

    t1 = mpqlib_open_archive(fname);
    if (!t1) {
        printf("Open error: %s.\n", mpqlib_error());
        perror("Details");
        exit(-1);
    }

    mpqlib_printtables(t1);

    f1 = mpqlib_open_filename(t1, "(listfile)");

    if (f1) {
        int size;
        char * b;
        printf("Found (listfile), trying to read.\n");
        size = mpqlib_seek(f1, 0, MPQLIB_SEEK_END);
        mpqlib_seek(f1, 0, MPQLIB_SEEK_SET);
        printf("Filesize seems to be: %d.\n", size);
        b = (char *) malloc(size + 1);
        b[size] = 0;
        mpqlib_read(f1, b, size);
        printf("Dumping:\n");
        printf("%s", b);
        printf("\nDone.\n");
        mpqlib_close(f1);
    }

    mpqlib_fs_add_archive(t1);
    f1 = mpqlib_fs_open("Interface\\FrameXML\\WorldMapFrame.lua");

    if (f1) {
        int size;
        char * b;
        printf("Found!\n");
        size = mpqlib_seek(f1, 0, MPQLIB_SEEK_END);
        mpqlib_seek(f1, 0, MPQLIB_SEEK_SET);
        printf("Filesize seems to be: %d.\n", size);
        b = (char *) malloc(size + 1);
        b[size] = 0;
        mpqlib_read(f1, b, size);
        printf("Dumping:\n");
        printf("%s", b);
        printf("\nDone.\n");
        mpqlib_close(f1);
    }

    mpqlib_close_archive(t1);

    return 0;
}
