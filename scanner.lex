/* definition section */
%{

/* declarations section */
#include "attributes.h"
#include "hw3_output.hpp"
#include "parser.tab.hpp"
#include<string.h>

%}

%option yylineno
%option noyywrap

WHITESPACE    ([\t\n\r ])
ID            [a-zA-Z][0-9a-zA-Z]*
NUM           0|[1-9][0-9]*
COMMENT       "//"[^\r\n]*[\r|\n|\r\n]?
STRING        \"([^\n\r\"\\]|\\[rnt\"\\])+\"

%%

void               {return VOID;}
int                {return INT;}
byte               {return BYTE;}
b                  {return B;}
bool               {return BOOL;}
override           {return OVERRIDE;}
and                {return AND;}
or                 {return OR;}
not                {return NOT;}
true               {return TRUE;}
false              {return FALSE;}
return             {return RETURN;}
if                 {return IF;}
else               {return ELSE;}
while              {return WHILE;}
break              {return BREAK;}
continue           {return CONTINUE;}
;                  {return SC;}
,                  {return COMMA;}
\(                 {return LPAREN;}
\)                 {return RPAREN;}
\{                 {return LBRACE;}
\}                 {return RBRACE;}
"="                {return ASSIGN;}

"=="               {return EQUAL;}
"!="               {return NOT_EQUAL;}
"<="               {return LESS_EQUAL;}
">="               {return GREATER_EQUAL;}
">"                {return GREATER;}
"<"                {return LESS;}

\+                 {return PLUS;}
\*                 {return MUL;}
\-                 {return MINUS;}
"/"                {return DIV;}

{ID}               {yylval.id_name = (char*)strdup(yytext);
                    return ID;}

{NUM}              {yylval.int_val = atoi(yytext);
                    return NUM;}

{COMMENT}          {}

{STRING}           {yylval.string_val = (char*)strdup(yytext);
                    return STRING;}

{WHITESPACE}       {}

.                  {output::errorLex(yylineno);
                    exit(0);}

%%

