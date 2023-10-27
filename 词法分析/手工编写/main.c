#include<stdio.h>
#include<stdlib.h>
#include<string.h>

// 定义宏来表示左缓冲区和右缓冲区
#define L_HALF_BUF 0
#define R_HALF_BUF 1

// 定义宏来表示不同的错误类型
#define UNKNOWN 0
#define ILLEGAL_NUM 1
#define ILLEGAL_CHAR 2
#define ILLEGAL_STRING 3

// 文件指针和相关的全局变量
FILE *in,*out;
int state,iskey;
int lines,cols,cols_copy,characters,keywords,numbers,chars,strings,ids,operators,delimiters;
char C;
char buffer[2048];   // 缓冲区
char token[256],*forward;
char keyword_table[32][10]={"short","int","long","double","float","unsigned","signed","char","typedef","sizeof","struct","enum","union","const","volatile","auto","register","static","extern","if","else","switch","case","default","for","do","while","continue","break","void","return","goto"};

// 错误结构
struct Error{
    int row,col;
    char *explaination;
    struct Error *next;
};
struct Error *head;

// 根据类型刷新缓冲区
void refresh(int type){
    char ch;
    if(type==L_HALF_BUF){
        for(int i=0;i<1023;i++){
            ch=fgetc(in);
            buffer[i]=ch;
            if(ch==EOF) break;
        }
        buffer[1023]=EOF;
    }else{
        for(int i=1024;i<2047;i++){
            ch=fgetc(in);
            buffer[i]=ch;
            if(ch==EOF) break;
        }
        buffer[2047]=EOF;
    }
}

// 获取下一个字符
void get_char(){
    if(!forward){
        forward=buffer;
        refresh(L_HALF_BUF);
    }
    if(*forward==EOF){
        if(forward==buffer+1023){
            refresh(R_HALF_BUF);
            forward++;
        }else if(forward==buffer+2047){
            refresh(L_HALF_BUF);
            forward=buffer;
        }
    }
    C=*forward;
    forward++;
    if(C=='\n'){
        lines++;
        cols_copy=cols;
        cols=0;
    }
    characters++;
    cols++;
}

// 获取下一个非空白字符
void get_nbc(){
    while(C==' '||C=='\t'||C=='\n'){
        get_char();
    }
}

// 连接当前字符到token
void cat(){
    int length=strlen(token);
    token[length]=C;
    token[length+1]='\0';
}

// 判断是否为数字
int is_digit(){
    return C>='0'&&C<='9';
}

// 判断是否为八进制数字
int is_oct_digit(){
    return C>='0'&&C<='7';
}

// 判断是否为十六进制数字
int is_hex_digit(){
    return C>='0'&&C<='9'||C>='A'&&C<='F'||C>='a'&&C<='f';
}

// 判断是否为转义字符
int is_escape(){
    return C=='\\'||C=='\''||C=='"'||C=='?'||C=='a'||C=='b'||C=='f'||C=='n'||C=='r'||C=='t'||C=='v'||C=='0';
}

// 判断字符是否不为单引号
int is_non_sing_quote(){
    return C!='\'';
}

// 判断字符是否不为双引号
int is_non_pl_quote(){
    return C!='"';
}

// 判断是否为字母或下划线
int is_letter(){
    return C=='_'||C>='A'&&C<='Z'||C>='a'&&C<='z';
}

// 回退一个字符并更新相应的行列统计
void retract(){
    forward--; // 回退指针
    if(C=='\n'){
        lines--;
        cols=cols_copy;
        characters--;
    }
    else{
        cols--;
        characters--;
    }
}

// 检查token是否是关键字，并返回其在keyword_table中的索引。如果不是关键字，则返回-1。
int reserve(){
    for(int i=0;i<32;i++){
        if(strcmp(token,keyword_table[i])==0)
            return i;
    }
    return -1;
}

// 将token从字符串形式转换为整数。考虑十进制、十六进制和八进制的情况。
int SToI(){
    int value=0;
    if(token[0]=='0'){
        if(token[1]=='x'){ // 十六进制
            for(int i=0;token[i];i++){
                if(token[i]>='0'&&token[i]<='9'){
                    value=value*16+(token[i]-'0');
                }else{
                    switch(token[i]){
                        // 对应十六进制中的a-f或A-F
                        case 'a':
                        case 'A':
                            value=value*16+10;
                            break;
                        case 'b':
                        case 'B':
                            value=value*16+11;
                            break;
                        case 'c':
                        case 'C':
                            value=value*16+12;
                            break;
                        case 'd':
                        case 'D':
                            value=value*16+13;
                            break;
                        case 'e':
                        case 'E':
                            value=value*16+14;
                            break;
                        case 'f':
                        case 'F':
                            value=value*16+15;
                    }
                }
            }
        }else{ // 八进制
            for(int i=0;token[i];i++){
                value=value*8+(token[i]-'0');
            }
        }
    }else{ // 十进制
        for(int i=0;token[i];i++){
            value=value*10+(token[i]-'0');
        }
    }
    return value;
}

