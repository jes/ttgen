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

enum type { VARIABLE, OPERATOR, BRACKET, NOT };
enum oper { OP_OR, OP_AND, OP_XOR, OP_NAND, OP_NOR, OP_IMP };

typedef struct Node {
  char type;/* VARIABLE or OPERATOR */
  char not;/* negation flag */
  int id;/* variable or operator index */
  struct Node *parent;/* pointer to parent node */
  struct Node *child[2];/* pointers to child nodes */
} Node;

typedef struct Token {
  char *text;/* the actual text of the token */
  char type;/* VARIABLE, OPERATOR, BRACKET, or NOT */
} Token;

/* array of operator names */
static char *operator[] = { "OR", "AND", "XOR", "NAND", "NOR", "IMP", NULL };

/* array of variable names, for looking up id's
 * unused entries are NULL */
#define VAR_MAX 64
static char *variable[VAR_MAX];
static int num_vars;

/* store raw text input expressions */
static char input[1024];

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

/* return the next token available in the global string "input", or NULL if
   the end of the string is reached. The returned token should be free'd with
   free_token() */
static Token *next_token(void) {
  static char *p = NULL;
  Token *t;
  size_t len;
  int i;

  /* starting a new string */
  if(!p) p = input;

  /* eat whitespace */
  p += strspn(p, WHITESPACE);

  /* reached the end of the string? */
  if(!*p) {
    p = NULL;
    return NULL;
  }

  t = malloc(sizeof(Token));

  /* check for brackets */
  if(*p == '(' || *p == ')') {
    t->text = malloc(2);
    t->text[0] = *p;
    t->text[1] = '\0';
    t->type = BRACKET;
    p++;
    return t;
  }

  /* not a bracket, copy the word */
  len = strcspn(p, WHITESPACE BRACKETS);
  t->text = malloc(len + 1);
  strncpy(t->text, p, len);
  t->text[len] = '\0';
  p += len;

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

/* make a zero-filled node for the expression tree */
static Node *make_node(void) {
  Node *n;

  n = malloc(sizeof(Node));
  memset(n, 0, sizeof(Node));
  return n;
}

/* make both child nodes for the given node */
static void branch(Node *n) {
  n->child[0] = make_node();
  n->child[0]->parent = n;
  n->child[1] = make_node();
  n->child[1]->parent = n;
}

/* evalute the expression tree, with variable n corresponding to the nth bit
   of 'bits' with 0 being the least significant */
static int evaluate(Node *tree, uint64_t bits) {
  int a, b, r;

  if(tree->type == VARIABLE) {
    r = !!(bits & (1 << tree->id));
  } else {
    a = evaluate(tree->child[LEFT], bits);
    b = evaluate(tree->child[RIGHT], bits);

    switch(tree->id) {
      case OP_OR:   r = a || b;    break;
      case OP_AND:  r = a && b;    break;
      case OP_XOR:  r = a != b;    break;
      case OP_NAND: r = !(a && b); break;
      case OP_NOR:  r = !(a || b); break;
      case OP_IMP:  r = b || !a;   break;
    }
  }

  return r ^ tree->not;
}

/* prints out the truth table for the given boolean expression tree */
static void print_table(Node *tree) {
  uint64_t i;
  uint64_t b;
  int var_len[num_vars];

  for(i = 0; i < num_vars; i++) {
    var_len[i] = strlen(variable[i]);
    printf("%s ", variable[i]);
  }
  printf("\n");

  /* NOTE: comparison between i and -1 works because of overflow */
  for(i = (1 << num_vars) - 1; i != -1; i--) {
    for(b = 0; b < num_vars; b++) {
      printf("%-*d ", var_len[b], !!(i & (1 << b)));
    }

    printf("  ");
    printf("%d\n", evaluate(tree, i));
  }
}

int main(int argc, char **argv) {
  Token *t;
  Node *tree;
  int i;

  tree = make_node();
  tree->parent = tree;

  while(fgets(input, 1024, stdin)) {
    /* repeatedly read tokens and store them in the expression tree */
    while((t = next_token())) {
      switch(t->type) {
        case BRACKET:
          if(*t->text == '(') {
            /* branch and descend left */
            branch(tree);
            tree = tree->child[LEFT];
          } else /* *t->text == ')' */{
            /* ascend */
            tree = tree->parent;
          }
          break;

        case VARIABLE:
          /* store variable and ascend */
          tree->type = VARIABLE;
          tree->id = var_id(t->text);
          tree = tree->parent;
          break;

        case OPERATOR:
          /* store operator and descend right */
          tree->type = OPERATOR;
          tree->id = oper_id(t->text);
          tree = tree->child[RIGHT];
          break;

        case NOT:
          /* toggle negation flag */
          tree->not = !tree->not;
          break;
      }

      free_token(t);
    }

    /* print the truth table */
    print_table(tree);

    /* clear out the variables */
    for(i = 0; i < num_vars; i++) {
      variable[i] = NULL;
    }
  }

  return 0;
}
