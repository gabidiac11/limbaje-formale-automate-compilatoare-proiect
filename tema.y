%{
#include <stdio.h>
#include "semantica.h"

extern FILE* yyin;
extern char* yytext;
extern void yyerror();
extern int yylex();

void popScope();
void pushScope(char*);
void scriereInTabelDeSimboluri();
void declarareClasa(char*);
void apelFunctie(char *);
void declarareVariabila(char* ,char*,char*, int);
void endAssign(char *);
void pushExpresieValuePeStack(char *);
void pushExpresieIdPeStack(char *);
void startAssignExpr(char *);
void declarareFunctie(char*, char*);
void asignareVariabila();
void expresieValoareAsignare(char*);
void evalNumberExpr(char* ,char*,char*, char*);
void evalExpresieBool(char* ,char*,char*, char*);
void evalIdNumberExpr(char*, char*);
void pushOperatorStack(char*);
void pushStackParan();
void popStackParan(char*);
void apelFunctie(char*);
void endApelFunctie(char*);
void evaluareIdCaBool(char*, char*);
%}

%union {
  char * tip;
  char * uTipBool;
  char * uTipString;
  char * id;

  char * numericValoare;
  char * exprNumeric;

  char * boolValoare;
  char * exprBool;
  char * operator;
  
  char * stringValue;
};

%token STRCMP STRCPY STRCAT RETURN

%token CONSTRUCT CLASS IF WHILE FOR ELSE START END START_FUN END_FUN BGIN_CLASS END_CLASS SECURE1 SECURE2 START_SECURE1 END_SECURE1 START_SECURE2 END_SECURE2

%token<tip> FLOAT INT VOID 
%token<id> ID ID_FUNCTIE
%token<boolValoare> BOOL_VALOARE
%token<numericValoare> INTEGER_VALOARE FLOAT_VALOARE 

%type<tip> tipVar
%token<uTipBool> BOOL
%token<uTipString> STRING
%token<stringValue> STRING_VALOARE
%type<boolValoare> VALOARE_TIP_BOOL
%type<stringValue> VALOARE_TIP_STRING
%type<exprNumeric> expresieComplexa VALOARE_NUMERICA
%type<exprBool> expresieBool
%token<operator> ASSIGN NEG EGAL MAI_MIC MIC_EGAL MAI_MARE MARE_EGAL AND OR
%left ASSIGN NEG EGAL MAI_MIC MIC_EGAL MAI_MARE MARE_EGAL AND OR
%left '+' '-'
%left '*' '/'

%start startProgram
%%
startProgram : global
      | startProgram global
      ;
global : tipVar ID';' { declarareVariabila($1,$2, "", 0);}
| tipVar ID { declarareVariabila($1,$2, "", 0); startAssignExpr($2);} '@' ASSIGN expresieComplexa';' { endAssign($2); }
| ID { startAssignExpr($1);} '@' ASSIGN expresieComplexa';' { endAssign($1); }
| STRING ID '*' ASSIGN VALOARE_TIP_STRING';' { declarareVariabila($1,$2, $5, 0); }
| STRING ID ';' 
| tipVar ID_FUNCTIE { declarareFunctie($1,$2); pushScope($2); } '(' listaParametriiDF ')' START_FUN  bloc  END_FUN 
| apelF ';'
| BOOL ID ';' { declarareVariabila($1,$2, "", 0); }
| BOOL ID ':' ASSIGN expresieBool ';' { declarareVariabila($1,$2, $5, 0); }
| ID  ':' ASSIGN expresieBool ';'
| CLASS  ID { pushScope($2); declarareClasa($2); } BGIN_CLASS bloc_class END_CLASS ';' 
|';'
;

bloc_class: security1 security2 constructor
;

security1: SECURE1 START_SECURE1  blocsecureR END_SECURE1
;

security2: SECURE2 START_SECURE2 blocsecureR END_SECURE2
;

constructor: CONSTRUCT '(' listaParametriiDF ')' START_FUN  bloc  END_FUN
;

blocsecureR:blocsecureR blocsecure
          | blocsecure;

blocsecure : tipVar ID';' { declarareVariabila($1,$2, "", 0);}
| tipVar ID { declarareVariabila($1,$2, "", 0); startAssignExpr($2);} '@' ASSIGN expresieComplexa';' { endAssign($2); }
| ID { startAssignExpr($1);} '@' ASSIGN expresieComplexa';' { endAssign($1); }
| STRING ID ';' 
| BOOL ID ';' { declarareVariabila($1,$2, "", 0); }
| BOOL ID ':' ASSIGN expresieBool ';' { declarareVariabila($1,$2, $5, 0); }
| ID  ':' ASSIGN expresieBool ';'
| tipVar ID_FUNCTIE { declarareFunctie($1,$2); pushScope($2); }'(' listaParametriiDF ')' START_FUN  bloc  END_FUN
;

