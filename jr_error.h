#ifndef JR_ERROR_H
#define JR_ERROR_H

/* meld-cut-here */
#define JR_ERROR_MAP(X)                                                        \
    X(OK, "no error")                                                          \
    X(INVAL, "invalid value")                                                  \
    X(NOMEM, "not enough memory")                                              \
    X(OUTRANGE, "out-of-range")                                                \
    X(NOTFOUND, "not found")

enum jr_error
{
#define X(A, _) JR_##A,
    JR_ERROR_MAP(X)
#undef X
};
/* meld-cut-here */

#endif
