%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define UNKNOWN 1
#define OPERATOR 18
#define DELIMITER 19
#define KEYWORD 20
#define ID 21
#define ILLEGAL_ID 22
#define NUMBER 23
#define CHAR 24
#define STRING 25

int lines,chars_last_line,characters,keywords,numbers,chars,strings,ids,operators,delimiters;

struct Error{
    int row,col;
    struct Error *next;
};
struct Error *head;

void errorStore(int row,int col);
%}

keyword short|int|long|double|float|unsigned|signed|char|typedef|sizeof|struct|enum|union|const|volatile|auto|register|static|extern|if|else|switch|case|default|for|do|while|continue|break|void|return|goto

delim [ \t]
ws {delim}+

digit [0-9]
oct_digit [0-7]
hex_digit [a-fA-F0-9]
oct_num 0{oct_digit}+
hex_num 0x{hex_digit}+
num {digit}+(\.{digit}+)?(E[+\-]?{digit}+)?

letter [_A-Za-z]
escape "\\"|"\\'"|"\\\""|"\\?"|"\\a"|"\\b"|"\\f"|"\\n"|"\\r"|"\\t"|"\\v"
char '([^']|{escape})'
string \"([^\"]|{escape})*\"

id {letter}({letter}|{digit})*
illegal_id {digit}({letter}|{digit})*
op "+"|"-"|"*"|"/"|"%"|"++"|"--"|"="|"<"|"<="|">"|">="|"=="|"!="|"&&"|"||"|"!"|"&"|"|"|"~"|"^"|"<<"|">>"|"?"|":"
delimiter [;,\.\{\}\(\)\[\]]

%s PREPROCESSOR
%s LINECOMMENT
%s BLOCKCOMMENT
%%

<INITIAL>#          {BEGIN PREPROCESSOR;}
<INITIAL>"//"       {BEGIN LINECOMMENT;}
<INITIAL>"/*"       {BEGIN BLOCKCOMMENT;}
<PREPROCESSOR>\n    {lines++; BEGIN INITIAL;}
<PREPROCESSOR>.     {;}
<LINECOMMENT>\n     {lines++; BEGIN INITIAL;}
<LINECOMMENT>.      {;}
<BLOCKCOMMENT>"*/"  {BEGIN INITIAL;}
<BLOCKCOMMENT>\n    {lines++;}
<BLOCKCOMMENT>.     {;}

<INITIAL>{ws}               ;
<INITIAL>"\n"               { lines++; chars_last_line=characters; }
<INITIAL>{keyword}          { characters+=yyleng; keywords++; return(KEYWORD); }
<INITIAL>{oct_num}          { characters+=yyleng; numbers++; return(NUMBER); }
<INITIAL>{hex_num}          { characters+=yyleng; numbers++; return(NUMBER); }
<INITIAL>{num}              { characters+=yyleng; numbers++; return(NUMBER); }
<INITIAL>{char}             { characters+=yyleng; characters++; return(CHAR); }
<INITIAL>{string}           { characters+=yyleng; strings++; return(STRING); }
<INITIAL>{id}               { characters+=yyleng; ids++; return(ID); }
<INITIAL>{illegal_id}       { characters+=yyleng; errorStore(lines+1,characters-chars_last_line-yyleng+1); return(ILLEGAL_ID); }
<INITIAL>{op}               { characters+=yyleng; operators++; return(OPERATOR); }
<INITIAL>{delimiter}        { characters+=yyleng; delimiters++; return(DELIMITER); }
<INITIAL>.                  { characters+=yyleng; errorStore(lines+1,characters-chars_last_line-yyleng+1); return(UNKNOWN); }

%%

void errorStore(int row,int col){
    if(head==NULL){
        head=(struct Error *)malloc(sizeof(struct Error));
        head->next=NULL;
    }
    struct Error *p,*cur=head;
    if(p=(struct Error *)malloc(sizeof(struct Error))){
      p->row=row;
      p->col=col;
      p->next=NULL;
    }
    while(cur){
        if(cur->next==NULL){
            cur->next=p;
            break;
        }
        cur=cur->next;
    }
}

void writeout(int c){
    fprintf(yyout,"[LOCATION] %d:%d\n",lines+1,characters-chars_last_line-yyleng+1);
    switch(c){
        case KEYWORD: fprintf(yyout,"[TYPE] KEYWORD, [VALUE] %s\n",yytext);break; 
        case NUMBER: fprintf(yyout,"[TYPE] NUMBER, [VALUE] %s\n",yytext);break;
        case CHAR: fprintf(yyout,"[TYPE] CHAR, [VALUE] %s\n",yytext);break;
        case STRING: fprintf(yyout,"[TYPE] STRING, [VALUE] %s\n",yytext);break;
        case OPERATOR: fprintf(yyout,"[TYPE] OPERATOR, [VALUE] %s\n",yytext);break;
        case DELIMITER: fprintf(yyout,"[TYPE] DELIMITER, [VALUE] %s\n",yytext);break;
        case ID: fprintf(yyout,"[TYPE] ID, [VALUE] %s\n",yytext);break;
        case ILLEGAL_ID: fprintf(yyout,"[TYPE] ILLEGAL_ID, [VALUE] %s\n",yytext);break;
        case UNKNOWN: fprintf(yyout,"[TYPE] UNKNOWN, [VALUE] %s\n",yytext);break;
    }
    fprintf(yyout,"\n");
}

void printAnalyses(){
    fprintf(yyout,
    "===========Analysis==========\n\
    [Rows] %d\n\
    [Characters] %d\n\
    [Keyword] %d\n\
    [Number] %d\n\
    [Character Constant] %d\n\
    [String Literal] %d\n\
    [Operator] %d\n\
    [Delimiter] %d\n\
    [Identifier] %d\n\
    [ERRORS]\n",
    lines,characters,keywords,numbers,chars,strings,ids,operators,delimiters);
    if(head){
        struct Error *cur=head->next;
        while(cur){
            fprintf(yyout,"   !!!ERROR AT %d:%d!!!    \n",cur->row,cur->col);
            cur=cur->next;
        }
    }
}

int main(int argc, char ** argv){
	int c;
	if(argc>=2){
	  if((yyin=fopen(argv[1], "r")) == NULL){
	    printf("Can't open file %s\n", argv[1]);
	    return 1;
	  }
	  if(argc>=3){
	    yyout=fopen(argv[2], "w");
	  }
	}
	while(c=yylex()){
		writeout(c);
	}
    printAnalyses();
	if(argc>=2){
	  fclose(yyin);
	  if(argc>=3) fclose(yyout);
	}
	return 0;
}