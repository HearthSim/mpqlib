#ifndef __MPQ_ERRORS_H__
#define __MPQ_ERRORS_H__

#ifdef __cplusplus
extern "C" {
#endif

int mpqlib_errno();
const char * mpqlib_error();

#ifdef __cplusplus
}
#endif

enum {
    MPQLIB_ERROR_NO_ERROR = 0,
    MPQLIB_ERROR_UNKNOWN,
    MPQLIB_ERROR_OPEN,
    MPQLIB_ERROR_NOT_MPQ_ARCHIVE,
    MPQLIB_ERROR_MEMORY,
    MPQLIB_ERROR_SEEK,
    MPQLIB_ERROR_READ,
    MPQLIB_ERROR_WRITE,
    MPQLIB_ERROR_INVALID_IOCTL,
    MPQLIB_ERROR_IOCTL_INVALID_ENTRY,
    MPQLIB_ERROR_FILE_NOT_FOUND,
    MPQLIB_ERROR_NOT_A_FILE,
    MPQLIB_ERROR_NOT_SUPPORTED,
    MPQLIB_ERROR_NOT_COMPRESSED,
    MPQLIB_ERROR_COMPRESSION,
    MPQLIB_ERROR_READONLY,
    MPQLIB_ERROR_TOO_MANY_FILES,

    MPQLIB_ERRORS_MAX
};

#endif
