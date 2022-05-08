%{
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include "ycel.h"

    extern char *string_val;
    extern double num_val;
%}

%option noyywrap

%%
=[^",\r\n]+ {
    string_val = strdup(yytext);    
    return TKN_FORMULA; 
}

[+-]?[0-9]+(\.[0-9]+)?([eE][0-9]+)? {
/* -- NUM -- */
    num_val = atof(yytext);
    return TKN_NUM;
}

\"([^"]|\"\")*\" {
/* -- STRING qith quotes -- */
	/* yyleng is precomputed strlen(yytext) */
    size_t i, n = yyleng;
    char *s;

    s = string_val = calloc(n, 1);
    if (!s)
        return TKN_STRING;

	/* copy yytext, changing "" to " */
    for (i = 1 /*skip 0="*/; i < n-1; i++)
    {
        *s++ = yytext[i];
        if (yytext[i] == '"')
            i++; /* skip second one */
    }
    return TKN_STRING;
}

[^",\r\n]+ { 
/* -- STRING w/o quotes -- */
    string_val = strdup(yytext);    
    return TKN_STRING; 
}

\n|\r\n    { 
/* -- Line End -- */
    return TKN_CRLF; 
}

\,          { 
/* -- Cell End -- */
    return TKN_SEP; 
}

.          { 
/* TODO -- all other -- */
    //yyerror ... 
    printf("ERROR -->%s<--\n", yytext);
    return *yytext; 
}
%%