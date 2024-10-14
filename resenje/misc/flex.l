%{
  #include "bison.tab.hpp"
  #include <cstring>
  int linenumber = 0;

%}

%option noyywrap

%%

%r([0-9]|10|11|12|13|14|15) {yylval.intVal = atoi(yytext + 2); return GP_REGISTER;}
%sp              {yylval.intVal = 14; return GP_REGISTER;}
%pc              {yylval.intVal = 15; return GP_REGISTER;}
%status          {yylval.intVal = 0; return CS_REGISTER;}
%handler         {yylval.intVal = 1; return CS_REGISTER;}
%cause           {yylval.intVal = 2; return CS_REGISTER;}

".global"   {return GLOBAL;}
".extern"   {return EXTERN;}
".section"  {return SECTION;}
".word"     {return WORD;}
".skip"     {return SKIP;}
".ascii"    {return ASCII;}
".equ"      {return EQU;}
".end"      {return END;}

halt    {return HALT;}
int     {return INT;}
iret    {return IRET;}
call    {return CALL;}
ret     {return RET;}
jmp     {return JMP;}
beq     {return BEQ;}
bne     {return BNE;}
bgt     {return BGT;}
push    {return PUSH;}
pop     {return POP;}
xchg    {return XCHG;}
add     {return ADD;}
sub     {return SUB;}
mul     {return MUL;}
div     {return DIV;}
not     {return NOT;}
and     {return AND;}
or      {return OR;}
xor     {return XOR;}
shl     {return SHL;}
shr     {return SHR;}
ld      {return LD;}
st      {return ST;} 
csrrd   {return CSRRD;}
csrwr   {return CSRWR;}


([0-9]+|0[xX][0-9a-fA-F]+) {
  yylval.strVal = strdup(yytext); 
  return LITERAL;
}


[a-zA-Z.][a-zA-Z0-9_]*: {
  size_t length = yyleng - 1;
  yylval.strVal = (char*)malloc(length + 1);
  strncpy(yylval.strVal, yytext, length);
  yylval.strVal[length] = '\0';
  return LABEL;  
}


[a-zA-Z_][a-zA-Z0-9_]* {
  yylval.strVal = strdup(yytext); 
  return SYMBOL;
}


","     {return COMMA;}
"+"     {return PLUS;}
"-"     {return MINUS;}
";"     {return SEMI;}
":"     {return COLON;}
"["     {return LSQUARE;}
"]"     {return RSQUARE;}
"%"     {return PERCENT;}
"$"     {return DOLLAR;}


"\""([^"\n]|\\.)*"\""     {yylval.strVal = strdup(yytext); return STRING;}
\n                        {linenumber = linenumber + 1; return END_LINE;}
"#"[^\n\r]*               {yylval.strVal = strdup(yytext); return COMMENT;}
[ \t]+ {;}


. { printf("ERROR: Unrecognized token '%s'\n", yytext); }

%%