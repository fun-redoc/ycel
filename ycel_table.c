#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "string_buffer_view.h"
#include "ycel_misc.h"
#include "ycel_parser.h"
#include "ycel_table.h"
#include "maybe.h"

//************ Table Handling ************

// prototypes
void calc_cell(TCellHeap *t, TCell *c);
TCellHeap *init_cell_heap() {
    TCellHeap *t = malloc(sizeof(TCellHeap));
    if(!t) {
        fprintf(stderr, "YCAL: cannot alloc table in %s %d.\n", __FILE__, __LINE__);
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
    printf("in dump cell heap\n");
     TCharBuffer buffer;
     clearCharBuffer(&buffer);
     if(t) {
         charBuffer_snprintf(&buffer, "%s","YCEL: Dump Cell Heap\n");
         charBuffer_snprintf(&buffer,"YCEL: max=%zu, last=%zu, rows=%zu, cols=%zu\n", t->max, t->last, t->rows, t->cols);

         if(t->cells){
            for(int i=0; i<t->last;i++) {
                TCell *c = &(t->cells[i]);
                charBuffer_snprintf(&buffer,"YCAL: Cell (%zu,%zu) of Kind %s; Value=", c->row, c->col, kind_names[c->kind]);

                // TODO Refactor using free_cell
                switch (c->kind)
                {
                    case KIND_NUM:
                        charBuffer_snprintf(&buffer,"%.2f",c->as.number);
                        break;
                    case KIND_TEXT:
                        charBuffer_snprintf(&buffer, "|%s|", get_string(&c->as.swText));
                        break;
                    case KIND_FORMULA:
                        charBuffer_snprintf(&buffer, "%s", get_string(&c->as.swFormula));
                        break;
                    case KIND_NODE:
                        dump_node(&buffer, c->as.node,0);
                        break;
                    default:
                        break;
                }
                charBuffer_snprintf(&buffer, "%s", "\n");
            }
         }
         else
         {
             charBuffer_snprintf(&buffer,"%s","YCEL: no cells\n");
         }

     }
     else
     {
         charBuffer_snprintf(&buffer, "%s", "YCEL: Cell Heap is nil!\n");
     }
     fprintf(f, "%s", buffer.cs);
 }
TCell *find_cell_in_table(TCellHeap *t, size_t row, size_t col)
{
    assert(t);
    for(int i=0; i<t->last; i++)
    {
        TCell *c = &(t->cells[i]);
        assert(c);
        if(c->row == row && c->col == col)
        {
            return c;
        }
    }
    return NULL;
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

void fill_node_cell(TCell *c, size_t row, size_t col, TNode *nd)
{
    c->row = row;
    c->col = col;
    c->kind = KIND_NODE;
    c->as.node = nd;
}

void fill_empty_cell(TCell *c, size_t row, size_t col, const char *str)
{
    c->row = row;
    c->col = col;
    c->kind = KIND_EMPTY;
}

EResult append_cell_to_table(TCellHeap *t, size_t row, size_t col, TCell cell)
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
    t->rows = MAX(t->rows, row+1);
    t->cols = MAX(t->cols, col+1);
    return SUCCESS;
}

EResult update_cell_into_table(TCellHeap *t, size_t row, size_t col, TCell cell)
{
    assert(t);
    assert(t->cells);
    TCell *old_cell = find_cell_in_table(t, row, col);
    if(old_cell)
    {
        *old_cell = cell;
        return SUCCESS;
    }
    else
    {
        return append_cell_to_table(t, row, col, cell);
    }
}

EResult update_num_into_table(TCellHeap *t, size_t row, size_t col, double num) {
    TCell cell; 
    fill_num_cell(&cell, row,col,num);
    return update_cell_into_table(t, row, col, cell);
}
EResult update_text_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw) {
    TCell cell;
    fill_text_cell(&cell, row,col,sw);
    return update_cell_into_table(t, row, col, cell);
}
EResult update_formula_into_table(TCellHeap *t, size_t row, size_t col, const TStringView *sw) {
    TCell cell; 
    fill_formula_cell(&cell, row,col,sw);
    return update_cell_into_table(t, row, col, cell);
}
EResult update_node_into_table(TCellHeap *t, size_t row, size_t col, TNode *nd)
{
    assert(nd);
    TCell cell; 
    fill_node_cell(&cell, row,col,nd);
    return update_cell_into_table(t, row, col, cell);
}

