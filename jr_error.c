#include "jr_error.h"
#include "jr.h"

/* meld-cut-here */
static char const *error_strings[] = {
#define X(_, A) A,
    JR_ERROR_MAP(X)
#undef X
};

char const *jr_strerror(int code)
{
    if (code < 0 || code >= (int)__JR_ARRAY_SIZE(error_strings))
        return "unknown error";
    return error_strings[code];
}
/* meld-cut-here */