bloc : local
| ifWhileFor
| apelF ';'
| STRCMP ID  ',' ID ';'
| STRCPY ID ',' ID ';'
| STRCAT ID ',' ID ';'
| bloc apelF ';'
| bloc  local
| bloc  ifWhileFor
| bloc STRCMP ID  ',' ID ';'
| bloc STRCPY ID ',' ID ';'
| bloc STRCAT ID ',' ID ';'
;

apelF : ID_FUNCTIE { apelFunctie($1); } '(' ')' { endApelFunctie($1); }
| ID_FUNCTIE { apelFunctie($1); } '#' '(' listaParametriiAF ')' { endApelFunctie($1); }
;

listaParametriiAF : { pushStackParan(); } expresieComplexa { popStackParan("fun_call"); }
| listaParametriiAF ',' { pushStackParan(); } expresieComplexa { popStackParan("fun_call"); }
;

local : tipVar ID';' { declarareVariabila($1,$2, "", 0);}
| tipVar ID { declarareVariabila($1,$2, "", 0); startAssignExpr($2);} '@' ASSIGN expresieComplexa';' { endAssign($2); }
| STRING ID ';' 
| ID { startAssignExpr($1);} '@' ASSIGN expresieComplexa';' { endAssign($1); }
| BOOL ID ';' { declarareVariabila($1,$2, "", 0); }
| BOOL ID ':' ASSIGN expresieBool ';' { declarareVariabila($1,$2, $5, 0); }
| ID  ':' ASSIGN expresieBool ';'
| RETURN { startAssignExpr("return");} '(' expresieComplexa ')' ';' { endAssign("return"); }
| ';'
;

listaParametriiDF : 
|tipVar ID { declarareVariabila($1,$2, "", 1);}
| listaParametriiDF ',' tipVar ID { declarareVariabila($3,$4, "", 1);}
| listaParametriiDF ',' BOOL ID { declarareVariabila($3,$4, "", 1);}
;

ifWhileFor :  instructiuneIf
| instructiuneWhile
| instructiuneFor
;

instructiuneIf : IF '(' expresieBool ')' START bloc END ELSE START bloc END 
| IF '(' expresieBool ')' START bloc END 
;

instructiuneWhile : WHILE '('expresieBool ')' START bloc END
; 

instructiuneFor : FOR '('bloc  expresieBool ';' bloc ')' START bloc END
;

expresieBool: VALOARE_TIP_BOOL { $$ = yyval.boolValoare ;}
         | ID { evaluareIdCaBool($$, $1); }
         | VALOARE_NUMERICA { $$ = $1; }
         | expresieBool EGAL  expresieBool { evalExpresieBool($$, $1, "==",$3); }
         | expresieBool NEG  expresieBool { evalExpresieBool($$, $1, "!=",$3); }
         | expresieBool MAI_MARE  expresieBool { evalExpresieBool($$, $1, ">=",$3); }
         | expresieBool MAI_MIC  expresieBool { evalExpresieBool($$, $1, "<",$3); }
         | expresieBool MARE_EGAL expresieBool { evalExpresieBool($$, $1, ">=",$3); }
         | expresieBool MIC_EGAL expresieBool { evalExpresieBool($$, $1, "<=",$3); }  
         | expresieBool AND expresieBool { evalExpresieBool($$, $1, "and",$3); }
         | expresieBool OR  expresieBool { evalExpresieBool($$, $1, "or",$3); }
         | '(' expresieBool ')' { $$ = $2; }
         ;

expresieComplexa: VALOARE_NUMERICA { pushExpresieValuePeStack($1); }
| ID { pushExpresieIdPeStack($1); }
| apelF
| expresieComplexa '+' { pushOperatorStack("+"); } expresieComplexa
| expresieComplexa '-' { pushOperatorStack("-"); } expresieComplexa
| expresieComplexa '*' { pushOperatorStack("*"); } expresieComplexa
| expresieComplexa '/' { pushOperatorStack("/"); } expresieComplexa
| '(' { pushStackParan(); } expresieComplexa ')' { popStackParan(""); }
;

tipVar : INT 
| FLOAT  
| VOID
; 

VALOARE_NUMERICA : INTEGER_VALOARE { $$ = yyval.numericValoare ; }
| FLOAT_VALOARE { $$ = yyval.numericValoare ; }
;

VALOARE_TIP_BOOL : BOOL_VALOARE { $$ = yyval.boolValoare;}
;
VALOARE_TIP_STRING : STRING_VALOARE { $$ = yyval.stringValue;}
;

%%
void yyerror(char * s) {
  scriereInTabelDeSimboluri();
  printf("eroare: %s la linia:%d\n",s,yylineno);
}
int main(int argc, char** argv) {
  yyin = fopen(argv[1],"r");
  yyparse();
  scriereInTabelDeSimboluri();
} 