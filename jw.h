#ifndef JW_H
#define JW_H

/* meld-cut-here */
#include <stdbool.h>

unsigned jw_bool(char buf[], bool x);
unsigned jw_long(char buf[], long x);
unsigned jw_null(char buf[]);
unsigned jw_str(char buf[], char const x[]);

unsigned jw_object_open(char buf[]);
unsigned jw_object_close(char buf[]);

unsigned jw_array_open(char buf[]);
unsigned jw_array_close(char buf[]);

unsigned jw_comma(char buf[]);
unsigned jw_colon(char buf[]);
/* meld-cut-here */

#endif
