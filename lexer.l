%{
#include "y.tab.h"
%}

%%

"("                   { return LPAREN;    }
")"                   { return RPAREN;    }
"or "                 { return OPER_OR;   }
"and"                 { return OPER_AND;  }
"xor"                 { return OPER_XOR;  }
"nand"                { return OPER_NAND; }
"nor"                 { return OPER_NOR;  }
"imp"                 { return OPER_IMP;  }
"not"                 { return OPER_NOT;  }
[A-Za-z][A-Za-z0-9_]* { return VARIABLE;  }
[ \t]+                ;
.|\n                  { return *yytext;   }
