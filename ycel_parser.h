#ifndef __YCEL_PARSER_H__
#define __YCEL_PARSER_H__

#include "string_buffer_view.h"
#include <stdlib.h>
#include <stdio.h>

//************ Parser ************
#define MAX_OPER_NAME_SIZE 100

typedef enum { TypeRef, TypeNum, TypeString, TypeCompound, TypeNeg, 
               TypeNewLine, TypeNewCell,TypeEmpty,
               TypeSum, TypeMul, TypeAvg,
               TypePlus,TypeMinus,TypeTimes,TypeDiv,
               TypeParam } ENodeType;

/* Reference to a Cell */
typedef struct {
    int x;
    int y;
} TRef;

/* Numbers */
typedef struct {
    double value;                       /* value of constant */
} TValNum;

/* String */
typedef struct {
    TStringView value;                  /* value of constant */
} TValString;

/* operators */
typedef struct {
    int oper;                            /* operator */
    char oper_name[MAX_OPER_NAME_SIZE];  /* operator name (for Debuging) */
    int nops;                            /* number of operands */
    struct nodeType *op[1];	             /* operands, extended at runtime */
} TOpr;

typedef struct nodeType {
    TRef coord;            /* coordinates in the ycel */
    ENodeType type;        /* type of node */

    union {
        TValNum num;      /* constants */
        TValString str;   /* constants */
        TRef ref;         /* referenced coordinates */
        TOpr opr;         /* operators */
    };
} TNode;
void dump_node(TCharBuffer *buffer, TNode *nd, const int level);
TNode *gather_params2(TNode *params, size_t *n, TNode *nd);

#endif