#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "string_buffer_view.h"
#include "ycel_misc.h"
#include "ycel_parser.h"
#include "ycel_table.h"
#include "maybe.h"

//************ general ************

void clearCharBuffer(TCharBuffer *buffer)
{
    assert(buffer);
    buffer->len = sizeof(buffer->cs)/sizeof(char);
    memset(buffer->cs, '\0', buffer->len);
    buffer->last = 0;
}

bool charBufferEmpty(const TCharBuffer *buffer)
{
    return buffer->len > buffer->last;
}

void level_prefix(TCharBuffer *buffer, int level)
{
    for(int i=0; i<=level && buffer->last < buffer->len; i++)
    {
        buffer->cs[buffer->last] = '.';
        buffer->last++;
    }
}

//MAYBE_FAIL(size_t);

void test_string_buffer() {
    TStringBuffer sb;
    init_string_buffer(&sb, INITIAL_STRING_BUFFER_SIZE);
    append_string_buffer(&sb, "hello");
    append_string_buffer(&sb, "my");
    append_string_buffer(&sb, "world!");

    TStringView sw1;
    sw1.sb = &sb;
    sw1.start = 0;
    //SOME(size_t, sw1.len , 5);
    NOTHING(size_t, sw1.len);
    TStringView sw2;
    sw2.sb = &sb;
    sw2.start = 6;
    //SOME(size_t,sw2.len,2);
    NOTHING(size_t, sw2.len);
    TStringView sw3;
    sw3.sb = &sb;
    sw3.start = 9;
    //SOME(size_t, sw3.len,4);
    NOTHING(size_t, sw3.len);

    char *aux = get_string(&sw1);
    fprintf(stdout, "YCEL: %s\n", aux);
    aux = get_string(&sw2);
    fprintf(stdout, "YCEL: %s\n", aux);
    aux = get_string(&sw3);
    fprintf(stdout, "YCEL: %s\n", get_string(&sw3));
}


