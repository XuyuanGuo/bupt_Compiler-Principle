%{
#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin,*yyout;
int yylex(void);
void yyerror(char *);
%}

%token NUM

%%

E : E '+' T     {fprintf(yyout,"E->E+T\n");}
  | E '-' T     {fprintf(yyout,"E->E-T\n");}
  | T           {fprintf(yyout,"E->T\n");}
  ;

T : T '*' F     {fprintf(yyout,"T->T*F\n");}
  | T '/' F     {fprintf(yyout,"T->T/F\n");}
  | F           {fprintf(yyout,"T->F\n");}
  ;

F : '(' E ')'   {fprintf(yyout,"F->(E)\n");}
  | NUM         {fprintf(yyout,"F->NUM\n");}
  ;

%%

void yyerror(char* str){
    fprintf(stderr,"error:%s\n",str);
} 

int main(int argc,char** argv) {
    if(argc>=2){
	  if((yyin=fopen(argv[1], "r")) == NULL){
	    printf("Can't open file %s\n", argv[1]);
	    return 1;
	  }
	  if(argc>=3){
	    yyout=fopen(argv[2], "w");
	  }
	}
    yyparse();
    return 0;
}
