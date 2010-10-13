/* ttgen - truth table generator for boolean logic expressions

   James Stanley 2010 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define WHITESPACE " \n\t"
#define BRACKETS   "()"

#define LEFT  0
#define RIGHT 1

enum type { UNKNOWN=0, VARIABLE, OPERATOR, LPAREN, RPAREN, NOT };
enum oper { OP_OR, OP_AND, OP_XOR, OP_NAND, OP_NOR, OP_IMP };
enum expect { EXPR, OPER };

typedef struct Node {
  char type;/* UNKNOWN, VARIABLE, OPERATOR, or NOT */
  int id;/* variable or operator index */
} Node;

typedef struct Token {
  char *text;/* the actual text of the token */
  char type;/* VARIABLE, OPERATOR, LPAREN, RPAREN, or NOT */
} Token;

/* expression nodes */
Node *node;
int np;

/* array of operator names */
static char *operator[] = { "OR", "AND", "XOR", "NAND", "NOR", "IMP", NULL };

/* array of variable names, for looking up id's
 * unused entries are NULL */
#define VAR_MAX 64
static char *variable[VAR_MAX];
static int num_vars;

/* store raw text input expressions */
static char input[1024];
static char *input_ptr;

/* print the given message to stderr and exit with code 1 */
static void die(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  exit(1);
}

/* return the variable id for the given variable name, creating if necessary */
static int var_id(const char *var_name) {
  int i;

  for(i = 0; (i < VAR_MAX) && (variable[i]); i++) {
    if(strcmp(var_name, variable[i]) == 0) return i;
  }

  if(i == VAR_MAX) die("error: maximum of %d variables\n", VAR_MAX);

  variable[i] = strdup(var_name);
  num_vars++;
  return i;
}

/* return the operator id for the given name, or -1 if none */
static int oper_id(const char *oper_name) {
  int i;

  for(i = 0; operator[i]; i++) {
    if(strcasecmp(oper_name, operator[i]) == 0) return i;
  }

  return -1;
}

/* resets the token stream so that it is ready for another string and clears
 * all the variables */
static void reset_tokstr(void) {
  int i;

  input_ptr = NULL;

  /* clear out the variables */
  for(i = 0; i < num_vars; i++) {
    free(variable[i]);
    variable[i] = NULL;
  }
  num_vars = 0;
}

/* return the next token available in the global string "input", or NULL if
 * the end of the string is reached. The returned token should be free'd with
 * free_token() */
static Token *next_token(void) {
  Token *t;
  size_t len;
  int i;

  /* starting a new string */
  if(!input_ptr) input_ptr = input;

  /* eat whitespace */
  input_ptr += strspn(input_ptr, WHITESPACE);

  /* reached the end of the string? */
  if(!*input_ptr) {
    input_ptr = NULL;
    return NULL;
  }

  t = malloc(sizeof(Token));

  /* check for brackets */
  if(*input_ptr == '(' || *input_ptr == ')') {
    t->text = malloc(2);
    t->text[0] = *input_ptr;
    t->text[1] = '\0';
    t->type = (*input_ptr == '(') ? LPAREN : RPAREN;
    input_ptr++;
    return t;
  }

  /* not a bracket, copy the word */
  len = strcspn(input_ptr, WHITESPACE BRACKETS);
  t->text = malloc(len + 1);
  strncpy(t->text, input_ptr, len);
  t->text[len] = '\0';
  input_ptr += len;

  /* special-case unary operator */
  if(strcasecmp(t->text, "NOT") == 0) {
    t->type = NOT;
    return t;
  }

  /* check for operators */
  for(i = 0; operator[i]; i++) {
    if(strcasecmp(t->text, operator[i]) == 0) {
      t->type = OPERATOR;
      return t;
    }
  }

  /* only remaining possibility is a variable */
  t->type = VARIABLE;
  return t;
}

/* free's the given token, including it's text field */
static void free_token(Token *t) {
  free(t->text);
  free(t);
}

