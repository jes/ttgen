/* ttgen - truth table generator for boolean logic expressions

   James Stanley 2010 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

enum type { VARIABLE, OPERATOR };
enum oper { OP_OR, OP_AND, OP_XOR, OP_NAND, OP_NOR, OP_IMP };

typedef struct Node {
  char type;/* VARIABLE or OPERATOR */
  char not;/* negation flag */
  int id;/* variable or operator index */
  Node *parent;/* pointer to parent node */
  Node *child[2];
} Node;

/* array of operator names */
char *operator[] = { "OR", "AND", "XOR", "NAND", "NOR", "IMP", NULL };

/* array of variable names, for looking up id's
 * unused entries are NULL */
#define VAR_MAX 64
char *variable[VAR_MAX];

/* print the given message to stderr and exit with code 1 */
void die(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  exit(1);
}

/* return the variable id for the given variable name, creating if necessary */
int var_id(const char *var_name) {
  int i;

  for(i = 0; (i < VAR_MAX) && (variable[i]); i++) {
    if(strcmp(var_name, variable[i]) == 0) return i;
  }

  if(i == VAR_MAX) die("error: maximum of %d variables\n", VAR_MAX);

  variable[i] = strdup(var_name);
  return i;
}

int main(int argc, char **argv) {
}
