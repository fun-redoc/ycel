#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "string_buffer_view.h"
#include "ycel.h"
#include "maybe.h"

// use with error lable in scope
#define DO(proc) if(ERR==proc) goto error 

#define charBuffer_snprintf(b,f,...) {\
    int res238279238479287;\
    res238279238479287=snprintf(&((b)->cs[(b)->last]),(((b)->len)-((b)->last)),f,__VA_ARGS__);\
    assert(res238279238479287>=0);\
    (b)->last+=res238279238479287;\
    assert(((b)->len)>((b)->last));}

#define MIN(x,y) (x<y?x:y)
#define MAX(x,y) (x<y?y:x)


extern int yylex();
extern char* yytext;
extern FILE* yyin;

int row=0;
int col=0;
char *string_val;
double num_val;

// prototypes
void dump_node(TCharBuffer *buffer, const TNode *nd, const int level);
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

void fill_node_cell(TCell *c, size_t row, size_t col, const TNode *nd)
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
    t->rows = MAX(t->rows, row);
    t->cols = MAX(t->cols, col);
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
EResult update_node_into_table(TCellHeap *t, size_t row, size_t col, const TNode *nd)
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

int main_no_more(int argc, char *argv[])
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
            printf("YCEL: (%d,%d): %d %s %.2f\n", row, col, tkn, token_names[tkn], num_val);
            DO(update_num_into_table(t,row,col, num_val));
            break;
        case TKN_STRING:
            printf("YCEL: (%d,%d): %d %s %s\n", row, col, tkn, token_names[tkn], string_val);
            //printf("(%d,%d): %d %s\n", row, col, tkn, string_val);
            sw = append_string_buffer(&sb,string_val);
            DO(update_text_into_table(t,row,col, &sw));
            break;
        case TKN_FORMULA:
            printf("YCEL: (%d,%d): %d %s %s\n", row, col, tkn, token_names[tkn], string_val);
            //printf("(%d,%d): %d %s\n", row, col, tkn, string_val);
            sw = append_string_buffer(&sb,string_val);
            DO(update_formula_into_table(t,row,col,&sw));
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

void level_prefix(TCharBuffer *buffer, int level)
{
    for(int i=0; i<=level && buffer->last < buffer->len; i++)
    {
        buffer->cs[buffer->last] = '.';
        buffer->last++;
    }
}

TNode *gather_params2(TNode *params, size_t *n, TNode *nd)
{
    assert(nd->opr.oper == ';' || nd->opr.oper == ':');
    char sep = nd->opr.oper;
    
    // make place for at least o new param

    if(sep == ':') 
    {

        TNode *head = nd->opr.op[0];
        TNode *tail = nd->opr.op[1];
        assert(head->type == TypeRef && tail->type == TypeRef); // only defined for references
        int from_x, from_y, to_x, to_y;
        if(head->ref.x < tail->ref.x)
        {
            from_x = head->ref.x;
            to_x = tail->ref.x;
        }
        else
        {
            to_x = head->ref.x;
            from_x = tail->ref.x;
        }
        if(head->ref.y < tail->ref.y)
        {
            from_y = head->ref.y;
            to_y = tail->ref.y;
        }
        else
        {
            to_y = tail->ref.y;
            from_y = head->ref.y;
        }
        
        for(int x=from_x; x<=to_x; x++)
        {
            for(int y=from_y; y<=to_y; y++)
            {
                if(!params)
                {
                    params = malloc(sizeof(TNode));
                    *n = 1;
                }
                else
                {
                    (*n)++;
                    params = realloc(params, ((*n)+1)*sizeof(TNode));
                }
                params[*n-1] = (TNode){head->coord, TypeRef, .ref=((TRef){x,y})};
            }
        }
        return params; 

    } else if(sep ==';')
    {
        if(!params)
        {
            params = malloc(sizeof(TNode));
            *n = 1;
        }
        else
        {
            (*n)++;
            params = realloc(params, ((*n)+1)*sizeof(TNode));
        }
        // TODO: expression list grows to the left, tail to head (look expr_list rule), TODO turn it the other way round
        memcpy(&(params[*n-1]), nd->opr.op[1], sizeof(TNode));
        // TAIL Recursion turn it into loop
        if(nd->opr.op[0]->opr.oper == sep)
        {
            return gather_params2(params,n, nd->opr.op[0]);
        }
        else
        {
            (*n)++;
            params = realloc(params, (*n)*sizeof(TNode));
            memcpy(&(params[*n-1]), nd->opr.op[0], sizeof(TNode));
            return params;
        }
    } else
    {
        assert(false && "unknown separator.");
        return params;
    }
}

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

void dump_node(TCharBuffer *buffer, const TNode *nd, const int level)
{
   level_prefix(buffer, level);

   switch(nd->type)
   {
       case TypeNum:
       {
          charBuffer_snprintf(buffer, "Num=%.2f", nd->num.value);
       }
       break;
       case TypeString:
       {
          charBuffer_snprintf(buffer, "Str=%s", get_string(&nd->str.value)); 
       }
       break;
       case TypeRef:
       {
          charBuffer_snprintf(buffer, "Ref=(%d,%d)", nd->ref.x, nd->ref.y); 
       }
       break;
       case TypeMinus:
       case TypeParam:
       case TypeCompound:
       {
          charBuffer_snprintf(buffer, "Nd=%s(%d) with nops=%d", nd->opr.oper_name, nd->opr.oper, nd->opr.nops); 
       }
       break;
       case TypeMul:
       case TypeSum:
       {
          assert(nd->opr.nops == 1);
          TNode *params = NULL;
          size_t n = 0;
          params = gather_params2(params, &n, nd->opr.op[0]);
          charBuffer_snprintf(buffer, "Nd=%s(%d) with nops=%d", nd->opr.oper_name, nd->opr.oper, nd->opr.nops); 
          if(charBufferEmpty(buffer))
          {
            for(int i = 0; i<n; i++)
            {
                if(charBufferEmpty(buffer))
                {
                    dump_node(buffer, &params[i],0);
                }
            }
          }
          free(params);
       }
       break;
   }
}

void dump_tree_preorder_internale(TCharBuffer *buffer, TNode *head, int level)
{
    dump_node(buffer, head, level);
    if(head->type == TypeCompound)
    {
        for(int i=0; i<head->opr.nops; i++)
        {
            dump_tree_preorder_internale(buffer, head->opr.op[i], level+1);
        }
    }
}
void dump_tree_preorder(TNode *head, FILE *f) 
{
    TCharBuffer buffer; 
    clearCharBuffer(&buffer);

    dump_tree_preorder_internale(&buffer, head, 0);
    fprintf(f,"%s\n", buffer.cs);
}

void dump_tree_postorder_internale(TCharBuffer *buffer, TNode *head, int level)
{
    if(head->type == TypeCompound)
    {
        for(int i=0; i<head->opr.nops; i++)
        {
            dump_tree_postorder_internale(buffer, head->opr.op[i], level+1);
        }
    }
    dump_node(buffer, head, level);
}
void dump_tree_postorder(TNode *head, FILE *f) 
{
    TCharBuffer buffer;
    clearCharBuffer(&buffer);
    dump_tree_postorder_internale(&buffer,head, 0);
    fprintf(f,"%s\n",(&buffer)->cs);
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
        case TypeMinus:
        case TypeParam:
        case TypeCompound:
        {
            assert(NULL && "not yet implemented");
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