/* free the expression nodes */
static void free_nodes(void) {
  free(node);
  np = 0;
}

/* pass tokens to this as if they were being output in RPN, and this function
 * builds the appropriate expression tree */
static void output(Token *t) {
  Node *n;

  /* make another expression node */
  node = realloc(node, sizeof(Node) * (np + 1));

  /* get a pointer to the next node */
  n = node + np++;

  /* assign type and id */
  n->type = t->type;
  if(t->type == OPERATOR) n->id = oper_id(t->text);
  if(t->type == VARIABLE) n->id = var_id(t->text);

  free_token(t);
}

/* evaluate the expression with the variable values given in 'bits' */
static int evaluate(uint64_t bits) {
  char stack[128];
  int sp = 0;
  int i, r;
  int a, b;

  /* for each expression node */
  for(i = 0; i < np; i++) {
    switch(node[i].type) {
      case VARIABLE:
        /* push variable value */
        stack[sp++] = !!(bits & (1 << node[i].id));
        break;

      case OPERATOR:
        /* pop operands */
        b = stack[--sp];
        a = stack[--sp];

        /* calculate result */
        switch(node[i].id) {
          case OP_OR: r = a || b; break;
          case OP_AND: r = a && b; break;
          case OP_XOR: r = a != b; break;
          case OP_NAND: r = !(a && b); break;
          case OP_NOR: r = !(a || b); break;
          case OP_IMP: r = !a || b; break;
        }

        /* push result */
        stack[sp++] = r;
        break;

      case NOT:
        stack[sp - 1] = !stack[sp - 1];
        break;
    }
  }

  return stack[--sp];
}

/* print the truth table for the expression */
static void print_table(void) {
  uint64_t i;
  uint64_t b;
  int var_len[num_vars];

  for(i = 0; i < num_vars; i++) {
    var_len[i] = strlen(variable[i]);
    printf("%s ", variable[i]);
  }
  printf("\n");

  /* NOTE: comparison between i and -1 works because of
   * overflow */
  for(i = (1 << num_vars) - 1; i != -1; i--) {
    for(b = 0; b < num_vars; b++) {
        printf("%-*d ", var_len[b], !!(i & (1 << b)));
    }

    printf(" ");
    printf("%d\n", evaluate(i));
  }
}

int main(int argc, char **argv) {
  Token *t;
  Token *stack[128];
  int sp = 0;

  while(fgets(input, 1024, stdin)) {
    /* repeatedly read tokens and use the output() function to convert from RPN
     * to an expression tree */
    /* TODO: error-checking */
    while((t = next_token())) {
      switch(t->type) {
        case VARIABLE:
          /* output variable */
          output(t);
          break;

        case OPERATOR:
          /* output operators from the top of the stack */
          while(stack[sp-1]->type == OPERATOR || stack[sp-1]->type == NOT) {
            output(stack[--sp]);
          }
          /* push operator */
          stack[sp++] = t;
          break;

        case LPAREN:
          /* push lparen */
          stack[sp++] = t;
          break;

        case RPAREN:
          /* pop operators until LPAREN encountered */
          while(stack[sp-1]->type != LPAREN) {
            if(stack[sp-1]->type != OPERATOR) {
              fprintf(stderr, "WHAT THE FUCK\n");
            }
            output(stack[--sp]);
          }
          /* pop and discard LPAREN */
          free_token(stack[--sp]);
          /* free RPAREN */
          free_token(t);
          break;

        case NOT:
          /* push operator */
          stack[sp++] = t;
          break;
      }
    }

    /* while operators left on stack, output them */
    while(sp) {
      if(stack[sp-1]->type != OPERATOR && stack[sp-1]->type != NOT) {
        fprintf(stderr, "NON-OPERATOR LEFT ON STACK!!!\n");
      }
      /* output operator */
      output(stack[--sp]);
    }

    /* print the truth table */
    print_table();

    cleanup:
    reset_tokstr();
    free_nodes();
    printf("\n");
 }

  return 0;
}
