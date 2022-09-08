#include "jr_cursor.h"

/* meld-cut-here */
extern void jr_cursor_init(struct jr_cursor *cursor, int length, char *json)
{
    cursor->length = length;
    cursor->json = json;
    cursor->pos = 0;
}
/* meld-cut-here */
