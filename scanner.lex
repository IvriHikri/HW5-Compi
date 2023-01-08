%{

#include "classes.h"
#include "hw3_output.hpp"
#include "parser.tab.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
%}

%option yylineno
%option noyywrap

whitespace ([\t\n\r ])

%%
void {yylval = new Node(yytext); return VOID;}
int {yylval = new Node(yytext); return INT;}
byte {yylval = new Node(yytext); return BYTE;}
bool {yylval = new Node(yytext); return BOOL;}
b {yylval = new Node(yytext); return B;}
and {yylval = new Node(yytext); return AND;}
or {yylval = new Node(yytext); return OR;}
not {yylval = new Node(yytext); return NOT;}
true {yylval = new Node(yytext, V_BOOL); return TRUE;}
false {yylval = new Node(yytext, V_BOOL); return FALSE;}
return {yylval = new Node(yytext); return RETURN;}
if {return IF;}
else {return ELSE;}
while {return WHILE;}
break {yylval = new Node(yytext); return BREAK;}
continue {yylval = new Node(yytext); return CONTINUE;}
\; {return SC;}
\, {return COMMA;}
\( {return LPAREN;}
\) {return RPAREN;}
\{ {return LBRACE;}
\} { return RBRACE;}
(<|>|<=|>=) {yylval = new Node(yytext); return RELOP;}
(==|!=) {yylval = new Node(yytext); return RELOP_EQ;}
[\+\-] {yylval = new Node(yytext); return BINOP;} 
[\*\/] {yylval = new Node(yytext); return BINOP_MUL_DIV;}
\= return ASSIGN;
([a-zA-Z][a-zA-Z0-9]*) {yylval = new Id(yytext); return ID;}
([1-9][0-9]*)|[0] {yylval = new Node(yytext, V_INT); return NUM;}
(\/\/[^\r\n]*[\r|\n|\r\n]?) ;
{whitespace} ;

(\"([^\n\r\"\\]|\\[rnt\"\\])+\") {yylval = new Node (yytext, V_STRING); return STRING;}

. {output::errorLex(yylineno);}
%%