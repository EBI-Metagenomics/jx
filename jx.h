#ifndef JX_H
#define JX_H

#include <stdbool.h>
#include <stdint.h>

struct jx;
struct jx_item;

struct jx *jx_new(unsigned nitems);
void jx_del(struct jx *jx);

int jx_parse(struct jx *jx, char *str);
unsigned jx_nitems(struct jx const *jx);
void jx_assert_nitems(struct jx *jx, unsigned nitems);
struct jx_item const *jx_root(struct jx const *jx);
int jx_errno(struct jx const *jx);

char *jx_string(struct jx const *jx, struct jx_item const *item);
int64_t jx_int64(struct jx *jx, struct jx_item const *item);

struct jx_item const *jx_next(struct jx const *jx, struct jx_item const *item);
bool jx_end(struct jx const *jx, struct jx_item const *item);
unsigned jx_nchild(struct jx const *jx, struct jx_item const *item);

bool jx_is_array(struct jx const *jx, struct jx_item const *item);
bool jx_is_bool(struct jx const *jx, struct jx_item const *item);
bool jx_is_null(struct jx const *jx, struct jx_item const *item);
bool jx_is_number(struct jx const *jx, struct jx_item const *item);
bool jx_is_object(struct jx const *jx, struct jx_item const *item);
bool jx_is_string(struct jx const *jx, struct jx_item const *item);

void jx_assert_array(struct jx *jx, struct jx_item const *item);
void jx_assert_bool(struct jx *jx, struct jx_item const *item);
void jx_assert_null(struct jx *jx, struct jx_item const *item);
void jx_assert_number(struct jx *jx, struct jx_item const *item);
void jx_assert_object(struct jx *jx, struct jx_item const *item);
void jx_assert_string(struct jx *jx, struct jx_item const *item);

#endif
