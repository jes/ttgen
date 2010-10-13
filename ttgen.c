/* ttgen - truth table generator for boolean logic expressions

   James Stanley 2010 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define WHITESPACE " \n\t"
#define BRACKETS   "()"

enum type { UNKNOWN=0, VARIABLE, OPERATOR, LPAREN, RPAREN, NOT, SLASHVARS };
enum oper { OP_OR, OP_AND, OP_XOR, OP_NAND, OP_NOR, OP_IMP };

typedef struct Node {
  char type;/* UNKNOWN, VARIABLE, OPERATOR, or NOT */
  int id;/* variable or operator index */
} Node;

typedef struct Token {
  char *text;/* the actual text of the token */
  char type;/* VARIABLE, OPERATOR, LPAREN, RPAREN, or NOT */
} Token;

/* array of operator names */
static char *operator[] = { "OR", "AND", "XOR", "NAND", "NOR", "IMP", NULL };

#define STACK_MAX 128

/* expression nodes */
Node *node;
int np;

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

  /* check for single-character tokens */
  if(strchr("()/", *input_ptr)) {
    t->text = malloc(2);
    t->text[0] = *input_ptr;
    t->text[1] = '\0';
    /* select token type */
    switch(*input_ptr) {
      case '(': t->type = LPAREN;    break;
      case ')': t->type = RPAREN;    break;
      case '/': t->type = SLASHVARS; break;
    }
    input_ptr++;
    return t;
  }

  /* not a bracket or a slash, copy the word */
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
  node = NULL;
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
  else if(t->type == VARIABLE) n->id = var_id(t->text);

  free_token(t);
}

/* evaluate the expression with the variable values given in 'bits'.
 * return -1 on stack overflow, -2 on underflow, and -3 if there is more than
 * one value left on the stack at the end */
static int evaluate(uint64_t bits) {
  char stack[STACK_MAX];
  int sp = 0;
  int i, r;
  int a, b;

  /* for each expression node */
  for(i = 0; i < np; i++) {
    switch(node[i].type) {
      case VARIABLE:
        /* push variable value */
        if(sp >= STACK_MAX) return -1;
        stack[sp++] = !!(bits & (1 << node[i].id));
        break;

      case OPERATOR:
        /* pop operands */
        if(sp <= 1) return -2;
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
        if(sp >= STACK_MAX) return -1;
        stack[sp++] = r;
        break;

      case NOT:
        if(sp <= 0) return -2;
        stack[sp - 1] = !stack[sp - 1];
        break;
    }
  }

  if(sp != 1) return -3;

  return stack[--sp];
}

/* print the truth table for the expression */
static void print_table(void) {
  uint64_t i;
  uint64_t b;
  int var_len[num_vars];
  int fail;

  /* HACK: see if the stack is going to fail before printing the variables */
  if((fail = evaluate(0)) < 0) {
    if(fail == -1) fprintf(stderr, "Stack overflow.\n");
    else if(fail == -2) fprintf(stderr, "Stack underflow.\n");
    else if(fail == -3) fprintf(stderr, "Stack not empty.\n");
    return;
  }

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
  Token *stack[STACK_MAX];
  int sp = 0;
  int first_token;
  int slashvar_mode;

#define PUSH(t) if(sp >= STACK_MAX) {                   \
                  fprintf(stderr, "Stack overflow.\n"); \
                  goto cleanup;                         \
                }                                       \
                stack[sp++] = (t);

  /* TODO: use getline() or similar */
  while(fgets(input, 1024, stdin)) {
    first_token = 1;
    slashvar_mode = 0;

    /* repeatedly read tokens and use the output() function to convert from RPN
     * to an expression tree */
    while((t = next_token())) {
      if(slashvar_mode) {
        /* define the order of variables */
        if(t->type == VARIABLE) var_id(t->text);
      } else {
        /* expression evaluation mode */
        switch(t->type) {
          case VARIABLE:
            /* output variable */
            output(t);
            break;

          case OPERATOR:
            /* output operators from the top of the stack */
            while(sp && (stack[sp-1]->type == OPERATOR
                      || stack[sp-1]->type == NOT)) {
              output(stack[--sp]);
            }
            /* push operator */
            PUSH(t);
            break;

          case LPAREN:
            /* push lparen */
            PUSH(t);
            break;

          case RPAREN:
            /* pop operators until LPAREN encountered */
            while(sp && (stack[sp-1]->type != LPAREN)) {
              output(stack[--sp]);
            }
            /* if stack runs out without finding an LPAREN, parentheses are
             * mismatched */
            if(sp == 0) {
              fprintf(stderr, "Mismatched parentheses.\n");
              goto cleanup;
            }
            /* pop and discard LPAREN */
            free_token(stack[--sp]);
            /* free RPAREN */
            free_token(t);
            break;

          case NOT:
            /* push operator */
            PUSH(t);
            break;

          case SLASHVARS:
            /* enter slashvar mode */
            if(!first_token) {
              fprintf(stderr, "Slashvars can not be embedded in expressions.\n");
              goto cleanup;
            }
            slashvar_mode = 1;
            break;
        }
      }

      /* not the first token any more */
      first_token = 0;
    }

    /* don't print out truth tables or wipe out variables */
    if(slashvar_mode) {
      input_ptr = NULL;/* reset token stream without clearing variables */
      continue;
    }

    /* while operators left on stack, output them */
    while(sp) {
      /* if any LPAREN's are on the stack, parentheses are mismatched */
      if(stack[sp-1]->type == LPAREN) {
        fprintf(stderr, "Mismatched parentheses.\n");
        goto cleanup;
      }
      /* output operator */
      output(stack[--sp]);
    }

    /* print the truth table */
    print_table();

   cleanup:
    /* free the stack */
    while(sp) {
      free_token(stack[--sp]);
    }
    /* reset the token stream */
    reset_tokstr();
    /* free the expression nodes */
    free_nodes();

    printf("\n");
 }

  return 0;
}
