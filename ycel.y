%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "ycel_misc.h"
#include "ycel_parser.h"
#include "ycel_table.h"
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
void check_type(TNode *n, ENodeType t);
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
%token FORMULA AVG MUL SUM IF EMPTY LINE_END CELL_END FILE_END
%nonassoc IFX
%nonassoc ELSE
%left '-'
%nonassoc UMINUS

%type <nPtr> ycel rows cells cell stmt expr_list expr 

%%
ycel:
            rows              {$$=$1;root_of_ast = $1;} 

rows:
          cells LINE_END rows {$$=mk_node((TRef){row_num, col_num},OPR(LINE_END),TypeNewLine, 2, $1 , $3);}
        | cells LINE_END      {$$=$1;} 
        | cells               {$$=$1;} 
        |                     {;} 

cells:
          cell CELL_END cells {$$=mk_node((TRef){row_num, col_num},OPR(CELL_END), TypeNewCell, 2, $1, $3);}
        | cell                {$$=$1;}
        |                     {;}

cell:
          NUMBER              {$$=mk_node_num((TRef){row_num, col_num},$1);}
        | STRING              {$$=mk_node_str((TRef){row_num, col_num},$1);}
        | FORMULA expr        {$$=$2;} 
        | EMPTY               {;}
        |                     {;}

stmt:
      SUM '(' expr_list ')'    {$$=mk_node((TRef){row_num, col_num},OPR(SUM), TypeSum,1,$3);}
    | MUL '(' expr_list ')'    {$$=mk_node((TRef){row_num, col_num},OPR(MUL), TypeMul,1,$3);}
    | AVG '(' expr_list ')'    {$$=mk_node((TRef){row_num, col_num},OPR(AVG), TypeAvg,1,$3);}
    | '-' stmt %prec UMINUS    {$$=mk_node((TRef){row_num, col_num},OPR(UMINUS), TypeNeg, 1, $2);}

expr_list:
      expr                   {$$=$1;}
    | expr_list ';' expr     {$$=mk_node((TRef){row_num, col_num},';', ";", TypeParam, 2, $1,$3);}
    | expr ':' expr          {check_type($1, TypeRef); check_type($3, TypeRef); $$=mk_node((TRef){row_num, col_num},':', ":", TypeParam, 2, $1,$3);}

expr:
    stmt                    {$$=$1;}
    | expr '+' expr         {$$=mk_node((TRef){row_num, col_num},'+',"+", TypePlus,2,$1,$3) ;}
    | expr '-' expr         {$$=mk_node((TRef){row_num, col_num},'-',"-", TypeMinus,2,$1,$3);}
    | expr '*' expr         {$$=mk_node((TRef){row_num, col_num},'*',"*", TypeTimes,2,$1,$3);}
    | expr '/' expr         {$$=mk_node((TRef){row_num, col_num},'/',"/", TypeDiv,2,$1,$3)  ;}
    | NUMBER                {$$=mk_node_num((TRef){row_num, col_num},$1);}
    | REFERENCE             {$$=mk_node_ref((TRef){row_num, col_num},$1);}
    | '-' expr %prec UMINUS {$$=mk_node((TRef){row_num, col_num},OPR(UMINUS), TypeNeg, 1, $2);}
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
#ifdef __DEBUG__
    yyin = fopen("test4.csv", "r"); 
#endif

    yyparse();
    tree_to_table(ch, root_of_ast, 0,0);

#ifdef __DEBUG__
    fprintf(stdout, "--- before calclulation ---\n");
    dump_cell_heap(stdout, ch);
#endif

    calc(ch);

#ifdef __DEBUG__
    fprintf(stdout, "--- after calclulation ---\n");
    dump_cell_heap(stdout, ch);
#endif

    table_out(stdout, ch, ',', "\n");

    cleanup();
    free_node(root_of_ast);

    return 0;
}


void check_type(TNode *n, ENodeType t){
    if(n->type != t)
    {
        yyerror("Reference expected here.\n");
    }
}