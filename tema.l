%{
#include <stdio.h>
#include "y.tab.h"
extern int yylineno;
%}
%option noyywrap
%%
"float" {yylval.tip = strdup(yytext);return FLOAT;}
"int" {yylval.tip = strdup(yytext);return INT;}
"class"  {yylval.tip = strdup(yytext);return CLASS;}
"void"   {yylval.tip = strdup(yytext);return VOID;}
"bool" {yylval.uTipBool = strdup(yytext);return BOOL;}
"string"   {yylval.uTipString = strdup(yytext);return STRING;}

"start" {return START;}
"end" {return END;}
"start_public" { pushClassPublic(); return START_SECURE1;}
"end_public" {popClassPublic(); return END_SECURE1;}
"start_privat" { pushClassPrivate(); return START_SECURE2;}
"end_privat" {popClassPrivate(); return END_SECURE2;}
"start_fun" {return START_FUN;}
"end_fun" { popScope(); return END_FUN;}
"begin_class" {return BGIN_CLASS;}
"end_class" {popScope(); return END_CLASS;}
"public:" {return SECURE1;}
"private:" {return SECURE2;}
"construct" {yylval.tip = strdup(yytext);return CONSTRUCT;}

"=" 			{return ASSIGN;}
"=="            { return EGAL;}
"!="            { return NEG;}
"<"             { return MAI_MIC;}
">"             { return MAI_MARE;}
">="            { return MARE_EGAL;}
"<="            { return MIC_EGAL;}
"and"            { return AND;}
"or"            { return OR;}
"=string="     { return STRCMP;}
"scopy="     { return STRCPY;}
"scat="     { return STRCAT;} 

"if"			{ return IF;}
"while"			{ return WHILE;}
"for"			{ return FOR;}
"else"			{ return ELSE;}
"return"			{ return RETURN;}

@[_a-zA-Z][_a-zA-Z0-9]* {yylval.tip = strdup(yytext); return ID;}
#[_a-zA-Z][_a-zA-Z0-9]* {yylval.tip = strdup(yytext); return ID_FUNCTIE;}
([1-9][0-9]*)|0 {yylval.numericValoare = strdup(yytext); return INTEGER_VALOARE;}

(([1-9][0-9]*)|0)\.?[0-9]+ {yylval.numericValoare = strdup(yytext); return FLOAT_VALOARE;}

\"[^"]*\" {yylval.stringValue = strdup(yytext); return STRING_VALOARE;}

(true)|(false) {yylval.boolValoare = strdup(yytext); return BOOL_VALOARE;}

[ \t] ;
\n { yylineno++;}
. { return yytext[0];}
%%