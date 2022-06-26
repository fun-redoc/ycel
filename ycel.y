%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "ycel.h"
#include "string_buffer_view.h"

#define DO(proc) if(ERR==proc) mk_error("Failure.", __LINE__, __FILE__)
#ifndef CAT 
    #define CAT(a,b) CAT2(a,b)
    #define CAT2(a,b) a##b
#endif

#define OPR(n)  n,#n

TStringBuffer sb;
TCellHeap *ch;

/* prototypes */
void cleanup();
void mk_error(char *s, int line, char* file);
TNode *mk_node    (TRef coord, int oper, const char *oper_name, ENodeType t, int nops, ...);
TNode *mk_node_ref(TRef coord, TRef r);
TNode *mk_node_num(TRef coord, double v);
TNode *mk_node_str(TRef coord, TStringView v);
void free_node(TNode *p);
TValNum exec_node(TNode *p);
int yylex(void);

int row_num=0;
int col_num=0;

void yyerror(char *s);
TNode *root_of_ast = NULL;

extern FILE* yyin;
%}

%union {
    double valNum;     
    TStringView valStr;
    TRef  ref;
    TNode *nPtr;       
};

%token <valNum> NUMBER
%token <valStr> STRING
%token <ref> REFERENCE
%token FORMULA SUM PROD IF EMPTY LINE_END CELL_END FILE_END
%nonassoc IFX
%nonassoc ELSE
%left '-'
%nonassoc UMINUS

%type <nPtr> ycel rows cells cell stmt expr_list expr 

%%
ycel:
            rows              {printf("before root assign\n");$$=$1;root_of_ast = $1; printf("ROOT\n");} 

rows:
        | cells LINE_END rows {printf("hier 5.3\n");$$=mk_node((TRef){row_num, col_num},OPR(LINE_END),TypeCompound, 2, $1 , $3);}
/*        | cells LINE_END      {printf("hier 5.2\n"); $$=$1; } */
        | cells               {printf("hier 5.1\n"); $$=$1; } 
/*        |  NULL           {printf("hier 5.4\n"); $$=mk_node((TRef){row_num, col_num},OPR(EMPTY),TypeCompound, 0); } */

cells:
          cell CELL_END cells {printf("hier 4.1\n");  $$=mk_node((TRef){row_num, col_num},OPR(CELL_END), TypeCompound, 2, $1, $3);}
        | cell                {printf("hier 4.2\n");  $$=$1;}
        |  /*NULL*/           {printf("hier 4.3\n"); $$=mk_node((TRef){row_num, col_num},OPR(EMPTY), TypeCompound, 0); }

cell:
          NUMBER              {$$=mk_node_num((TRef){row_num, col_num},$1);DO(update_num_into_table(ch, row_num, col_num, $1));}
        | STRING              {printf("hier 1\n");$$=mk_node_str((TRef){row_num, col_num},$1);printf("hier 2\n");DO(update_text_into_table(ch, row_num, col_num, &$1)); printf("hier 3\n");}
        | FORMULA stmt        {$$=$2;} 
        | EMPTY               {printf("hier empty\n");$$=mk_node((TRef){row_num, col_num},OPR(EMPTY),TypeCompound, 0);}
        |  /*NULL*/           {$$=mk_node((TRef){row_num, col_num},OPR(EMPTY),TypeCompound, 0);} 

stmt:
    SUM '(' expr_list ')'    {$$=mk_node((TRef){row_num, col_num},OPR(SUM), TypeSum,1,$3);DO(update_node_into_table(ch, row_num, col_num, $$));}

expr_list:
      expr                   {$$=$1;}
    | expr_list ';' expr     {$$=mk_node((TRef){row_num, col_num},';', ";", TypeParam, 2, $1,$3);}
    | expr ':' expr          {$$=mk_node((TRef){row_num, col_num},':', ":", TypeParam, 2, $1,$3);}

expr:
    stmt                    {$$=$1;}
    | NUMBER                {$$=mk_node_num((TRef){row_num, col_num},$1);}
    | REFERENCE             {$$=mk_node_ref((TRef){row_num, col_num},$1);}
    | '-' expr %prec UMINUS {$$=mk_node((TRef){row_num, col_num},OPR(UMINUS), TypeMinus, 1, $2);}
    | '(' expr ')'          {$$=$2;}

%%

