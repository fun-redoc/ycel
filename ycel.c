#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "string_buffer_view.h"
#include "ycel.h"
#include "maybe.h"

// use with error lable in scope
#define DO(proc) if(ERR==proc) goto error 

#define INITIAL_STRING_BUFFER_SIZE 1
#define INITIAL_TABLE_SIZE 1

#define MIN(x,y) (x<y?x:y)
#define MAX(x,y) (x<y?y:x)

extern int yylex();
extern char* yytext;
extern FILE* yyin;

int row=0;
int col=0;
char *string_val;
double num_val;

TCellHeap *init_cell_heap() {
    TCellHeap *t = malloc(sizeof(TCellHeap));
    if(!t) {
        fprintf(stderr, "cannot alloc table in %s %d.\n", __FILE__, __LINE__);
        goto error;
    }
    t->rows = 0;
    t->cols = 0;
    t->last = 0;
    t->max = INITIAL_TABLE_SIZE;
    t->cells = malloc(sizeof(TCell)*t->max);
    return t;
error:
    free(t);
    exit(1);
}
 void free_cell_heap(TCellHeap *t) {
     if(t) {
         if(t->cells){
            free(t->cells);
         }
         free(t);
     }
 }

 void dump_cell_heap(FILE *f, TCellHeap *t) {
     if(t) {
         fprintf(f,"Dump Cell Heap\n");
         fprintf(f,"max=%zu, last=%zu, rows=%zu, cols=%zu\n", t->max, t->last, t->rows, t->cols);
         if(t->cells){
            for(int i=0; i<t->last;i++) {
                TCell *c = &(t->cells[i]);
                fprintf(f,"Cell (%zu,%zu) of Kind %s; Value=", c->row, c->col, kind_names[c->kind]);
                // TODO Refactor using free_cell
                switch (c->kind)
                {
                    case KIND_NUM:
                        fprintf(f,"%.2f",c->as.number);
                        break;
                    case KIND_TEXT:
                        fprintf(f, "%s", get_string(&c->as.swText));
                        break;
                    case KIND_FORMULA:
                        fprintf(f, "%s", get_string(&c->as.swFormula));
                        break;
                    default:
                        break;
                }
                fprintf(f, "\n");
            }
         }
         else
         {
             fprintf(f, "no cells\n");
         }

     }
     else
     {
         fprintf(f, "Cell Heap is nil!\n");
     }
 }

void fill_num_cell(TCell *c, size_t row, size_t col, double num)
{
    c->row = row;
    c->col = col;
    c->kind = KIND_NUM;
    c->as.number = num;
}

void fill_text_cell(TCell *c, size_t row, size_t col, const TStringView *sw)
{
    c->row = row;
    c->col = col;
    c->kind = KIND_TEXT;
    c->as.swText = *sw;
}

void fill_formula_cell(TCell *c, size_t row, size_t col, const TStringView *sw)
{
    c->row = row;
    c->col = col;
    c->kind = KIND_FORMULA;
    c->as.swFormula = *sw;
}

void fill_empty_cell(TCell *c, size_t row, size_t col, const char *str)
{
    c->row = row;
    c->col = col;
    c->kind = KIND_EMPTY;
}

EResult insert_cell_into_table(TCellHeap *t, size_t row, size_t col, TCell cell)
{
    assert(t);
    assert(t->cells);
    if(t->max <= t->last)
    {
        t->cells = realloc(t->cells, sizeof(TCell)*t->max*2);
        if(!t) return ERR;
        t->max *= 2;
    }
    t->cells[t->last] = cell;
    TCell aux = t->cells[t->last];
    t->last++;
    t->rows = MAX(t->rows, row);
    t->cols = MAX(t->cols, col);
    return SUCCESS;
}

EResult insert_num_into_table(TCellHeap *t, size_t row, size_t col, double num) {
    TCell cell; 
    fill_num_cell(&cell, row,col,num);
    return insert_cell_into_table(t, row, col, cell);
}
EResult insert_text_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw) {
    TCell cell;
    fill_text_cell(&cell, row,col,sw);
    return insert_cell_into_table(t, row, col, cell);
}
EResult insert_formula_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw) {
    TCell cell; 
    fill_formula_cell(&cell, row,col,sw);
    return insert_cell_into_table(t, row, col, cell);
}

void table_from_cell_heap(TCell *table, TCellHeap *t)
{
    for(int i=0; i<t->last; i++)
    {
        TCell c = t->cells[i];
        table[c.row*t->rows+c.col] = c;
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
    fprintf(stdout, "%s\n", aux);
    aux = get_string(&sw2);
    fprintf(stdout, "%s\n", aux);
    aux = get_string(&sw3);
    fprintf(stdout, "%s\n", get_string(&sw3));
}

int main(int argc, char *argv[])
{
   TStringView sw;
   TStringBuffer sb;
   init_string_buffer(&sb, INITIAL_STRING_BUFFER_SIZE);

   TCellHeap *t = init_cell_heap();
   EToken tkn;

    if(argc == 2)
    {
        //yyin defaults to stdin
        yyin = fopen(argv[1], "r");
    }

   tkn = yylex();
   while(tkn) {
    switch (tkn)
    {
        case TKN_SEP:
            col++;
            break;
        case TKN_CRLF:
            row++;
            col=0;
            break;
        case TKN_NUM:
            printf("(%d,%d): %d %s %.2f\n", row, col, tkn, token_names[tkn], num_val);
            DO(insert_num_into_table(t,row,col, num_val));
            break;
        case TKN_STRING:
            printf("(%d,%d): %d %s %s\n", row, col, tkn, token_names[tkn], string_val);
            //printf("(%d,%d): %d %s\n", row, col, tkn, string_val);
            sw = append_string_buffer(&sb,string_val);
            DO(insert_text_into_table(t,row,col, &sw));
            break;
        case TKN_FORMULA:
            printf("(%d,%d): %d %s %s\n", row, col, tkn, token_names[tkn], string_val);
            //printf("(%d,%d): %d %s\n", row, col, tkn, string_val);
            sw = append_string_buffer(&sb,string_val);
            DO(insert_formula_into_table(t,row,col,&sw));
            break;
        default:
            break;
    }
    tkn = yylex();
   }  

   dump_cell_heap(stdout,t);

   TCell *table = malloc(sizeof(TCell)*t->rows*t->cols);

   table_from_cell_heap(table,t);

   free_cell_heap(t);

   return 0;
error:
    free_cell_heap(t);
    exit(1);
}