#ifndef JR_CURSOR_H
#define JR_CURSOR_H

/* meld-cut-here */
struct jr_cursor
{
    int length;
    char *json;
    int pos;
};
/* meld-cut-here */

void __jr_cursor_init(struct jr_cursor *cursor, char *json);

#endif
