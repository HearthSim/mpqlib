#include "mpq-errors.h"
#include "errors.h"

static const char * error_strings[] = {
    "No error.",
    "Unknown error - internal error.",
    "Error opening file.",
    "File specified isn't an MPQ archive.",
    "Not enough memory.",
    "File seek error."
    "File read error.",
    "File write error."
    "Invalid ioctl.",
    "Invalid ioctl's file entry.",
    "File not found.",
    "Entry isn't a file.",
    "Imploded compression not supported yet.",
    "File isn't flagged as compressed - not supported yet.",
    "Compression error.",
    "MPQ archive in read-only mode.",
    "Too many files.",
};

static const char * wrong_errno = "Invalid error number - internal error.";

int mpqlib_errno() {
    return __mpqlib_errno;
}

const char * mpqlib_error() {
    if ((__mpqlib_errno < 0) || (__mpqlib_errno >= MPQLIB_ERRORS_MAX))
        return wrong_errno;
    return error_strings[__mpqlib_errno];
}
