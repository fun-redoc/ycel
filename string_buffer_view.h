#ifndef __STRING_BUFFER_VIEW_H__
#define __STRING_BUFFER_VIEW_H__

#include <stdbool.h>
#include "maybe.h"

typedef struct {
    size_t last;
    size_t max;
    char *buffer;
} TStringBuffer;

MAYBE_TYPE(size_t);
//MAYBE_FAIL(size_t);
typedef struct {
    size_t start;
    MAYBE(size_t) len;
    TStringBuffer *sb;
} TStringView;

TStringBuffer *alloc_string_buffer(size_t initial_size);
void init_string_buffer(TStringBuffer *sb, size_t initialSize);
TStringView append_string_buffer(TStringBuffer *sb, const char *str);
void clear_string_buffer(TStringBuffer *sb);
char *substr_dup_at(TStringBuffer *sb, size_t start, size_t len);
char *string_at(TStringBuffer *sb, size_t start);


char *get_string(const TStringView *sw);

char *dup_substr(TStringView *sw);

#endif