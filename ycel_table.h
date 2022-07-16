#ifndef __YCEL_TABLE_H__
#define __YCEL_TABLE_H__

#include "string_buffer_view.h"
#include <stdlib.h>
#include <stdio.h>

//************ Table Handling ************
#define INITIAL_STRING_BUFFER_SIZE 1
#define INITIAL_TABLE_SIZE 1
typedef enum {
    SUCCESS,
    ERR
} EResult;

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
    TNode *node;
} UCell;

typedef enum { not_yet, proceeding, ready } ECellEvalSstatus;
typedef struct {
    ECellEvalSstatus evalStatus;
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

TCellHeap *init_cell_heap();
void free_cell_heap(TCellHeap *t);
void dump_cell_heap(FILE *f, TCellHeap *t);
EResult update_cell_into_table(TCellHeap *t, size_t row, size_t col, TCell cell);
EResult update_num_into_table(TCellHeap *t, size_t row, size_t col, double num);
EResult update_text_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw);
EResult update_node_into_table(TCellHeap *t, size_t row, size_t col, TNode *nd);
EResult update_formula_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw);
TCell *find_cell_in_table(TCellHeap *t, size_t row, size_t col);
void calc(TCellHeap *t);

#endif