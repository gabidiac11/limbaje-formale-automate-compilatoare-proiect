# lfac-proiect
### Instructiuni:
```
yacc -d tema.y; 
lex tema.l; 
gcc lex.yy.c y.tab.c -o tema.bin; 
```
### Instructiuni coroborate:
```
yacc -d -v tema.y; lex tema.l; gcc lex.yy.c y.tab.c -o tema.bin; 
./tema.bin ./tema.txt;
```