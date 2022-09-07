#ifndef JX_CURSOR_H
#define JX_CURSOR_H

struct jx_cursor
{
    int length;
    char *json;
    int pos;
};

void jx_cursor_init(struct jx_cursor *cursor, char *json);

#endif