// 将token从字符串形式转换为浮点数
float SToF(){
    int i=0;
    float value=0;
    for(;token[i]!='.';i++){ // 读取小数点前的部分
        value=value*10+(token[i]-'0');
    }
    i++;
    for(int j=10;token[i];i++,j*=10){ // 读取小数点后的部分
        value=value+1.0*(token[i]-'0')/j;
    }
    return value;
}

// 将token从科学计数法的字符串形式转换为浮点数
float SToE(){
    int i=0,E_value=0,flag=0;
    float value=1,F_value=0;
    for(;token[i]!='.'&&token[i]!='E';i++){ // 读取E前面的部分
        F_value=F_value*10+(token[i]-'0');
    }
    if(token[i]=='.'){ // 读取小数点和E之间的部分
        i++;
        for(int j=10;token[i]!='E';i++,j*=10){
            F_value=F_value+1.0*(token[i]-'0')/j;
        }
    }
    i++;
    if(token[i]=='+'){ // 考虑正指数
        i++;
    }else if(token[i]=='-'){ // 考虑负指数
        i++;
        flag=1;
    }
    for(;token[i];i++){ // 读取E后面的指数部分
        E_value=E_value*10+(token[i]-'0');
    }
    while(E_value--){ // 根据指数值，计算最终浮点数
        value=value*F_value;
    }
    return value;
}

// 插入表项的函数，但还未实现
int table_insert(){

}

// 错误处理函数，将发生的错误类型和位置添加到错误链表中
void error(int type){
    // 如果错误链表头为空，则初始化
    if(!head){
        head=(struct Error*)malloc(sizeof(struct Error));
        head->next=NULL;
    }
    // 找到链表的尾部
    struct Error *tail=head, *p=(struct Error*)malloc(sizeof(struct Error));
    while(tail->next){
        tail=tail->next;
    }
    // 设置新错误节点的位置信息
    p->row=lines+1;
    p->col=cols+1;
    p->next=NULL;
    // 根据错误类型设置错误描述
    switch(type){
        case UNKNOWN:
            p->explaination="Unknown symbol.";
            break;
        case ILLEGAL_NUM:
            p->explaination="Illegal number.";
            break;
        case ILLEGAL_CHAR:
            p->explaination="Illegal character constant.";
            while(C!='\''&&C!='\n'){
                get_char();
            }
            break;
        case ILLEGAL_STRING:
            p->explaination="Illegal string literal.";
            while(C!='"'&&C!='\n'){
                get_char();
            }
            break;
    }
    // 将新错误节点连接到链表的尾部
    tail->next=p;
}

// 输出token的信息到输出文件
void write_out(char* type, void* value){
    fprintf(out, "[ROW] %d, [COL] %d\n", lines+1, cols+1);
    // 根据token类型输出相应的值
    // 这里进行了多种类型的检查并相应地格式化输出
    // 例如：如果是整数，就输出其整数值；如果是字符串，则输出字符串值。
    if(strcmp(type,"num_I")==0||strcmp(type,"oct_num")==0||strcmp(type,"hex_num")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %d\n",type,*(int*)value);
    }else if(strcmp(type,"num_F")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %f\n",type,*(float*)value);
    }else if(strcmp(type,"num_E")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %f\n",type,*(float*)value);
    }else if(strcmp(type,"char")==0||strcmp(type,"string")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %s\n",type,(char*)value);
    }else if(strcmp(type,"id")==0||strcmp(type,"keyword")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %s\n",type,(char*)value);
    }else if(strcmp(type,"arithmetic_op")==0||strcmp(type,"relational_op")==0||strcmp(type,"assignment_op")==0||strcmp(type,"logical_op")==0||strcmp(type,"bitwise_op")==0||strcmp(type,"ternary_op")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %s\n",type,(char*)value);
    }else if(strcmp(type,"delimiter")==0){
        fprintf(out,"[TYPE] %s, [VALUE] %c\n",type,*(char*)value);
    }else{
        fprintf(out,"[TYPE] %s, [VALUE] %c\n",type,*(char*)value);
    }
    fprintf(out,"\n");
}

