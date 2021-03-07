#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylineno = 0;

#define SCOPE_GLOBAL 0

#define STACK_TYPE_VAR 0
#define STACK_TYPE_FUN 1
#define STACK_TYPE_CLASS 2
#define STACK_TYPE_ASSIGN 3
#define STACK_TYPE_OPERATOR 4
#define STACK_TYPE_RETURN 5
#define STACK_TYPE_PARAMETRU 6
#define STACK_TYPE_VALUE 7
#define STACK_TYPE_VAR_ADDRESS 8
#define STACK_TYPE_ASSIGN_END 9
#define STACK_TYPE_VAR_INNER 10
#define STACK_TYPE_FUN_END 11
#define STACK_TYPE_FUN_CALL 12
#define STACK_TYPE_FUN_CALL_END 13
#define STACK_TYPE_FUN_RETRUN 14

const char STACK_TYPE_NAMES[15][30] = {
    "STACK_TYPE_VAR",
    "STACK_TYPE_FUN",
    "STACK_TYPE_CLASS",
    "STACK_TYPE_ASSIGN",
    "STACK_TYPE_OPERATOR",
    "STACK_TYPE_RETURN",
    "STACK_TYPE_PARAMETRU",
    "STACK_TYPE_VALUE",
    "STACK_TYPE_VAR_ADDRESS",
    "STACK_TYPE_ASSIGN_END",
    "STACK_TYPE_VAR_INNER",
    "STACK_TYPE_FUN_END",
    "STACK_TYPE_FUN_CALL",
    "STACK_TYPE_FUN_CALL_END",
    "STACK_TYPE_FUN_RETRUN"};

#define VAR_TYPE_INT 0
#define VAR_TYPE_FLOAT 1
#define VAR_TYPE_BOOL 2
#define VAR_TYPE_STRING 3
#define VAR_TYPE_VOID 4
#define VAR_TYPE_CLASS 5

const unsigned int VAR_NUME_TIPURI_LEN = 6;
const char VAR_NUME_TIPURI[6][100] = {
    "int",
    "float",
    "bool",
    "string",
    "void",
    "class"};
#define MAX_NUM_OF_STACK 1024
unsigned int numOfIds = 0;

#define MAX_SCOPE_LEN 100
char scopeStack[100][100] = {"global"}; /* initializat cu global */
int currentScopeLength = 1;

struct ENTITATE_VAR_STRUCT {
    char id[100];
    int tip;
    unsigned int initializat;
    char valoare[100];
    char numeScope[100]; /* "" - global, id functie/clasa */
    unsigned int esteParametru;
    unsigned int derivaDinParametru;
} entitatiVar[1024];

struct ENTITATE_FUNCTIE_STRUCT {
    char id[100];
    char listaParametrii[50][100];
    unsigned int nrParametrii;
    unsigned int tip;
    char numeScope[100]; /* "" - global, id clasa */
};

struct STACK_STRUCT {
    struct ENTITATE_VAR_STRUCT entitateVar;
    struct ENTITATE_FUNCTIE_STRUCT entitateFunctie;
    char numeScope[100];
    unsigned int blockIndex;
    unsigned stackType;  //func or var
    char operatie[15];
    char operator[3];

} stack[2000];
unsigned int stackLength = 0;

#define MAX_SYMBOL_TABLE 1000
unsigned int symbolTableLen = 0;
struct SYMBOL_TABLE_STRUCT {
    char id[100];
    char tipDeDate[20];
    char tipEntitate[20];
    char scope[20];
    int initializat;
    char valoare[200];
    unsigned int esteParametru;
    unsigned int clasaNivelProtectie;
} symbolTable[1000];

/* marcheaza stack-ul de paranteze pentru o expresie (ajuta la evaloarea instructiunilor de pe stack) 
   pentru fiecare paranteze deschise se creeaza o variabila noua intena unica 
   ce va avea propria instruciune de asignare, dupa care va fi stearsa cand va fi asignata unei variabile declarate
*/
#define MAX_PARAN_IDS 300
int paranIdsLen = 0;
char paranIds[300][100] = {};

/* marcheaza stack-ul de expresii return (ajuta la evaloarea instructiunilor de pe stack) 
   pentru fiecare instructiune de return se creeaza o variabila noua intena unica 
   ce va avea propria instruciune de asignare, care va ramane in instructinile functiei
*/
#define MAX_RETRUN_IDS 300
int returnIdsLen = 0;
char returnIds[300][100] = {};

/* marcheaza instrarea/iesirea dintr-un nivel de protectie al unei clase */
unsigned int clasaNivelProtectie = 0;

/* marcheaza cautarea parametrilor */
#define MAX_FUNC_PARAM_NUM 30
unsigned int parametriiApelLen = 0;
char parametriiApel[30] = {};
unsigned int inregistrareParametrii = 0;

void pushClassPublic() {
    clasaNivelProtectie = 1;
}
void popClassPublic() {
    clasaNivelProtectie = 0;
}
void pushClassPrivate() {
    clasaNivelProtectie = 2;
}
void popClassPrivate() {
    clasaNivelProtectie = 0;
}

/* symbol table */
void scriereInTabelDeSimboluri() {
    FILE *sTableFile = fopen("table.txt", "w");
    for (unsigned int i = 0; i < symbolTableLen; i++) {
        if (strcmp(symbolTable[i].tipEntitate, "clasa") == 0) {
            fprintf(sTableFile, "Clasa, Nume: '%s', Scope: global \n", symbolTable[i].id);
        }
        if (strcmp(symbolTable[i].tipEntitate, "variabila") == 0) {
            char label[] = "Variabila";
            if (symbolTable[i].esteParametru == 1) {
                strcpy(label, "Parametru");
            }
            char nivelDeProtectie[50] = "";
            if (symbolTable[i].clasaNivelProtectie == 1) {
                strcpy(nivelDeProtectie, ", Nivel protectie: public");
            }
            if (symbolTable[i].clasaNivelProtectie == 2) {
                strcpy(nivelDeProtectie, ", Nivel protectie: privat");
            }
            if (symbolTable[i].initializat) {
                fprintf(sTableFile, "%s, Nume: '%s', Tip: %s, Valoare: '%s', Scope: '%s' %s\n", label, symbolTable[i].id, symbolTable[i].tipDeDate, symbolTable[i].valoare, symbolTable[i].scope, nivelDeProtectie);
            } else {
                fprintf(sTableFile, "%s, Nume: '%s', Tip: %s, Valoare: nedefinit, Scope: '%s'%s\n", label, symbolTable[i].id, symbolTable[i].tipDeDate, symbolTable[i].scope, nivelDeProtectie);
            }
        }
        if (strcmp(symbolTable[i].tipEntitate, "functie") == 0) {
            char nivelDeProtectie[50] = "";
            if (symbolTable[i].clasaNivelProtectie == 1) {
                strcpy(nivelDeProtectie, ", Nivel protectie: public");
            }
            if (symbolTable[i].clasaNivelProtectie == 2) {
                strcpy(nivelDeProtectie, ", Nivel protectie: privat");
            }

            fprintf(sTableFile, "Functie, Nume: '%s', Tip: %s, Scope: '%s'%s\n", symbolTable[i].id, symbolTable[i].tipDeDate, symbolTable[i].scope, nivelDeProtectie);
        }
    }
    fclose(sTableFile);
}

void pushStackVar(struct ENTITATE_VAR_STRUCT var) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de variabile depasit.\n", yylineno);
        exit(0);
    }
    stack[stackLength].entitateVar = var;
    strcpy(stack[stackLength].numeScope, var.numeScope);
    stack[stackLength].blockIndex = 0;
    stack[stackLength].stackType = STACK_TYPE_VAR;
    stackLength += 1;

    verificareStack();
}

