#ifndef JX_WRITE_H
#define JX_WRITE_H

#include <stdbool.h>

unsigned jx_put_bool(char buf[], bool val);
unsigned jx_put_int(char buf[], long val);
unsigned jx_put_null(char buf[]);
unsigned jx_put_str(char buf[], char const val[]);

unsigned jx_put_object_open(char buf[]);
unsigned jx_put_object_close(char buf[]);

unsigned jx_put_array_open(char buf[]);
unsigned jx_put_array_close(char buf[]);

unsigned jx_put_comma(char buf[]);
unsigned jx_put_colon(char buf[]);

#endif
