#ifndef __YCEL_H__
#define __YCEL_H__

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
    KIND_FORMULA
} EKindOfCell;
static char *kind_names[] = {"EMPTY","NUM","TEXT","FORMULA"};

typedef union {
    double number;
    char*  text;
    char*  formula;
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

#endif