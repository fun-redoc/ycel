#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "string_buffer_view.h"
#include "ycel_misc.h"
#include "ycel_parser.h"
#include "maybe.h"

//************ Parser ************
extern int yylex();
extern char* yytext;
extern FILE* yyin;
int row=0;
int col=0;
char *string_val;
double num_val;

// prototypes

TNode *gather_params2(TNode *params, size_t *n, TNode *nd)
{
    assert(nd->opr.oper == ';' || nd->opr.oper == ':');
    char sep = nd->opr.oper;
    
    // make place for at least o new param

    if(sep == ':') 
    {

        const TNode *head = nd->opr.op[0];
        const TNode *tail = nd->opr.op[1];
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

void dump_node(TCharBuffer *buffer, TNode *nd, const int level)
{
   level_prefix(buffer, level);

   switch(nd->type)
   {
       case TypeNum:
       {
          charBuffer_snprintf(buffer, "Num=%.2f", nd->num.value);
          charBuffer_snprintf(buffer,"%s","\n"); 
       }
       break;
       case TypeString:
       {
          charBuffer_snprintf(buffer, "Str=%s", get_string(&nd->str.value)); 
          charBuffer_snprintf(buffer,"%s","\n"); 
       }
       break;
       case TypeRef:
       {
          charBuffer_snprintf(buffer, "Ref=(%d,%d)", nd->ref.x, nd->ref.y); 
          charBuffer_snprintf(buffer,"%s","\n"); 
       }
       break;
       case TypeNeg:
       {
          assert(nd->opr.nops == 1);
          charBuffer_snprintf(buffer, "Nd=%s(%d) with nops=%d", nd->opr.oper_name, nd->opr.oper, nd->opr.nops); 
          charBuffer_snprintf(buffer,"%s","\n"); 
          dump_node(buffer, nd->opr.op[0],0);
       }
       break;
       case TypeParam:
       {
            assert(false && "not implemented");
       }
       break;
       case TypeCompound:
       case TypeNewLine:
       case TypeNewCell:
       {
          charBuffer_snprintf(buffer, "Nd=%s(%d) with nops=%d", nd->opr.oper_name, nd->opr.oper, nd->opr.nops); 
          charBuffer_snprintf(buffer,"%s","\n"); 
          for(int i=0; i < nd->opr.nops; i++)
          {
            dump_node(buffer, nd->opr.op[i],0);
          }
       }
       break;
       case TypeAvg:
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
          charBuffer_snprintf(buffer,"%s","\n"); 
          free(params);
       }
       break;
       case TypePlus:
       case TypeMinus:
       case TypeTimes:
       case TypeDiv:
       {
          assert(nd->opr.nops == 2);
          charBuffer_snprintf(buffer, "Nd=%s(%d) with nops=%d", nd->opr.oper_name, nd->opr.oper, nd->opr.nops); 
          dump_node(buffer, nd->opr.op[0],0);
          dump_node(buffer, nd->opr.op[1],0);
          charBuffer_snprintf(buffer,"%s","\n"); 
       }
       break;
   }
}
