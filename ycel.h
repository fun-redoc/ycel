#ifndef __YCEL_H__
#define __YCEL_H__

#include "string_buffer_view.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_STRING_BUFFER_SIZE 1
#define INITIAL_TABLE_SIZE 1
#define MAX_OPER_NAME_SIZE 100
#define STR_BUF_SIZE 1024

typedef struct {
    size_t len; 
    size_t last;
    char   cs[STR_BUF_SIZE];
} TCharBuffer;
void clearCharBuffer(TCharBuffer *buffer);

typedef enum { TypeRef, TypeNum, TypeString, TypeCompound, TypeMinus, TypeSum, TypeParam } ENodeType;

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


typedef enum {
    SUCCESS,
    ERR
} EResult;

typedef enum {
    TKN_CRLF=1,  // must start wit 1 for the loop
    TKN_SEP,
    TKN_NUM,
    TKN_STRING,
    TKN_FORMULA
} EToken;
static char *token_names[] = {"","CRLF","SEP","NUM","STRING", "FORMULA"};

typedef enum {
    KIND_EMPTY,
    KIND_NUM,
    KIND_TEXT,
    KIND_FORMULA,
    KIND_NODE
} EKindOfCell;
static char *kind_names[] = {"EMPTY","NUM","TEXT","FORMULA", "NODE"};

typedef union {
    double number;
    TStringView swText;
    TStringView swFormula;
    const TNode *node;
} UCell;

typedef struct {
    size_t row;
    size_t col;
    EKindOfCell kind;
    UCell as;
} TCell;


typedef struct {
    // heap of cells while parsing
    size_t rows;
    size_t cols;
    size_t max;
    size_t last;
    TCell *cells;
} TCellHeap;

typedef struct {
    size_t rows;
    size_t cols;
    TCell *cells;
} TTable;

TCellHeap *init_cell_heap();
void free_cell_heap(TCellHeap *t);
void dump_cell_heap(FILE *f, TCellHeap *t);
EResult update_cell_into_table(TCellHeap *t, size_t row, size_t col, TCell cell);
EResult update_num_into_table(TCellHeap *t, size_t row, size_t col, double num);
EResult update_text_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw);
EResult update_node_into_table(TCellHeap *t, size_t row, size_t col, const TNode *nd);
EResult update_formula_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw);
TCell *find_cell_in_table(TCellHeap *t, size_t row, size_t col);

void dump_tree_preorder(TNode *head, FILE *f);
void dump_tree_postorder(TNode *head, FILE *f);
#endif