void pushStackFunction(struct ENTITATE_FUNCTIE_STRUCT func) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de variabile depasit.\n", yylineno);
        exit(0);
    }
    stack[stackLength].entitateFunctie = func;
    strcpy(stack[stackLength].numeScope, func.numeScope);
    stack[stackLength].blockIndex = 0;
    stack[stackLength].stackType = STACK_TYPE_FUN;
    stackLength += 1;

    verificareStack();
}

/* cauta sa vada variabile duplicate in acelasi scope */
void verificareStack() {
    if (stackLength > 1) {
        char idHead[100] = "";
        if (stack[stackLength - 1].stackType == STACK_TYPE_VAR) {
            strcpy(idHead, stack[stackLength - 1].entitateVar.id);
        }
        if (stack[stackLength - 1].stackType == STACK_TYPE_FUN) {
            strcpy(idHead, stack[stackLength - 1].entitateFunctie.id);
        }

        for (int i = stackLength - 2; i >= 0; i--) {
            if (strcmp(stack[stackLength - 1].numeScope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            if (strcmp(stack[i].numeScope, stack[stackLength - 1].numeScope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                continue;
            }
            //nume
            char idHeadCurrent[100] = "";

            if (stack[i].stackType == STACK_TYPE_VAR) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (stack[i].stackType == STACK_TYPE_FUN) {
                strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
            }

            if (strcmp(idHead, idHeadCurrent) == 0) {
                eroareRedefinireVar(idHead);
            }
        }
    }
}

void eroareRedefinireVar(char *id) {
    printf("\nEROARE LA LINIA %d : Variabila '%s' redefinita.\n", yylineno, id);
    exit(0);
}

void inregistareValoareParametru(char *value) {
    parametriiApelLen += 1;
}

void declarareVariabila(char *tip, char *id, char *valoare, int esteParametru) {
    struct ENTITATE_VAR_STRUCT var;
    strcpy(var.id, id);
    int tipIndex = -1;

    for (unsigned int i = 0; i < VAR_NUME_TIPURI_LEN; i++) {
        if (strcmp(VAR_NUME_TIPURI[i], tip) == 0) {
            tipIndex = i;
            break;
        }
    }
    /* verificam tipurile de variabila simpla (fara class si fara void) */
    if (tipIndex == VAR_TYPE_CLASS || tipIndex == VAR_TYPE_VOID) {
        printf("\nEROARE LA LINIA %d : \n Tipul '%s' nu poate fi declarat aici.\n", yylineno, id);
        exit(0);
    }

    if (tipIndex < 0) {
        printf("\nEROARE LA LINIA %d : \n Tipul '%s' nu exista.\n", yylineno, id);
        exit(0);
    }

    var.tip = tipIndex;
    var.initializat = 0;
    strcpy(var.valoare, valoare);
    strcpy(var.numeScope, scopeStack[currentScopeLength - 1]);
    var.esteParametru = esteParametru;
    var.derivaDinParametru = esteParametru;

    /*TABEL DE SIMBOLURI*/
    strcpy(symbolTable[symbolTableLen].id, var.id);
    strcpy(symbolTable[symbolTableLen].tipEntitate, "variabila");
    strcpy(symbolTable[symbolTableLen].tipDeDate, tip);
    strcpy(symbolTable[symbolTableLen].scope, var.numeScope);
    symbolTable[symbolTableLen].esteParametru = esteParametru;
    symbolTable[symbolTableLen].clasaNivelProtectie = clasaNivelProtectie;
    if (strcmp(valoare, "") == 0) {
        symbolTable[symbolTableLen].initializat = 0;
    } else {
        symbolTable[symbolTableLen].initializat = 1;
        var.initializat = 1;
        strcpy(symbolTable[symbolTableLen].valoare, valoare);
    }
    symbolTableLen += 1;

    printf("scope = '%s' ; variabila: '%s' ; valoare: '%s' \n", var.numeScope, var.id, valoare, symbolTable[symbolTableLen - 1].valoare);
    pushStackVar(var);
}

void asignareVariabila() {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de variabile depasit.\n", yylineno);
        exit(0);
    }
    if (stackLength < 1 || stack[stackLength - 1].stackType != STACK_TYPE_VAR) {
        printf("\nEROARE LA LINIA %d : \n Eroare stack variabila care primeste valoare.\n", yylineno);
        exit(0);
    }
    strcpy(stack[stackLength].numeScope, stack[stackLength - 1].numeScope);
    stack[stackLength].stackType = STACK_TYPE_ASSIGN;
    strcpy(stack[stackLength].entitateVar.id, stack[stackLength - 1].entitateVar.id);
    stackLength += 1;
}

void expresieValoareAsignare(char *valoare) {
    // //cautare asignare in stack
    // int i = stackLength - 1;

    // while(i > 1 && stack[i].stackType != STACK_TYPE_ASSIGN) {
    //     stackLength--;
    //     i--;
    // }
    // if( i > 1 && stack[i].stackType == STACK_TYPE_ASSIGN
    // && stack[i - 1].stackType == STACK_TYPE_VAR
    // ) {
    //     stack[i-1].
    // }
    // if(stackLength < 1 ) {
    //     printf("\nEROARE LA LINIA %d : \n Eroare stack variabila care primeste valoare.\n", yylineno);
    //     exit(0);
    // }
}

void evalNumberExpr(char *result, char *number1, char *operator, char * number2) {
    int numberInt1 = atoi(number1);
    int numberInt2 = atoi(number2);
    int resultInt = 0;

    if (strcmp(operator, "+") == 0) {
        resultInt = numberInt2 + numberInt1;
    }
    if (strcmp(operator, "-") == 0) {
        resultInt = numberInt1 - numberInt2;
    }
    if (strcmp(operator, "*") == 0) {
        resultInt = numberInt1 * numberInt2;
    }
    if (strcmp(operator, "/") == 0) {
        resultInt = numberInt1 / numberInt2;
    }
    size_t stringSize = snprintf(NULL, 0, "%d", resultInt);
    snprintf(result, stringSize + 1, "%d", resultInt);

    // strcpy(result, itoa(resultInt));
}

void evaluareIdCaBool(char *result, char *id) {
    if (stackLength > 1) {
        char scope[100] = "";
        /* scopul este mostenit de la ultimul element din stack */
        strcpy(scope, stack[stackLength - 1].numeScope);

        for (int i = stackLength - 1; i >= 0; i--) {
            if (strcmp(scope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            /*excludem cautarea variabilelor locale din alte functii*/
            if (strcmp(scope, "global") != 0) {
                if (strcmp(stack[i].numeScope, scope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }

            //nume
            char idHeadCurrent[100] = "";

            if (stack[i].stackType == STACK_TYPE_VAR) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (strcmp(id, idHeadCurrent) == 0) {
                struct ENTITATE_VAR_STRUCT entitate = stack[i].entitateVar;
                if (entitate.initializat != 1) {
                    eroareVariabilaNeinitializata(id);
                }
                strcpy(result, entitate.valoare);
                return;
            }
        }
        eroareVariabilaNedefinita(id);
    } else {
        eroareVariabilaNedefinita(id);
    }
}

void evalExpresieBool(char *resultFinal, char *val1, char *operator, char * val2) {
    int numberInt1 = atoi(val1);
    int numberInt2 = atoi(val2);
    if (strcmp(val1, "true") == 0) {
        numberInt1 = 1;
    }
    if (strcmp(val2, "true") == 0) {
        numberInt2 = 1;
    }
    if (strcmp(val1, "false") == 0) {
        numberInt1 = 0;
    }
    if (strcmp(val2, "false") == 0) {
        numberInt2 = 0;
    }
    char result[100] = "";
    if (strcmp(operator, "==") == 0) {
        if (strcmp(val1, val2) == 0) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }
    if (strcmp(operator, "!=") == 0) {
        if (strcmp(val1, val2) == 0) {
            strcpy(result, "false");
        } else {
            strcpy(result, "true");
        }
    }
    if (strcmp(operator, ">") == 0) {
        if (numberInt1 > numberInt2) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }
    if (strcmp(operator, "<") == 0) {
        if (numberInt1 < numberInt2) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }

    if (strcmp(operator, ">=") == 0) {
        if (numberInt1 >= numberInt2) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }

    if (strcmp(operator, "<=") == 0) {
        if (numberInt1 <= numberInt2) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }

    if (strcmp(operator, "and") == 0) {
        if (numberInt1 && numberInt2) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }

    if (strcmp(operator, "or") == 0) {
        if (numberInt1 || numberInt2) {
            strcpy(result, "true");
        } else {
            strcpy(result, "false");
        }
    }
    if (strcmp(result, "") == 0) {
        strcpy(result, "false");
    }
    size_t stringSize = snprintf(NULL, 0, "%s", result);
    snprintf(resultFinal, stringSize + 1, "%s", result);
}

void eroareVariabilaNedefinita(char *id) {
    printStack();
    printf("\nEROARE LA LINIA %d : Variabila '%s' nu este definita in contextul dat.\n", yylineno, id);
    exit(0);
}

void eroareFunctieNedefinita(char *id) {
    printStack();
    printf("\nEROARE LA LINIA %d : Functia '%s' nu este definita in contextul dat.\n", yylineno, id);
    exit(0);
}

void eroareVariabilaNeinitializata(char *id) {
    printStack();
    printf("\nEROARE LA LINIA %d : Variabila '%s' nu a fost initializata.\n", yylineno, id);
    exit(0);
}

int getStackIndexReference(char *id) {
    for (int i = stackLength - 1; i >= 0; i--) {
        char idHeadCurrent[100] = "";
        if (stack[i].stackType == STACK_TYPE_VAR) {
            strcpy(idHeadCurrent, stack[i].entitateVar.id);
        }
        if (stack[i].stackType == STACK_TYPE_FUN) {
            strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
        }
        if (stack[i].stackType == STACK_TYPE_VAR_INNER) {
            strcpy(idHeadCurrent, stack[i].entitateVar.id);
        }
        if (stack[i].stackType == STACK_TYPE_FUN_RETRUN) {
            strcpy(idHeadCurrent, stack[i].entitateVar.id);
        }
        if (strcmp(id, idHeadCurrent) == 0) {
            return i;
        }
    }
    printf("\nEROARE LA LINIA %d : Cautare stack locatie pentru '%s' nu a fost reusit.:442\n", yylineno, id);
    exit(0);

    return 0;
}

int getStackIndexReferenceFun(char *id) {
    for (int i = stackLength - 1; i >= 0; i--) {
        char idHeadCurrent[100] = "";
        if (stack[i].stackType == STACK_TYPE_FUN) {
            strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
        }
        if (strcmp(id, idHeadCurrent) == 0) {
            return i;
        }
    }
    printf("\nEROARE LA LINIA %d : Cautare stack locatie pentru '%s' nu a fost reusit.:442\n", yylineno, id);
    exit(0);

    return 0;
}

void printStack() {
    printf("\n\n--------------stack------------------------------start\n");
    for (unsigned int i = 0; i < stackLength; i++) {
        printf("%d ", i);
        if (i < 10) {
            printf(" ");
        }
        if (strcmp(stack[i].numeScope, "global") != 0) {
            printf("-----");
        }
        if (stack[i].stackType == STACK_TYPE_VAR || stack[i].stackType == STACK_TYPE_VAR_INNER || stack[i].stackType == STACK_TYPE_FUN_RETRUN) {
            if (stack[i].stackType == STACK_TYPE_VAR_INNER) {
                printf("'%s' (Variabila INTERNA) | ", stack[i].entitateVar.id);
            } else {
                if (stack[i].stackType == STACK_TYPE_FUN_RETRUN) {
                    printf("'%s' (RETURN - Variabila INTERNA) | ", stack[i].entitateVar.id);
                } else {
                    printf("'%s' (Variabila) | ", stack[i].entitateVar.id);
                }
            }
            if (stack[i].entitateVar.initializat == 1) {
                printf("valoare: '%s' | ", stack[i].entitateVar.valoare);
            } else {
                printf("valoare: neinitializat | ");
            }
            if (stack[i].entitateVar.esteParametru == 1) {
                printf("parametru: '%d' | ", stack[i].entitateVar.esteParametru);
            }
            if (stack[i].entitateVar.derivaDinParametru == 1) {
                printf("obtinut prin parametrii: '%d' | ", stack[i].entitateVar.derivaDinParametru);
            }
        } else {
            if (stack[i].stackType == STACK_TYPE_FUN) {
                printf("'%s' (Functie) | ", stack[i].entitateFunctie.id);
            } else {
                printf("'%s' (Other type) | ", STACK_TYPE_NAMES[stack[i].stackType]);
            }
        }
        if (stack[i].stackType == STACK_TYPE_ASSIGN) {
            const int stackReference = getStackIndexReference(stack[i].entitateVar.id);
            printf("Index de asignat: '%d' | ", stackReference);
        }
        if (stack[i].stackType == STACK_TYPE_OPERATOR) {
            printf("'%s' | ", stack[i].operator);
        }
        if (stack[i].stackType == STACK_TYPE_VAR_ADDRESS) {
            const int stackReference = getStackIndexReference(stack[i].entitateVar.id);
            if (stackReference < stackLength && stackReference > 0) {
                printf("Index ref: '%d' | Id ref: '%s' |  ", stackReference, stack[stackReference].entitateVar.id);
            }
        }
        if (stack[i].stackType == STACK_TYPE_VALUE) {
            printf("Valoare: '%s' | ", stack[i].entitateVar.valoare);
        }

        printf("Scope '%s' | ", stack[i].numeScope);
        printf("\n");
    }
    printf("--------------stack------------------------------start\n\n");

}

void evalIdNumberExpr(char *result, char *id) {
    if (stackLength > 1) {
        char scope[100] = "";
        /* scopul este mostenit de la ultimul element din stack */
        strcpy(scope, stack[stackLength - 2].numeScope);

        for (int i = stackLength - 1; i >= 0; i--) {
            if (strcmp(scope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            /*excludem cautarea variabilelor locale din alte functii*/
            if (strcmp(scope, "global") != 0) {
                if (strcmp(stack[i].numeScope, scope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }

            //nume
            char idHeadCurrent[100] = "";

            if (stack[i].stackType == STACK_TYPE_VAR) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (stack[i].stackType == STACK_TYPE_FUN) {
                strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
            }
            if (strcmp(id, idHeadCurrent) == 0) {
                if (stack[i].stackType != STACK_TYPE_VAR) {
                    printf("\nEROARE LA LINIA %d : '%s' nu este o variabila.\n", yylineno, id);
                    exit(0);
                }
                struct ENTITATE_VAR_STRUCT entitate = stack[i].entitateVar;
                if (entitate.tip != VAR_TYPE_INT && entitate.tip != VAR_TYPE_FLOAT) {
                    printf("\nEROARE LA LINIA %d : Variabila '%s' are tipul '%s', in timp ce se asteapta o variabila de tip int.\n", yylineno, id, VAR_NUME_TIPURI[entitate.tip]);
                    exit(0);
                }
                if (entitate.initializat != 1) {
                    eroareVariabilaNeinitializata(id);
                }
                strcpy(result, entitate.valoare);
                return;
            }
        }
        eroareVariabilaNedefinita(id);
    } else {
        eroareVariabilaNedefinita(id);
    }
}

void declarareFunctie(char *tip, char *id) {
    struct ENTITATE_FUNCTIE_STRUCT func;

    strcpy(func.id, id);
    int tipIndex = -1;

    for (unsigned int i = 0; i < VAR_NUME_TIPURI_LEN; i++) {
        if (strcmp(VAR_NUME_TIPURI[i], tip) == 0) {
            tipIndex = i;
            break;
        }
    }
    /* verificam tipurile permise */
    if (tipIndex == VAR_TYPE_CLASS) {
        printf("\nEROARE LA LINIA %d : \n Tipul '%s' nu poate fi declarat pentru o functie.\n", yylineno, id);
        exit(0);
    }

    if (tipIndex < 0) {
        printf("\nEROARE LA LINIA %d : \n Tipul '%s' nu exista.\n", yylineno, id);
        exit(0);
    }

    func.tip = tipIndex;

    strcpy(func.numeScope, scopeStack[currentScopeLength - 1]);
    for (unsigned int i = 0; i < currentScopeLength; i++) {
        printf("   ");
    }
    printf("scope = '%s' ; functie: '%s' \n", func.numeScope, func.id);

    strcpy(symbolTable[symbolTableLen].id, func.id);
    strcpy(symbolTable[symbolTableLen].tipEntitate, "functie");
    strcpy(symbolTable[symbolTableLen].tipDeDate, tip);
    strcpy(symbolTable[symbolTableLen].scope, func.numeScope);
    symbolTable[symbolTableLen].clasaNivelProtectie = clasaNivelProtectie;
    symbolTableLen += 1;

    pushStackFunction(func);
}

void declarareClasa(char *id) {
    strcpy(symbolTable[symbolTableLen].id, id);
    strcpy(symbolTable[symbolTableLen].tipEntitate, "clasa");
    symbolTableLen += 1;
}
/*
 marcheaza in ce functie suntem
*/
void pushScope(char *scope) {
    if (currentScopeLength >= MAX_SCOPE_LEN) {
        printf("\nEROARE LA LINIA %d : \nScope depasit\n", yylineno);
        exit(0);
    }
    strcpy(scopeStack[currentScopeLength], scope);
    currentScopeLength += 1;
    printf("\nPUSH '%s'\n", scope);
}

void popScope() {
    /*
        marcam sfarsitul unei serii de instructiuni ale unei functii definite
    */
    if (stackLength >= 1 && strcmp(stack[stackLength - 1].numeScope, "global") != 0) {
        if (stackLength >= MAX_NUM_OF_STACK) {
            printf("\nEROARE LA LINIA %d : \nScope depasit\n", yylineno);
            exit(0);
        }
        stack[stackLength].stackType = STACK_TYPE_FUN_END;
        strcpy(stack[stackLength].numeScope, stack[stackLength - 1].numeScope);
        stackLength += 1;
    }
    if (currentScopeLength > 1) {
        currentScopeLength = currentScopeLength - 1;
    }
}

int uniqueIdkey = 0;
void generareIdNou(char *result) {
    uniqueIdkey++;
    size_t stringSize = snprintf(NULL, 0, "stack-id-%d", uniqueIdkey);
    snprintf(result, stringSize + 1, "stack-id-%d", uniqueIdkey);
}

void inlocuireLinieStack(struct STACK_STRUCT inlocuitor, int index) {
    stack[index].entitateVar = inlocuitor.entitateVar;
    stack[index].entitateFunctie = inlocuitor.entitateFunctie;
    strcpy(stack[index].numeScope, inlocuitor.numeScope);
    strcpy(stack[index].operatie, inlocuitor.operatie);
    strcpy(stack[index].operator, inlocuitor.operator);
    stack[index].blockIndex = inlocuitor.blockIndex;
    stack[index].stackType = inlocuitor.stackType;  //func or var
}

void copiereStackLine(index, index2) {
    stack[index].entitateVar = stack[index2].entitateVar;
    stack[index].entitateFunctie = stack[index2].entitateFunctie;
    strcpy(stack[index].numeScope, stack[index2].numeScope);
    strcpy(stack[index].operatie, stack[index2].operatie);
    strcpy(stack[index].operator, stack[index2].operator);
    stack[index].blockIndex = stack[index2].blockIndex;
    stack[index].stackType = stack[index2].stackType;  //func or var
}

void inlocuireLiniiStack(struct STACK_STRUCT inlocuitor, int indexStart, int indexEnd) {
    if (indexStart > indexEnd || indexStart < 0 || indexStart > stackLength - 1 || indexEnd > stackLength - 1) {
        printStack();
        printf("\nEROARE LA LINIA %d : Eroare inlocuire stack index_start '%s', index_end '%s' .\n", yylineno, indexStart, indexEnd);
        exit(0);
    }
    inlocuireLinieStack(inlocuitor, indexStart);
    int copie = indexStart + 1;
    for (unsigned int ii = indexEnd + 1; ii < stackLength; ii++) {
        copiereStackLine(copie, ii);
        copie++;
    }
    stackLength = stackLength - (indexEnd - indexStart);
}

void stegeLiniiStack(int indexStart, int indexEnd) {
    int copie = indexStart;
    for (unsigned int ii = indexEnd + 1; ii < stackLength; ii++) {
        copiereStackLine(copie, ii);
        copie++;
    }
    stackLength = stackLength - (indexEnd - indexStart) - 1;
}

/*
    
*/
void citireValoareStackLinie(int indexPointer, char *result) {
    char valoare[100] = "";
    if (stack[indexPointer].stackType == STACK_TYPE_VALUE) {
        strcpy(valoare, stack[indexPointer].entitateVar.valoare);
    }
    if (stack[indexPointer].stackType == STACK_TYPE_VAR_ADDRESS) {
        const int indexValoare = getStackIndexReference(stack[indexPointer].entitateVar.id);
        if ((stack[indexValoare].stackType != STACK_TYPE_VAR && stack[indexValoare].stackType != STACK_TYPE_VALUE && stack[indexValoare].stackType != STACK_TYPE_VAR_INNER)) {
            printStack();
            printf("\nEROARE LA LINIA %d : \n Eroare cautare pozitie '%d' in stack.\n", yylineno, indexValoare);
            exit(0);
        }
        if (stack[indexValoare].entitateVar.initializat != 1) {
            eroareVariabilaNeinitializata(stack[indexValoare].entitateVar.id);
        }
        strcpy(valoare, stack[indexValoare].entitateVar.valoare);
    }

    if (stack[indexPointer].stackType == STACK_TYPE_VAR_INNER) {
        rezolvareAsignari(indexPointer + 1, valoare);
    }
    strcpy(result, valoare);
}

void citireOperatieStackLine(int indexPointer, char *result) {
    if (stack[indexPointer].stackType == STACK_TYPE_OPERATOR) {
        strcpy(result, stack[indexPointer].operator);
    } else {
        printStack();
        printf("\nEROARE LA LINIA %d : \n Eroare cautare operator la pozitia '%d' in stack.\n", yylineno, indexPointer);
        exit(0);
    }
}
void rezolvareAsignari(int indexAsignare, char *result) {
    int stackReference = -1;
    char valoare[100] = "";

    /*gasire stackReference - locul in stack unde variabila va fi actualizata */
    if (indexAsignare > 0 && indexAsignare < stackLength - 1 && stack[indexAsignare].stackType == STACK_TYPE_ASSIGN) {
        const int indexVariabila = getStackIndexReference(stack[indexAsignare].entitateVar.id);
        if (indexVariabila >= 0 && indexVariabila < indexAsignare &&
            (stack[indexVariabila].stackType == STACK_TYPE_VAR || stack[indexVariabila].stackType == STACK_TYPE_VAR_INNER)) {
            stackReference = indexVariabila;
        }
    }

    if (stackReference >= 0) {
        /* o asignare simpla al unei valori sau valoarea unei variabile */
        if (indexAsignare + 2 < stackLength && stack[indexAsignare + 2].stackType == STACK_TYPE_ASSIGN_END) {
            if (stack[indexAsignare + 2].stackType == STACK_TYPE_VAR_INNER) {
                printStack();
            }
            citireValoareStackLinie(indexAsignare + 1, valoare);
            stegeLiniiStack(indexAsignare, indexAsignare + 2);
        } else {
            unsigned int i = indexAsignare + 1;
            while (stack[i].stackType != STACK_TYPE_ASSIGN_END) {
                if (i > stackLength - 1) {
                    printf("Eroare while");
                    exit(0);
                }
                char valoare1[100] = "0";
                char valoare2[100] = "0";
                char operator[3] = "+";

                if (strcmp(valoare, "") != 0) {
                    /*valoare1*/
                    strcpy(valoare1, valoare);

                    /*operator*/
                    citireOperatieStackLine(i, operator);

                    /*valoare2*/
                    citireValoareStackLinie(i + 1, valoare2);
                    i += 2;
                } else {
                    /*valoare1*/
                    citireValoareStackLinie(i, valoare1);

                    // /* daca se intampla o atribuire 'a = (2+2+2)', ramane in stack ASSIGN_START-varinner-ASSIGN-END*/
                    if (stack[i + 1].stackType == STACK_TYPE_ASSIGN_END) {
                        strcpy(valoare, valoare1);
                        i += 1;
                        break;
                    }

                    /*operator*/
                    citireOperatieStackLine(i + 1, operator);

                    /*valoare2*/
                    citireValoareStackLinie(i + 2, valoare2);
                    i += 3;
                }

                evalNumberExpr(valoare, valoare1, operator, valoare2);
            }
            stackReference = getStackIndexReference(stack[indexAsignare].entitateVar.id);
            stegeLiniiStack(indexAsignare, i);
        }
        if (strcmp(valoare, "") != 0) {
            strcpy(stack[stackReference].entitateVar.valoare, valoare);
            stack[stackReference].entitateVar.initializat = 1;
            /* daca este declarare este trecut in tabelul de simboluri*/
            if (indexAsignare - 1 == stackReference && stack[stackReference].entitateVar.esteParametru != 1) {
                for (int i = symbolTableLen - 1; i > 0; i--) {
                    if (strcmp(stack[stackReference].entitateVar.id, symbolTable[i].id) == 0
                    && strcmp(stack[stackReference].numeScope, symbolTable[i].scope) == 0
                    ) {
                        strcpy(symbolTable[i].valoare, valoare);
                        symbolTable[i].initializat = 1;
                        break;
                    }
                }
            }
        }
    } else {
        printStack();
        printf("\nEROARE LA LINIA %d : \n Referinta negasita pentru linia de asignare '%d' :781.\n", yylineno, indexAsignare);
        exit(0);
    }
    strcpy(result, valoare);
    printStack();
}

void endAssign(char *id) {
    if (strcmp(id, "return") == 0) {
        if (returnIdsLen < 1) {
            printf("\nEROARE LA LINIA %d : \n Eroare stack pentru return statements '%s'.\n", yylineno, id);
            exit(0);
        }
        char newId[100] = "";
        strcpy(newId, returnIds[returnIdsLen - 1]);
        returnIdsLen -= 1;
        endAssign(newId);
        return;
    }

    int indexAsignare = -1;
    if (stackLength >= 3) {
        for (int i = stackLength - 1; i >= 0; i--) {
            if (stack[i].stackType == STACK_TYPE_ASSIGN) {
                const int indexVariabila = getStackIndexReference(stack[i].entitateVar.id);
                if (indexVariabila >= 0 && indexVariabila < i && (stack[indexVariabila].stackType == STACK_TYPE_VAR || stack[indexVariabila].stackType == STACK_TYPE_FUN_RETRUN

                                                                  )) {
                    //nume
                    char idHeadCurrent[100] = "";
                    strcpy(idHeadCurrent, stack[indexVariabila].entitateVar.id);
                    if (strcmp(idHeadCurrent, id) == 0) {
                        indexAsignare = i;
                        break;
                    }
                }
            }
        }
    }

    if (indexAsignare > -1) {
        if (stackLength < MAX_NUM_OF_STACK) {
            stack[stackLength].stackType = STACK_TYPE_ASSIGN_END;
            strcpy(stack[stackLength].numeScope, stack[indexAsignare].numeScope);
            strcpy(stack[stackLength].entitateVar.id, id);
            stackLength += 1;

            char valoare[100] = "";

            unsigned int expresieNereductibila = 0;
            /* operatia are loc global */
            if (strcmp(stack[indexAsignare].numeScope, "global") == 0) {
            } else {
                /* daca e un asignment in cadrul unei functii
                assignment-ul nu este calculabil daca
                - contine parametrii sau variabile obtinute din parametrii -> atunci marcam si prezentul id daca este local
                - contine variabile global (evitam schimbarea lor la definirea functiei)
                - daca variabila interna este pentru returnare in functie
                daca nu e calculabil - pastram doar instructiunile
                */
                int stackReference = getStackIndexReference(id);
                /* variabila asignata este globa: NU EXECUTAM INSTRUCTIUNI */
                if (strcmp(stack[stackReference].entitateVar.numeScope, "global") == 0) {
                    expresieNereductibila = 1;
                } else {
                    /* variabila de asignata este variabila interna de return */
                    if (stack[stackReference].stackType == STACK_TYPE_FUN_RETRUN) {
                        expresieNereductibila = 1;
                    }
                    printf("\n\n............%d....%s............................................\n\n", stack[stackReference].stackType, stack[stackReference].entitateVar.id);
                    /* variabila de asignata este parametru: NU EXECUTAM INSTRUCTIUNI */
                    if (stack[stackReference].entitateVar.derivaDinParametru == 1) {
                        expresieNereductibila = 1;
                    }
                    /* variabila de asignata este determinata de parametrii: NU EXECUTAM INSTRUCTIUNI */
                    if (stack[stackReference].entitateVar.esteParametru == 1) {
                        expresieNereductibila = 1;
                    }
                    /* variabila de asignata primeste o expresie in care apar variabile globale / parametrii / variabile determinate: NU EXECUTAM INSTRUCTIUNI */
                    for (unsigned int i = indexAsignare + 1; i < stackLength && expresieNereductibila == 0; i++) {
                        if (stack[i].stackType == STACK_TYPE_VAR ||
                            stack[i].stackType == STACK_TYPE_VAR_INNER ||
                            stack[i].stackType == STACK_TYPE_VAR_ADDRESS ||
                            stack[i].stackType == STACK_TYPE_FUN_RETRUN) {
                            int auxIndex = i;
                            if (stack[i].stackType == STACK_TYPE_VAR_ADDRESS) {
                                auxIndex = getStackIndexReference(stack[i].entitateVar.id);
                            }
                            if (strcmp(stack[auxIndex].entitateVar.numeScope, "global") == 0) {
                                expresieNereductibila = 1;
                                stack[stackReference].entitateVar.derivaDinParametru = 1;
                                break;
                            } else {
                                if (stack[auxIndex].entitateVar.esteParametru == 1 || stack[auxIndex].entitateVar.derivaDinParametru == 1) {
                                    expresieNereductibila = 1;
                                    stack[stackReference].entitateVar.derivaDinParametru = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (expresieNereductibila == 0) {
                rezolvareAsignari(indexAsignare, valoare);
            }
        } else {
            printf("\nEROARE LA LINIA %d : \n Stack depasit negasita pentru asignarea '%s' ::888.\n", yylineno, id);
            exit(0);
        }
    } else {
        printf("\nEROARE LA LINIA %d : \n Asignare negasita pentru '%s' ::917.\n", yylineno, id);
        exit(0);
    }
}

void startAssignExpr(char *id) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    if (stackLength >= 1) {
        //scope
        char scope[100] = "";
        strcpy(scope, stack[stackLength - 1].numeScope);

        // if(strcmp(scope, "global") != 0) {
        //     printStack();
        //     printf("\nhere-------------");
        //     exit(0);
        // }
        if (strcmp(id, "return") == 0) {
            if (strcmp(scope, "global") == 0) {
                printf("\nEROARE LA LINIA %d : \n Incercare return in afara unei functii.\n", yylineno);
                exit(0);
            }
            /* cautam functia */
            for (int i = stackLength - 1; i >= 0; i--) {
                if (stack[i].stackType == STACK_TYPE_FUN || stack[i].stackType == STACK_TYPE_FUN_CALL &&
                                                                strcmp(stack[i].entitateFunctie.id, scope) == 0) {
                    if (stack[i].entitateFunctie.tip == VAR_TYPE_VOID) {
                        printf("\nEROARE LA LINIA %d : \n Incercare return intr-o functie void.\n", yylineno);
                        exit(0);
                    }

                    /* verificare tip potrivire.... */
                    // if(stack[i].entitateFunctie.tip == VAR_TYPE_VOID) {
                    //     printf("\nEROARE LA LINIA %d : \n Incercare return intr-o functie void.\n", yylineno);
                    //     exit(0);
                    // }

                    //cream o variabila interna noua
                    if (stackLength >= MAX_NUM_OF_STACK) {
                        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
                        exit(0);
                    }
                    if (returnIdsLen >= MAX_RETRUN_IDS) {
                        printf("\nEROARE LA LINIA %d : \n Overflow de stack return statements.\n", yylineno);
                        exit(1);
                    }
                    char newId[100] = "";
                    generareIdNou(newId);

                    struct ENTITATE_VAR_STRUCT var;
                    strcpy(var.id, newId);
                    var.tip = 0;
                    var.initializat = 0;
                    strcpy(var.numeScope, scope);
                    pushStackVar(var);
                    stack[stackLength - 1].stackType = STACK_TYPE_FUN_RETRUN;

                    strcpy(returnIds[returnIdsLen], newId);
                    returnIdsLen += 1;
                    startAssignExpr(newId);
                    return;
                }
            }
        }

        for (int i = stackLength - 1; i >= 0; i--) {
            //verificare scope
            if (strcmp(scope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            if (strcmp(stack[i].numeScope, scope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                continue;
            }
            //nume
            char idHeadCurrent[100] = "";
            if (stack[i].stackType == STACK_TYPE_VAR) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (stack[i].stackType == STACK_TYPE_VAR_INNER) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (stack[i].stackType == STACK_TYPE_FUN_RETRUN) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (stack[i].stackType == STACK_TYPE_FUN) {
                strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
            }
            //gasit
            if (strcmp(id, idHeadCurrent) == 0) {
                if (stack[i].stackType != STACK_TYPE_VAR && stack[i].stackType != STACK_TYPE_VAR_INNER && stack[i].stackType != STACK_TYPE_FUN_RETRUN) {
                    printf("\nEROARE LA LINIA %d : \n Asignare invalida pentru: '%s'.\n", yylineno, id);
                    exit(0);
                }
                stack[stackLength].stackType = STACK_TYPE_ASSIGN;
                strcpy(stack[stackLength].entitateVar.id, id);
                strcpy(stack[stackLength].numeScope, scope);
                stackLength += 1;
                printStack();
                return;
            }
        }
        eroareVariabilaNedefinita(id);
    } else {
        eroareVariabilaNedefinita(id);
    }
    printStack();
}

void apelFunctie(char *id) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    /* cautam functia pe stack */
    if (stackLength > 1) {
        //scope
        char scope[100] = "global";
        for (int i = stackLength - 1; i >= 0; i--) {
            //verificare scope
            if (strcmp(scope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            if (strcmp(stack[i].numeScope, scope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                continue;
            }

            char idHeadCurrent[100] = "";
            //nume
            if (stack[i].stackType == STACK_TYPE_FUN) {
                strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
            }
            //gasit
            if (strcmp(id, idHeadCurrent) == 0) {
                /* marcam inceput de apel pentru a inregistra si valida parametrii mai departe */
                stack[stackLength].stackType = STACK_TYPE_FUN_CALL;
                strcpy(stack[stackLength].entitateFunctie.id, stack[i].entitateFunctie.id);
                stack[stackLength].entitateFunctie.tip = stack[i].entitateFunctie.tip;
                strcpy(stack[stackLength].numeScope, scope);
                stackLength += 1;

                /* verificam daca functia este folosita intr-o asignare */
                if (stackLength > 1 && stack[stackLength - 2].stackType == STACK_TYPE_ASSIGN) {
                    if (stack[i].entitateFunctie.tip == VAR_TYPE_VOID) {
                        printf("\nEROARE LA LINIA %d : \n Eroare asignare a valorii unei functii void.\n", yylineno);
                        exit(0);
                    }
                }
                return;
            }
        }
        eroareFunctieNedefinita(id);
    } else {
        eroareFunctieNedefinita(id);
    }
}

void executaInstructiuneCall(int indexCallStart) {
    if (indexCallStart >= 0 && indexCallStart < stackLength - 1 &&
        stack[indexCallStart].stackType == STACK_TYPE_FUN_CALL) {
        int indexInstr = indexCallStart + 1;
        char resultRetrun[100] = "";

        while (stack[indexInstr].stackType != STACK_TYPE_FUN_CALL_END) {
            printStack();
            if (stack[indexInstr].stackType == STACK_TYPE_ASSIGN) {
                char resultAsignare[100] = "";
                rezolvareAsignari(indexInstr, resultAsignare);
                indexInstr--;
                continue;
            }
            /* gasim o instructiune de return - oprim executia */
            if (stack[indexInstr].stackType == STACK_TYPE_FUN_RETRUN) {
                /*  
                    intructiunea de return are mereu un asigment asociat (o expresie, fie ea constanta)
                    marchez ca variabila simplu pentru ca evaluatorul sa nu opreasca rezolvarea expresiei 
                */
                stack[indexInstr].stackType = STACK_TYPE_VAR;

                if (stack[indexInstr + 1].stackType == STACK_TYPE_ASSIGN) {
                    rezolvareAsignari(indexInstr + 1, resultRetrun);
                } else {
                    printf("\nEROARE LA LINIA %d : \n Eroare la retrun stack - asignare negasita.\n", yylineno);
                    exit(0);
                }
                break;
            }
            indexInstr++;
        }
        /* la sfarsit: 
            1. stergem instructiunile ramase (declararile sau instructiunile ramase dupa return)
            2. rezultatul instructiunii return intr-o variabila interna (globala sau locala in functie variabila care primeste asignarea) */
        /* NOTE: chiar daca e de tip void rezultatul va fi cuantificam intr-o variabila */
        char scopeApel[100] = "";
        strcpy(scopeApel, stack[indexCallStart].numeScope);

        stackLength = indexCallStart;  //1.
        //2.
        stack[stackLength].stackType = STACK_TYPE_VALUE;
        char idNou[100] = "";
        generareIdNou(idNou);
        strcpy(stack[stackLength].entitateVar.id, idNou);

        if (strcmp(resultRetrun, "") != 0) {
            stack[stackLength].entitateVar.initializat = 1;
            strcpy(stack[stackLength].entitateVar.valoare, resultRetrun);
        } else {
            stack[stackLength].entitateVar.initializat = 0;
        }
        strcpy(stack[stackLength].numeScope, scopeApel);
        stackLength += 1;
        printStack();
        printf("\n EVALUARE FUNCTIE INSTRUCTIUNI - END - return '%s'\n", resultRetrun);
        // exit(1);
    } else {
        printf("\nEROARE LA LINIA %d : \n Eroare executie instructiuni stack.\n", yylineno);
        exit(0);
    }
}

void endApelFunctie(char *id) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    /* cautam apelul de functie pe stack */
    if (stackLength > 1) {
        //scope
        char scope[100] = "global";
        for (int i = stackLength - 1; i >= 0; i--) {
            //verificare scope
            if (strcmp(scope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            if (strcmp(stack[i].numeScope, scope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                continue;
            }

            char idHeadCurrent[100] = "";
            //nume
            if (stack[i].stackType == STACK_TYPE_FUN_CALL) {
                strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
            }
            //gasit
            if (strcmp(id, idHeadCurrent) == 0) {
                /* copiem toate instructiunile din stack */
                stack[stackLength].stackType = STACK_TYPE_FUN_CALL_END;
                strcpy(stack[stackLength].entitateFunctie.id, stack[i].entitateFunctie.id);
                strcpy(stack[stackLength].numeScope, scope);
                stackLength += 1;

                /* creeze un scope nou de evaluare in care voi include elementele copiate din functia originala */
                char scopeNou[100] = "";
                generareIdNou(scopeNou);

                /* evaluez parametrii */
                int stackReferenceFun = getStackIndexReferenceFun(stack[i].entitateFunctie.id);
                /* scope original - numele functiei - */
                char scopeOriginal[100] = "";
                strcpy(scopeOriginal, stack[stackReferenceFun].entitateFunctie.id);

                int indexRefFuncInstruc = stackReferenceFun + 1;
                int indexValParametriiStack = i + 1;

                while (
                    stack[indexRefFuncInstruc].stackType == STACK_TYPE_VAR &&
                    stack[indexRefFuncInstruc].entitateVar.esteParametru) {
                    if (stack[indexValParametriiStack].stackType == STACK_TYPE_FUN_CALL_END) {
                        printf("\nEROARE LA LINIA %d : \n Parametrii insuficienti pentru apel '%s'.\n", yylineno, stack[stackReferenceFun].entitateFunctie.id);
                        exit(0);
                    }
                    /* copii numele variabilelor locale de tip parametru - vor avea scope diferit */
                    strcpy(stack[indexValParametriiStack].entitateVar.id,
                           stack[indexRefFuncInstruc].entitateVar.id);
                    strcpy(stack[indexValParametriiStack].numeScope, scopeNou);
                    strcpy(stack[indexValParametriiStack].entitateVar.numeScope, scopeNou);

                    /* verificam daca parametrii sunt de acelasi tip */
                    unsigned int cParamNumeric = 0;
                    unsigned int cValueNumeric = 1;
                    if(stack[indexRefFuncInstruc].entitateVar.tip == VAR_TYPE_INT
                    || stack[indexRefFuncInstruc].entitateVar.tip == VAR_TYPE_FLOAT
                    ) {
                        cParamNumeric = 1;
                    }
                    char valoareCurrenta[200] = "";
                    strcpy(valoareCurrenta, stack[indexValParametriiStack].entitateVar.valoare);
                    if(
                        strcmp(valoareCurrenta, "true") == 0
                    ||
                        strcmp(valoareCurrenta, "false") == 0
                    ) {
                        cValueNumeric = 0;
                    }
                    if(cValueNumeric != cParamNumeric) {
                        printf("\nEROARE LA LINIA %d : \n Parametru '%s' nu primeste o valoare de tip corect.\n", yylineno, stack[indexRefFuncInstruc].entitateVar.id);
                        exit(0);
                    }

                    indexRefFuncInstruc++;
                    indexValParametriiStack++;
                }
                if (indexValParametriiStack >= stackLength ||
                    stack[indexValParametriiStack].stackType != STACK_TYPE_FUN_CALL_END) {
                    printf("\nEROARE LA LINIA %d : \n Prea multi parametrii pentru apel '%s'.\n", yylineno, stack[stackReferenceFun].entitateFunctie.id);
                    exit(0);
                }
                /* 
                    copii instructiunile functiei
                    creez un scope nou ca sa i-au doar variabilele din acest block
                */
                /* sterg linia de end a call-ului */
                stackLength = stackLength - 1;
                /* copii instrutiuni */
                while (stack[indexRefFuncInstruc].stackType != STACK_TYPE_FUN_END) {
                    if (stackLength >= MAX_NUM_OF_STACK) {
                        printf("\nEROARE LA LINIA %d : \n Stack depasit la executia apelului functiei '%s'.\n", yylineno, scopeOriginal);
                        exit(0);
                    }
                    copiereStackLine(stackLength, indexRefFuncInstruc);
                    /* inlocuiesc scope-ul original cu cel nou */
                    if (strcmp(stack[stackLength].numeScope, scopeOriginal) == 0) {
                        strcpy(stack[stackLength].numeScope, scopeNou);
                        strcpy(stack[stackLength].entitateFunctie.numeScope, scopeNou);
                        strcpy(stack[stackLength].entitateVar.numeScope, scopeNou);
                    }
                    stackLength++;
                    /* la sfarsit inserez si STACK_TYPE_FUN_END stack line */
                    if (stack[indexRefFuncInstruc + 1].stackType == STACK_TYPE_FUN_END) {
                        if (stackLength >= MAX_NUM_OF_STACK) {
                            printf("\nEROARE LA LINIA %d : \n Stack depasit la executia apelului functiei '%s' :1135.\n", yylineno, scopeOriginal);
                            exit(0);
                        }
                        stack[stackLength].stackType = STACK_TYPE_FUN_CALL_END;
                        stackLength++;
                    }
                    indexRefFuncInstruc++;
                }
                printf("\n EVALUARE FUNCTIE INSTRUCTIUNI - START - \n");
                executaInstructiuneCall(i);
                printStack();

                return;
            }
        }
        eroareFunctieNedefinita(id);
    } else {
        eroareFunctieNedefinita(id);
    }
}

void pushExpresieValuePeStack(char *valoare) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    if (stackLength > 1) {
        //verificam daca ultimul element djn stack este o asignare sau operatie
        if (stack[stackLength - 1].stackType != STACK_TYPE_ASSIGN && stack[stackLength - 1].stackType != STACK_TYPE_OPERATOR) {
            printf("\nEROARE LA LINIA %d : \n Eroare de stack, adaugare valoare '%s', fara asignare.\n", yylineno, valoare);
            exit(0);
        }
        //scope
        char scope[100] = "";
        strcpy(scope, stack[stackLength - 1].numeScope);
        stack[stackLength].stackType = STACK_TYPE_VALUE;
        strcpy(stack[stackLength].entitateVar.valoare, valoare);
        strcpy(stack[stackLength].entitateVar.numeScope, scope);
        char idNou[100] = "";
        generareIdNou(idNou);
        strcpy(stack[stackLength].entitateVar.id, idNou);

        stack[stackLength].entitateVar.initializat = 1;
        strcpy(stack[stackLength].entitateVar.id, stack[stackLength - 1].entitateVar.id);
        strcpy(stack[stackLength].numeScope, scope);
        stackLength += 1;
    } else {
        printf("\nEROARE LA LINIA %d : \n Eroare de stack, adaugare valoare '%s', fara asignare.\n", yylineno, valoare);
        exit(0);
    }
    printStack();
}
/* expresii de tip numeric */
void pushExpresieIdPeStack(char *id) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    if (stackLength > 1) {
        //verificam daca ultimul element djn stack este o asignare sau operatie
        if (stack[stackLength - 1].stackType != STACK_TYPE_ASSIGN && stack[stackLength - 1].stackType != STACK_TYPE_OPERATOR) {
            printf("\nEROARE LA LINIA %d : \n Eroare de stack, adaugare valoare expresiei '%s', fara asignare.\n", yylineno, id);
            exit(0);
        }
        //scope
        char scope[100] = "";
        strcpy(scope, stack[stackLength - 1].numeScope);
        for (int i = stackLength - 1; i >= 0; i--) {
            //verificare scope
            if (strcmp(scope, "global") == 0) {
                if (strcmp(stack[i].numeScope, "global") != 0) {
                    continue;
                }
            }
            if (strcmp(stack[i].numeScope, scope) != 0 && strcmp(stack[i].numeScope, "global") != 0) {
                continue;
            }

            //nume
            char idHeadCurrent[100] = "";
            if (stack[i].stackType == STACK_TYPE_VAR) {
                strcpy(idHeadCurrent, stack[i].entitateVar.id);
            }
            if (stack[i].stackType == STACK_TYPE_FUN) {
                strcpy(idHeadCurrent, stack[i].entitateFunctie.id);
            }
            //gasit
            if (strcmp(id, idHeadCurrent) == 0) {
                if (stack[i].stackType != STACK_TYPE_VAR) {
                    printf("\nEROARE LA LINIA %d : \n Asignare invalida pentru expresia '%s'.\n", yylineno, id);
                    exit(0);
                }

                /* verific daca variabila e de tip numeric */
                if (stack[i].entitateVar.tip != VAR_TYPE_INT && stack[i].entitateVar.tip != VAR_TYPE_FLOAT) {
                    printf("\nEROARE LA LINIA %d : \n '%s' trebuie sa fie de tip numeric .\n", yylineno, id);
                    exit(0);
                }

                stack[stackLength].stackType = STACK_TYPE_VAR_ADDRESS;
                strcpy(stack[stackLength].entitateVar.id, stack[i].entitateVar.id);

                strcpy(stack[stackLength].numeScope, scope);
                stackLength += 1;
                return;
            }
        }
        eroareVariabilaNedefinita(id);
    } else {
        eroareVariabilaNedefinita(id);
    }
    printStack();
}

void pushOperatorStack(char *operator) {
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    if (stackLength > 1) {
        //scope
        char scope[100] = "";
        strcpy(scope, stack[stackLength - 1].numeScope);

        stack[stackLength].stackType = STACK_TYPE_OPERATOR;
        strcpy(stack[stackLength].entitateVar.id, stack[stackLength - 1].entitateVar.id);

        strcpy(stack[stackLength].numeScope, scope);
        strcpy(stack[stackLength].operator, operator);

        stackLength += 1;
    } else {
        printf("\nEROARE LA LINIA %d : \n Eroare de stack operator push.\n", yylineno);
        exit(0);
    }
}

/*asigneaza o variabila noua si un block de asignare care vor fi eliminate dupa calcularea operatiei din paranteza */
/* pentru expresii de tip integere */
void pushStackParan() {
    if (paranIdsLen >= MAX_PARAN_IDS) {
        printf("\nEROARE LA LINIA %d : \n Eroare de stack paranteze.\n", yylineno);
        exit(1);
    }
    if (stackLength >= MAX_NUM_OF_STACK) {
        printf("\nEROARE LA LINIA %d : \n Numar maxim de stack depasit.\n", yylineno);
        exit(0);
    }
    if (stackLength > 1) {
        char newId[100] = "";
        generareIdNou(newId);

        struct ENTITATE_VAR_STRUCT var;
        strcpy(var.id, newId);
        var.tip = 0;
        var.initializat = 0;
        strcpy(var.numeScope, scopeStack[currentScopeLength - 1]);
        pushStackVar(var);

        strcpy(paranIds[paranIdsLen], newId);
        paranIdsLen += 1;

        stack[stackLength - 1].stackType = STACK_TYPE_VAR_INNER;

        startAssignExpr(newId);
    } else {
        printf("\nEROARE LA LINIA %d : \n Numar de stack.\n", yylineno);
        exit(0);
    }
    printStack();
}

void popStackParan(char *tipVarCreata) {
    char id[100] = "";
    if (paranIdsLen < 1) {
        printf("\nEROARE LA LINIA %d : \n Eroare paranteze.\n", yylineno, id);
        exit(1);
    }
    strcpy(id, paranIds[paranIdsLen - 1]);
    paranIdsLen -= 1;

    int indexAsignare = -1;
    int stackToBeUpdated = -1;
    if (stackLength > 3) {
        for (int i = stackLength - 1; i >= 0; i--) {
            if (stack[i].stackType == STACK_TYPE_ASSIGN) {
                const int indexVariabila = getStackIndexReference(stack[i].entitateVar.id);
                if (indexVariabila >= 0 && indexVariabila < i && stack[indexVariabila].stackType == STACK_TYPE_VAR_INNER) {
                    //nume
                    char idHeadCurrent[100] = "";
                    strcpy(idHeadCurrent, stack[indexVariabila].entitateVar.id);
                    if (strcmp(idHeadCurrent, id) == 0) {
                        indexAsignare = i;
                        stackToBeUpdated = indexVariabila;
                        break;
                    }
                }
            }
        }
    }

    if (indexAsignare > -1) {
        if (stackLength < MAX_NUM_OF_STACK) {
            stack[stackLength].stackType = STACK_TYPE_ASSIGN_END;
            strcpy(stack[stackLength].numeScope, stack[indexAsignare].numeScope);
            stackLength += 1;

            /* daca variabila interna este destinata unui parametru */
            if (strcmp(tipVarCreata, "fun_call") == 0) {
                stack[stackToBeUpdated].stackType = STACK_TYPE_VAR;
                endAssign(id);
                /* un artificiu pentru ca se intampla ca stck assign_end sa ramana in stack cand este apelat de aici */
                stackLength -= 1;
            }

        } else {
            printf("\nEROARE LA LINIA %d : \n Stack depasit negasita pentru asignarea '%s' ::1400.\n", yylineno, id);
            exit(0);
        }
    } else {
        printStack();
        printf("\nEROARE LA LINIA %d : \n Asignare negasita pentru '%s'::1260.\n", yylineno, id);
        exit(0);
    }
}