TNode *mk_node_num(TRef coord, double value) {
    TNode *p;

    /* allocate node */
    if ((p = malloc(sizeof(TNode))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->coord = coord;
    p->type = TypeNum;
    p->num.value = value;

    return p;
}
TNode *mk_node_str(TRef coord, TStringView value) {
    TNode *p;

    /* allocate node */
    if ((p = malloc(sizeof(TNode))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->coord = coord;
    p->type = TypeString;
    p->str.value = value;

    return p;
}

TNode *mk_node_ref(TRef coord, TRef r) {
    TNode *p;

    /* allocate node */
    if ((p = malloc(sizeof(TNode))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->coord = coord;
    p->type = TypeRef;
    p->ref = r;

    return p;
}

TNode *mk_node(TRef coord, int oper, const char *oper_name, ENodeType t, int nops, ...) {
    va_list ap;
    TNode *p;
    int i;

    /* allocate node, extending op array */
    if ((p = malloc(sizeof(TNode) + (nops-1) * sizeof(TNode *))) == NULL)
        yyerror("out of memory");

    /* copy information */
    p->coord = coord;
    p->type = t;
    p->opr.oper = oper;
    strncpy(p->opr.oper_name, oper_name, MAX_OPER_NAME_SIZE);
    p->opr.nops = nops;
    va_start(ap, nops);
    for (i = 0; i < nops; i++)
        p->opr.op[i] = va_arg(ap, TNode*);
    va_end(ap);
    return p;
}

void free_node(TNode *p) {
    if (!p) return;
    if (p->type == TypeCompound) {
        for (int i = 0; i < p->opr.nops; i++)
        {
            if(p->opr.op[i])
             free_node(p->opr.op[i]);
        }
    }
    free (p);
}

TValNum exec_node(TNode *nPtr)
{
    assert(nPtr);
    switch(nPtr->type)
    {
        case TypeRef:
            assert("not yet implemented" && '\0');
            break;
        case TypeNum:
            return nPtr->num;
            break;
        case TypeString:
            break;
        case TypeParam:
        case TypeMinus:
        case TypeCompound:
        case TypeSum:
            switch(nPtr->opr.oper)
            {
                case SUM:
                {
                    TValNum res = {0.0};
                    res.value = 0.0;
                    for(int i=0; i<nPtr->opr.nops; i++)
                    {
                        TValNum op_res = exec_node(nPtr->opr.op[i]);
                        printf("op_res = %.2f\n", op_res.value);
                        res.value += op_res.value;
                    }
                    printf("res = %.2f\n", res.value);
                    return res;
                }
                break;
                case ';':
                {
                    TValNum left = exec_node(nPtr->opr.op[0]);


                }
                break;
                default:
                {
                    TValNum res;
                    for(int i=0; i<nPtr->opr.nops; i++)
                    {
                        res = exec_node(nPtr->opr.op[i]);
                    }
                    return res;
                    break;
                }

            }
            break;
    }
    return (TValNum){0};
}

void mk_error(char *s, int line, char* file)
{
    char buffer[1024];
    sprintf(buffer, "YCEL: %s in %s:%d\n", s, file, line);
    yyerror(buffer);
}

void yyerror(char *s) {
    fprintf(stdout, "YYERR: %s\n", s);
    cleanup();
    exit(1);
}

void cleanup()
{
    free_cell_heap(ch);
    clear_string_buffer(&sb);
}

int main(void) {
    init_string_buffer(&sb, INITIAL_STRING_BUFFER_SIZE);
    ch = init_cell_heap();

    //yyin = fopen("test_no_form.csv", "r");
    //yyin = fopen("test.csv", "r");
    //yyin = fopen("test2.csv", "r"); // with Ref second
    //yyin = fopen("test1.csv", "r"); // with Ref
    //yyin = fopen("test0.csv", "r"); // only nums
    //yyin = fopen("test3.csv", "r"); // with Ref loop should fail with assertion error
    yyin = fopen("test4.csv", "r"); // with Ref loop should fail with assertion error

    yyparse();
    //TODO better interpret the ast instad of cell heap, get rid of cell heap

    //dump_tree_preorder(root_of_ast, stdout); 
    fprintf(stdout, "--- before calclulation ---\n");
    dump_cell_heap(stdout, ch);
    calc(ch);
    fprintf(stdout, "--- after calclulation ---\n");
    dump_cell_heap(stdout, ch);
    cleanup();
    free_node(root_of_ast);
    return 0;
}