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

\"([^"]|\"\")*\" {
/* -- STRING with quotes -- */
	/* yyleng is precomputed strlen(yytext) */
    size_t i=0, n = yyleng;
    char s[n];
    char *sp;

    if (n == 0)
    {
        return EMPTY;
    }

    memset(s,0, n*sizeof(char));
    sp = s;

	/* copy yytext, changing "" to " */
    for (i = 1 /*skip 0="*/; i < n-1; i++)
    {
        *sp++ = yytext[i];
        if (yytext[i] == '"')
            i++; /* skip second one */
    }
    TStringView sw = append_string_buffer(&sb,s);
    //DO(insert_text_into_table(t,row,col, &sw));
    yylval.valStr = sw;
    return STRING;
}


[^=",\r\n][^",\r\n]* { 
/* -- STRING w/o quotes -- */
    TStringView sw = append_string_buffer(&sb,yytext);
    //DO(insert_text_into_table(t,row,col, &sw));
    yylval.valStr = sw;
    return STRING;
}

\n|\r\n    { 
/* -- Line End -- */
    row_num++;
    BEGIN INITIAL;
    return LINE_END; 
}


\,          { 
/* -- Cell End -- */
    col_num++;
    BEGIN INITIAL;
    return CELL_END; 
}

<fml>"sum" {
    return SUM; 
}
<fml>[();-] {
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
    return REFERENCE;
}
<fml>\([1-9][0-9]*,[1-9][0-9]*\) {
    char *sx,*sy,*tofree;
    tofree = strdup(yytext);
    sx = strsep(&tofree, ",");
    sy = strsep(&tofree, ",");
    sy[strlen(sy)-1] = '\0';
    double x = atof(++sx);
    double y = atof(sy);
    //printf("YCEL: ref=(%.2f,%.2f)\n", x,y);
    TRef ref = {x,y};
    free(tofree);
    return REFERENCE;
}

<fml>.               yyerror("Unknown character");

.               yyerror("Unknown character");
%%