void table_from_cell_heap(TCell *table, TCellHeap *t)
{
    for(int i=0; i<t->last; i++)
    {
        TCell c = t->cells[i];
        table[c.row*t->rows+c.col] = c;
    }
}



double calc_node(TCellHeap *t, const TNode *nd)
{
    assert(nd);
    switch(nd->type)
    {
        case TypeNum:
        {
            return nd->num.value;
        }
        break;
        case TypeString:
        {
            assert(NULL &&"Type Error, string is not expected here.");
        }
        break;
        case TypeRef:
        {
            TCell *c = find_cell_in_table(t, nd->ref.y-1, nd->ref.x-1);
            assert(c && "Referenced cell not found.");
            calc_cell(t, c);
            assert(c->kind == TypeNum);
            return c->as.number;
        }
        break;
        case TypeNeg:
        {
            assert(nd->opr.nops == 1);
            double res = 0;
            res =-calc_node(t, nd->opr.op[0]);
            return res;
        }
        break;
        case TypeParam:
        {
            assert(NULL && "not yet implemented");
        }
        break;
        case TypeNewLine:
        case TypeNewCell:
        {
            assert(false && "should not happen");
        }
        case TypeCompound:
        {
            assert(nd->opr.nops == 1);
            return calc_node(t, nd->opr.op[0]);
        }
        break;
        case TypeSum:
        {
            assert(nd->opr.nops == 1);
            TNode* params = NULL;
            size_t n = 0;
            params = gather_params2(params, &n, nd->opr.op[0]);
            double res = 0;
            for(int i = 0; i<n; i++)
            {
                res +=calc_node(t, &params[i]);
            }
            free(params);
            return res;
        }
        break;
        case TypeMul:
        {
            assert(nd->opr.nops == 1);
            TNode* params = NULL;
            size_t n = 0;
            params = gather_params2(params, &n, nd->opr.op[0]);
            double res = 1;
            for(int i = 0; i<n; i++)
            {
                res *=calc_node(t, &params[i]);
            }
            free(params);
            return res;
        }
        break;
        case TypeAvg:
        {
            assert(nd->opr.nops == 1);
            TNode* params = NULL;
            size_t n = 0;
            params = gather_params2(params, &n, nd->opr.op[0]);
            double res = 0;
            for(int i = 0; i<n; i++)
            {
                res += calc_node(t, &params[i]);
            }
            free(params);
            return res/((double)n);
        }
        break;
        case TypePlus:
                assert(nd->opr.nops == 2);
                return calc_node(t, nd->opr.op[0]) + calc_node(t,nd->opr.op[1]);
            break;
        case TypeMinus:
                assert(nd->opr.nops == 2);
                return calc_node(t, nd->opr.op[0]) - calc_node(t,nd->opr.op[1]);
            break;
        case TypeTimes:
                assert(nd->opr.nops == 2);
                return calc_node(t, nd->opr.op[0]) * calc_node(t,nd->opr.op[1]);
            break;
        case TypeDiv:
                assert(nd->opr.nops == 2);
                return calc_node(t, nd->opr.op[0]) / calc_node(t,nd->opr.op[1]);
            break;
    }
}

void calc_cell(TCellHeap *t, TCell *c)
{
    assert(((c->evalStatus == not_yet) || (c->evalStatus == ready)) && "there is a circular reference.");
    c->evalStatus = proceeding;
    switch (c->kind)
    {
        case KIND_EMPTY:
        case KIND_NUM:
        case KIND_TEXT:
            c->evalStatus = ready;
        break;
        case KIND_FORMULA: assert(NULL && "obsolete"); break;
        case KIND_NODE:
        {
            const TNode *nd = c->as.node;
            double res = calc_node(t, nd);
            c->as.number = res;
            c->kind = KIND_NUM;
            c->evalStatus = ready;
        }
        break;
    }   
}
void calc_cell_by_idx(TCellHeap *t, size_t i)
{
    assert(t);
    assert(t->cells);
    calc_cell(t, &(t->cells[i]));
}