// 打印词法分析的统计结果到输出文件
void printAnalyses(){
    fprintf(out,
    "===========Analysis==========\n\
    [Rows] %d\n\
    [Characters] %d\n\
    [Keyword] %d\n\
    [Number] %d\n\
    [Character Constant] %d\n\
    [String Literal] %d\n\
    [Operator] %d\n\
    [Delimiter] %d\n\
    [Identifier] %d\n",
    lines, characters, keywords, numbers, chars, strings, operators, delimiters, ids);
    
    // 如果存在错误链表，则输出所有错误的位置和描述
    if(head){
        fprintf(out, "[ERRORS]\n");
        struct Error *cur = head->next;
        while(cur){
            fprintf(out, "   !!!ERROR AT %d:%d!!!    \n   EXPLAINATION: %s\n", cur->row, cur->col, cur->explaination);
            cur=cur->next;
        }
    }
}

//主函数，使用有限状态机进行词法分析，详见文档说明
int main(int argc, char ** argv){
    if(argc>=2){
	    if((in=fopen(argv[1], "r")) == NULL){
            printf("Can't open file %s\n", argv[1]);
            return 1;
        }
        if(argc>=3){
            out=fopen(argv[2], "w");
        }
    }
    do{
        switch(state){
            case 0:
                token[0]='\0';
                get_char();
                get_nbc();
                if(C==EOF) break;
                if(C>='1'&&C<='9'){
                    state=1;
                }else if(C=='0'){
                    state=7;
                }else if(C=='\''){
                    state=9;
                }else if(C=='"'){
                    state=12;
                }else if(is_letter()){
                    state=13;
                }else if(C=='+'){
                    state=14;
                }else if(C=='-'){
                    state=15;
                }else if(C=='*'||C=='%'){
                    state=0;
                    switch(C){
                        case '*':
                            operators++;
                            write_out("arithmetic_op",(void*)"*");
                            break;
                        case '%':
                            operators++;
                            write_out("arithmetic_op",(void*)"%");
                            break;
                    }
                    
                }else if(C=='<'){
                    state=16;
                }else if(C=='>'){
                    state=17;
                }else if(C=='='){
                    state=18;
                }else if(C=='!'){
                    state=19;
                }else if(C=='&'){
                    state=20;
                }else if(C=='|'){
                    state=21;
                }else if(C=='/'){
                    state=22;
                }else if(C=='^'||C=='~'){
                    state=0;
                    switch(C){
                        case '^':
                            operators++;
                            write_out("bitwise_op",(void*)"^");
                            break;
                        case '~':
                            operators++;
                            write_out("bitwise_op",(void*)"~");
                            break;
                    }
                }else if(C==':'||C=='?'){
                    state=0;
                    switch(C){
                        case ':':
                            operators++;
                            write_out("bitwise_op",(void*)"^");
                            break;
                        case '?':
                            operators++;
                            write_out("bitwise_op",(void*)"~");
                            break;
                    }
                }else if(C==';'||C==','||C=='.'||C=='('||C==')'||C=='['||C==']'||C=='{'||C=='}'){
                    state=0;
                    delimiters++;
                    write_out("delimiter",&C);
                }else{
                    state=0;
                    error(UNKNOWN);
                    write_out("unknown",&C);
                }
                break;
            case 1:
                cat();
                get_char();
                if(is_digit()){
                    state=1;
                }else if(C=='.'){
                    state=2;
                }else{
                    retract();
                    state=0;
                    int value=SToI();
                    numbers++;
                    write_out("num_I",&value);
                }
                break;
            case 2:
                cat();
                get_char();
                if(is_digit()){
                    state=3;
                }else{
                    state=0;
                    error(ILLEGAL_NUM);
                    write_out("unknown",&C);
                }
                break;
            case 3:
                cat();
                get_char();
                if(is_digit()){
                    state=3;
                }else if(C=='E'){
                    state=4;
                }else{
                    retract();
                    state=0;
                    float vaule=SToF();
                    numbers++;
                    write_out("num_F",&vaule);
                }
                break;
            case 4:
                cat();
                get_char();
                if(is_digit()){
                    state=6;
                }else if(C=='+'||C=='-'){
                    state=5;
                }else{
                    state=0;
                    error(ILLEGAL_NUM);
                    write_out("unknown",&C);
                }
                break;
            case 5:
                cat();
                get_char();
                if(is_digit()){
                    state=6;
                }else{
                    state=0;
                    error(ILLEGAL_NUM);
                    write_out("unknown",&C);
                }
                break;
            case 6:
                cat();
                get_char();
                if(is_digit()){
                    state=6;
                }else{
                    retract();
                    state=0;
                    float vaule=SToE();
                    numbers++;
                    write_out("num_E",&vaule);
                }
                break;
            case 7:
                cat();
                get_char();
                if(is_oct_digit()){
                    state=7;
                }else if(C=='x'){
                    state=8;
                }else{
                    retract();
                    state=0;
                    int vaule=SToI();
                    numbers++;
                    write_out("oct_num",&vaule);
                }
                break;
            case 8:
                cat();
                get_char();
                if(is_hex_digit()){
                    state=8;
                }else{
                    retract();
                    state=0;
                    int vaule=SToI();
                    numbers++;
                    write_out("hex_num",&vaule);
                }
                break;
            case 9:
                cat();
                get_char();
                if(C=='\\'){
                    state=10;
                }else if(is_non_sing_quote()){
                    state=11;
                }else{
                    state=0;
                    error(ILLEGAL_CHAR);
                    write_out("unknown",&C);
                }
                break;
            case 10:
                cat();
                get_char();
                if(is_escape()){
                    state=11;
                }else{
                    state=0;
                    error(ILLEGAL_CHAR);
                    write_out("unknown",&C);
                }
                break;
            case 11:
                cat();
                get_char();
                if(C=='\''){
                    state=0;
                    cat();
                    chars++;
                    write_out("char",token);
                }else{
                    state=0;
                    error(ILLEGAL_CHAR);
                    write_out("unknown",&C);
                }
                break;
            case 12:
                cat();
                get_char();
                if(C=='"'){
                    state=0;
                    cat();
                    strings++;
                    write_out("string",token);
                }else if(C!='\n'&&is_non_pl_quote()){
                    state=12;
                }else{
                    state=0;
                    error(ILLEGAL_STRING);
                    write_out("unknown",&C);
                }
                break;
            case 13:
                cat();
                get_char();
                if(is_letter()||is_digit()){
                    state=13;
                }else{
                    retract();
                    state=0;
                    iskey=reserve();
                    if(~iskey){
                        keywords++;
                        write_out("keyword",token);
                    }else{
                        ids++;
                        write_out("id",token);
                    }
                }
                break;
            case 14:
                cat();
                get_char();
                if(C=='+'){
                    state=0;
                    operators++;
                    write_out("arithmetic_op",(void*)"++");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("arithmetic_op",(void*)"+");
                }
                break;
            case 15:
                cat();
                get_char();
                if(C=='-'){
                    state=0;
                    operators++;
                    write_out("arithmetic_op",(void*)"--");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("arithmetic_op",(void*)"-");
                }
                break;
            case 16:
                cat();
                get_char();
                if(C=='='){
                    state=0;
                    operators++;
                    write_out("relational_op",(void*)"<=");
                }else if(C=='<'){
                    state=0;
                    operators++;
                    write_out("bitwise_op",(void*)"<<");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("relational_op",(void*)"<");
                }
                break;
            case 17:
                cat();
                get_char();
                if(C=='='){
                    state=0;
                    operators++;
                    write_out("relational_op",(void*)">=");
                }else if(C=='>'){
                    state=0;
                    operators++;
                    write_out("bitwise_op",(void*)">>");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("relational_op",(void*)">");
                }
                break;
            case 18:
                cat();
                get_char();
                if(C=='='){
                    state=0;
                    operators++;
                    write_out("relational_op",(void*)"==");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("assignment_op",(void*)"=");
                }
                break;
            case 19:
                cat();
                get_char();
                if(C=='='){
                    state=0;
                    operators++;
                    write_out("relational_op",(void*)"!=");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("logical_op",(void*)"!");
                }
                break;
            case 20:
                cat();
                get_char();
                if(C=='&'){
                    state=0;
                    operators++;
                    write_out("logical_op",(void*)"&&");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("bitwise_op",(void*)"&");
                }
                break;
            case 21:
                cat();
                get_char();
                if(C=='|'){
                    state=0;
                    operators++;
                    write_out("logical_op",(void*)"||");
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("bitwise_op",(void*)"|");
                }
                break;
            case 22:
                cat();
                get_char();
                if(C=='/'){
                    state=23;
                }else if(C=='*'){
                    state=24;
                }else{
                    retract();
                    state=0;
                    operators++;
                    write_out("arithmetic_op",(void*)"|");
                }
                break;
            case 23:
                cat();
                get_char();
                if(C=='\n'){
                    state=0;
                }else{
                    state=23;
                }
                break;
            case 24:
                cat();
                get_char();
                if(C=='*'){
                    state=25;
                }else{
                    state=24;
                }
                break;
            case 25:
                cat();
                get_char();
                if(C=='*'){
                    state=25;
                }else if(C=='/'){
                    state=0;
                }else{
                    state=24;
                }
                break;
        }
    }while(C!=EOF);
    printAnalyses();
    if(argc>=2){
        fclose(in);
        if(argc>=3) fclose(out);
	}
}