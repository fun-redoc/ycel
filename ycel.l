%{
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <ctype.h>
    #include <assert.h>
    #include "ycel.h"
    #include "y.tab.h"
    #include "string_buffer_view.h"

    #define DO(proc) if(ERR==proc) goto error 


    void yyerror(char *);

    extern TStringBuffer sb;
    extern int row_num;
    extern int col_num;
%}

%option noyywrap
%x fml 

%%
= {
    //TStringView sw = append_string_buffer(&sb,yytext);
    //DO(insert_text_into_table(t,row,col, &sw));
    //yylval.valFormula = sw;
    BEGIN fml;
    return FORMULA; 
}

[+-]?[0-9]+(\.[0-9]+)?([eE][0-9]+)? {
/* -- NUM -- */
    yylval.valNum = atof(yytext);
    return NUMBER;
}

[ \t]*\"([^"]|\"\")*\" {
/* -- STRING with quotes -- */
    printf("BEGIN: YYCEL wq: row=%d col=%d TEXT=%s\n", row_num, col_num, yytext);
	/* yyleng is precomputed strlen(yytext) */
    size_t n_spaces = 0;
    while(yytext[n_spaces]!='"') n_spaces++;
    size_t i=0, n = yyleng-n_spaces;
    char s[n];
    char *sp;

    if (n == 2) // only 2 quotes
    {
        return EMPTY;
    }

    memset(s,0, n*sizeof(char));
    sp = s;

	/* copy yytext, changing "" to " */
    for (i = 1 /*skip 0="*/; i < n-1; i++)
    {
        *sp++ = yytext[n_spaces+i];
        if (yytext[n_spaces + i] == '"')
            i++; /* skip second one */
    }
    TStringView sw = append_string_buffer(&sb,s);
    //DO(insert_text_into_table(t,row,col, &sw));
    yylval.valStr = sw;
    printf("END: YYCEL wq: row=%d col=%d TEXT=%s\n", row_num, col_num, get_string(&sw));
    return STRING;
}


[ \t]*[^=",\r\n][^",\r\n]* { 
/* -- STRING w/o quotes -- */
    printf("BEGIN: YYCEL woq: row=%d col=%d TEXT=|%s|\n", row_num, col_num, yytext);
    TStringView sw = append_string_buffer(&sb,yytext);
    //DO(insert_text_into_table(t,row,col, &sw));
    yylval.valStr = sw;
    printf("END: YYCEL woq: row=%d col=%d TEXT=|%s|\n", row_num, col_num, get_string(&sw));
    return STRING;
}

\n|\r\n   { 
/* -- Line End -- */
    printf("-6-->YCEL: row=%d col=%d\n",row_num,col_num);
    row_num++;
    col_num = 0;
    BEGIN INITIAL;
    return LINE_END; 
}

\, { 
/* -- Cell End -- */
    printf("-5-->YCEL: row=%d col=%d\n",row_num,col_num);
    col_num++;
    BEGIN INITIAL;
    return CELL_END; 
}

<fml>"sum" {
    printf("YCEL: row=%d col=%d : sum\n", row_num, col_num);
    return SUM; 
}
<fml>[:();-] {
    return *yytext;
}
<fml>[+-]?[0-9]+(\.[0-9]+)?([eE][0-9]+)? {
/* -- NUM -- */
    yylval.valNum = atof(yytext);
    return NUMBER;
}
<fml>([a-zA-Z]{1,2})([1-9][0-9]{0,2}) {
    char *sx,*sy,*tofree, *p;
    tofree = p = strdup(yytext);
    assert(tofree);
    for ( ; *p; ++p) *p = tolower(*p);    
    int pos = 0;
    int pos_factor = 1;
    p = tofree;
    int y = 0;
    int i = 0;
    for(i = strlen(p)-1;isdigit(p[i]);i--) {
        y += (p[i] - '0') * pos_factor;
        pos_factor *= 10;
    }
    int x = 0;
    pos_factor = 1; 
    for(int j = i;j>=0 && isalpha(p[j]);j--) {
        x += (p[j] - 'a' + 1) * pos_factor;
        pos_factor *= 'z'-'a' + 1;
    }
    free(tofree);
    TRef ref = {x,y};
    yylval.ref = ref;
    printf("-1-->YCEL: row=%d col=%d %s ref=(%d,%d)\n",row_num, col_num, yytext, x,y);
    return REFERENCE;
}

<fml>\([1-9][0-9]*,[1-9][0-9]*\) {
    char *sx,*sy,*tofree;
    tofree = strdup(yytext);
    assert(tofree);
    sy = strsep(&tofree, ",");
    sx = strsep(&tofree, ",");
    sx[strlen(sy)-1] = '\0';
    int y = atoi(++sy);
    int x = atoi(sx);
    TRef ref = {x,y};
    yylval.ref = ref;
    free(tofree);
    printf("-2-->YCEL: row=%d col=%d %s ref=(%d,%d)\n", row_num, col_num, yytext, x,y);
    return REFERENCE;
}

<fml>\n|\r\n { 
/* -- Line End -- */
    printf("-3-->YCEL: row=%d col=%d\n", row_num, col_num);
    row_num++;
    col_num = 0;
    BEGIN INITIAL;
    return LINE_END; 
}


<fml>\, { 
/* -- Cell End -- */
    printf("-4-->YCEL: row=%d col=%d\n",row_num,col_num);
    col_num++;
    BEGIN INITIAL;
    return CELL_END; 
}

.   {
    printf("unknow error"); yyerror("Unknown character");
}

<fml>.   {
    printf("unknown error in formula\n");
    char buffer[STR_BUF_SIZE] = {0};
    snprintf((char *)&buffer, STR_BUF_SIZE, "unexpected character in Cell (%d,%d) Formula: %s\n", row_num, col_num, yytext);
    yyerror((char *)&buffer);
}

%%

//int yywrap(void) {
//    printf("yywrap\n");
//    return 1;
//}