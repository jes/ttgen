%{
#include <stdio.h>

void yyerror(const char *s) {
  fprintf(stderr, "yyerror: %s\n", s);
}

int yywrap(void) {
  return 1;
}
%}

%token OPER_OR OPER_AND OPER_XOR OPER_NAND OPER_NOR OPER_IMP OPER_NOT VARIABLE
%token LPAREN RPAREN

%%

input: input line
     | '\n'
     | /* empty */
     ;

line: expr '\n'           { print_table($1); }
    ;

operator: OPER_OR
        | OPER_AND
        | OPER_XOR
        | OPER_NAND
        | OPER_NOR
        | OPER_IMP
        ;

expr: LPAREN expr operator expr RPAREN
    | OPER_NOT expr
    | VARIABLE
    ;

%%

int main(int argc, char **argv) {
  yyparse();
  return 0;
}