void calc(TCellHeap *t)
{
    assert(t);
    // initialize loop detection 
    for(int i=0; i<t->last; i++)
    {
        t->cells[i].evalStatus = not_yet;
    }
    // calculate 
    for(int i=0; i<t->last; i++)
    {
        calc_cell_by_idx(t, i);
    }
}

// transforms tree to an appropriate structure for a table and
// adjusts the row,col numbers. I wasnt able to manage row,col
// during parsing
void tree_to_table(TCellHeap *t, TNode *nd, int row, int col)
{
   switch(nd->type)
   {
       case TypeNum:
       {
        nd->coord = (TRef){col, row};
        TRY(update_num_into_table(t, row, col, nd->num.value));
       }
       break;
       case TypeString:
       {
        nd->coord = (TRef){col, row};
        TRY(update_text_into_table(t, row, col, &(nd->str.value)));
       }
       break;
       case TypeRef:
       {
       }
       break;
       case TypeNeg:
       {
        nd->coord = (TRef){col, row};
        TRY(update_node_into_table(t, row, col, nd));
       }
       break;
       case TypeParam:
       break;
       case TypeCompound:
       {
        for(int i=0; i < nd->opr.nops; i++)
        {
            tree_to_table(t, nd->opr.op[i],row,col);
        }
       }
       break;
       case TypeNewLine:
       {
        assert(nd->opr.nops == 2);
        tree_to_table(t, nd->opr.op[0],row,col);
        tree_to_table(t, nd->opr.op[1],row+1,0);
       }
       break;
       case TypeNewCell:
       {
        assert(nd->opr.nops == 2);
        nd->coord = (TRef){col, row};
        tree_to_table(t, nd->opr.op[0],row,col);
        tree_to_table(t, nd->opr.op[1],row,col+1);
       }
       break;
       case TypeAvg:
       case TypeMul:
       case TypeSum:
       {
          assert(nd->opr.nops == 1);
          nd->coord = (TRef){col, row};
          TRY(update_node_into_table(t, row, col, nd));
       }
       break;
       case TypePlus:
       case TypeMinus:
       case TypeTimes:
       case TypeDiv:
       {
          assert(nd->opr.nops == 2);
          nd->coord = (TRef){col, row};
          TRY(update_node_into_table(t, row, col, nd));
       }
       break;
   }
   return;
   error:
    assert(false && "error happened.");
}

int comp_cell_ref (const void * cell1, const void * cell2) 
{
    TCell c1 = *((TCell*)cell1);
    TCell c2 = *((TCell*)cell2);
    if(c1.row < c2.row) return  1;
    if(c1.row == c2.row && c1.col < c2.col) return 1;
    if(c1.row == c2.row && c1.col > c2.col) return -1;
    if(c1.row > c2.row) return  -1;
    return 0;
}

void table_out(FILE *f, TCellHeap *ch, const char colsep, const char *rowsep)
{
    assert(ch);
    assert(f);
    // 1. sort by cell reference (row, col), it maybe is not guarantied, that the parse result is in correct order
    qsort (ch->cells, sizeof(ch->cells)/sizeof(TCell*), sizeof(TCell*), comp_cell_ref); 
    //dump_cell_heap(stdout, ch);
    // 2. write csv.
    if(ch->last > 0)
    {
        size_t row = ch->cells[0].row;
        size_t col = ch->cells[0].col;
        for(int i=0; i < ch->last; i++)
        {
            assert(   ch->cells[i].evalStatus == ready 
                   && (   ch->cells[i].kind == KIND_EMPTY 
                       || ch->cells[i].kind == KIND_NUM 
                       || ch->cells[i].kind == KIND_TEXT
                      )
                  );

            if( row < ch->cells[i].row) fprintf(f,"%s",rowsep);
            if( col < ch->cells[i].col) fprintf(f,"%c",colsep);
            if(ch->cells[i].kind == KIND_TEXT) fprintf(f,"%s",get_string(&(ch->cells[i].as.swText)));
            if(ch->cells[i].kind == KIND_NUM) fprintf(f, "%f", ch->cells[i].as.number);
            row = ch->cells[i].row;
            col = ch->cells[i].col;
        }
    }
    fprintf(f,"\n");
    fflush(f);
    return;
}