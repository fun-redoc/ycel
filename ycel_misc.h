#ifndef __YCEL_MISC_H__
#define __YCEL_MISC_H__

#include "string_buffer_view.h"
#include <stdlib.h>
#include <stdio.h>

//************ general ************
// use with error lable in scope
#define TRY(proc) if(ERR==proc) goto error 

#define charBuffer_snprintf(b,f,...) {\
    int res238279238479287;\
    res238279238479287=snprintf(&((b)->cs[(b)->last]),(((b)->len)-((b)->last)),f,__VA_ARGS__);\
    assert(res238279238479287>=0);\
    (b)->last+=res238279238479287;\
    assert(((b)->len)>((b)->last));}

#define MIN(x,y) (x<y?x:y)
#define MAX(x,y) (x<y?y:x)

#define STR_BUF_SIZE 1024
typedef struct {
    size_t len; 
    size_t last;
    char   cs[9*STR_BUF_SIZE];
} TCharBuffer;
void clearCharBuffer(TCharBuffer *buffer);
bool charBufferEmpty(const TCharBuffer *buffer);
void level_prefix(TCharBuffer *buffer, int level);

#endif