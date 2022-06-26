#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "maybe.h"
#include "string_buffer_view.h"

TStringBuffer *alloc_string_buffer(size_t initialSize)
{
    TStringBuffer *sb = malloc(sizeof(TStringBuffer));
    init_string_buffer(sb, initialSize);
    return sb;
}

void init_string_buffer(TStringBuffer *sb, size_t initialSize)
{
    sb->max = initialSize+1;
    sb->last = 0;
    sb->buffer = calloc(sb->max, sizeof(char));
}

TStringView append_string_buffer(TStringBuffer *sb, const char *str)
{
    assert(sb);
    assert(str);
    size_t len = strlen(str);
    size_t start = sb->last;
    if(sb->last + len >= sb->max)
    {
        sb->max = (sb->last + len) * 2 + 1 ;
        sb->buffer = realloc(sb->buffer, sizeof(char)*(sb->max));
        if(!sb->buffer) goto error;
    }

    strcpy(sb->buffer + sb->last, str);
    sb->last += len+1;

    TStringView sw;
    sw.sb = sb;
    sw.start = start;
    NOTHING(size_t,sw.len);
    return sw;
    error:
        fprintf(stderr, "Failed to alloc buffer in %s %d\n", __FILE__, __LINE__);
        exit(1);
}
void clear_string_buffer(TStringBuffer *sb)
{
    free(sb->buffer);
    sb->last = 0;
    sb->max = 0;
}

char *string_at(TStringBuffer *sb, size_t start)
{
    return (sb->buffer + start);
}

char *substr_dup_at(TStringBuffer *sb, size_t start, size_t len)
{
    char *aux = strndup(sb->buffer + start, len);
    return aux;
}

char *get_string(const TStringView *sw)
{
    assert(IS_NOTHING2(sw->len));
    return string_at(sw->sb, sw->start);
}

char *dup_substr(TStringView *sw)
{
    if(IS_NOTHING2(sw->len)) 
    {
        return strdup(string_at(sw->sb, sw->start));
    }
    else
    {
        return substr_dup_at(sw->sb, sw->start, MAYBE_VALUE_ACCESS(sw->len));
    }
}
