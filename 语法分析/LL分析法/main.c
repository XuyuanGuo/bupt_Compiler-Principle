#include<stdio.h>
#include<string.h>

// 定义一些常量，如产生式数量、终结符数量、非终结符数量等。
#define NUM_OF_PRODUCTION 10
#define NUM_OF_TERMINATE 9
#define NUM_OF_NONTERMINATE 5
#define MAX_TOKEN_SIZE 10
#define STACK_SIZE 100
#define BUFFER_SIZE 100

FILE *in,*out;
int step;                      // 全局计数器，记录当前操作步数
int rsp,stack[STACK_SIZE];     // rsp是栈顶指针，stack是解析栈
int ip,buffer[BUFFER_SIZE];    // ip是输入缓冲区的指针，buffer存储输入的令牌序列

// 定义所有的符号，包括终结符和非终结符
enum symbol_type {
    ERROR,SYNCH,
    EPLISON,END,PLUS,MINUS,MULTIPLY,DIVIDE,LEFT_PAREN,RIGHT_PAREN,NUMBER,
    E,E_PRIME,T,T_PRIME,F
};

// 符号的名称数组，用于输出
char *symbol_name[2+NUM_OF_TERMINATE+NUM_OF_NONTERMINATE]= {
	"ERROR","SYNCH","EPLISON","END","+","-","*","/","(",")","NUM",
	"E","E_PRIME","T","T_PRIME","F"
};

// FOLLOW集
struct follow {
	int non_terminate;
	int follow[NUM_OF_TERMINATE+1];
} follows[NUM_OF_NONTERMINATE+1]= {
	{E,{RIGHT_PAREN,END,-1}},
	{E_PRIME,{RIGHT_PAREN,END,-1}},
	{T,{PLUS,MINUS,RIGHT_PAREN,END,-1}},
	{T_PRIME,{PLUS,MINUS,RIGHT_PAREN,END,-1}},
	{F,{MULTIPLY,DIVIDE,PLUS,MINUS,RIGHT_PAREN,END,-1}},
	{-1}
};

// 定义产生式结构
struct production {
	int left;                           // 左边的非终结符
	int right[MAX_TOKEN_SIZE];              // 右边的符号序列
	int first[NUM_OF_TERMINATE+1];      // FIRST集
} productions[NUM_OF_PRODUCTION+1]= {  // 产生式数组
	// ... 各种产生式 ...
	{E,{T,E_PRIME,-1},{LEFT_PAREN,NUMBER,-1}},
	{E_PRIME,{PLUS,T,E_PRIME,-1},{PLUS,-1}},
	{E_PRIME,{MINUS,T,E_PRIME,-1},{MINUS,-1}},
	{E_PRIME,{EPLISON,-1},{EPLISON,-1}},
	{T,{F,T_PRIME,-1},{LEFT_PAREN,NUMBER,-1}},
	{T_PRIME,{MULTIPLY,F,T_PRIME,-1},{MULTIPLY,-1}},
	{T_PRIME,{DIVIDE,F,T_PRIME,-1},{DIVIDE,-1}},
	{T_PRIME,{EPLISON,-1},{EPLISON,-1}},
	{F,{LEFT_PAREN,E,RIGHT_PAREN,-1},{LEFT_PAREN,-1}},
	{F,{NUMBER,-1},{NUMBER,-1}},
	{-1}   // 用于标记结束
},
analyse_table[2+NUM_OF_TERMINATE+NUM_OF_NONTERMINATE][2+NUM_OF_TERMINATE+NUM_OF_NONTERMINATE];   // 预测分析表

// 判断一个符号是否是终结符
int is_termainate(int symbol) {
	return symbol>=2 && symbol<2+NUM_OF_TERMINATE;
}

// 通过名称得到符号类型
int get_type(char* name) {
	for(int i=0; symbol_name[i][0]; i++) {
		if(strcmp(name,symbol_name[i])==0)
			return i;
	}
	return -1;
}

// 初始化输入缓冲区，将输入文件转换为令牌序列
void buf_init() {
	int cnt=0;
	char ch,str[20]="";
	// ... 解析输入文件 ...
	while((ch=fgetc(in))!=EOF) {
		if(ch>='a'&&ch<='z') {
			ch=ch-'a'+'A';
		}
		if(ch==' '||ch=='\n') {
			buffer[ip++]=get_type(str);
			str[0]=0;
		} else {
			char *p=str;
			while(*p) p++;
			*p++=ch;
			*p=0;
		}
	}
	buffer[ip++]=END;  // 在输入末尾添加END令牌
	ip=0;
}

// 初始化解析栈
void stack_init() {
	rsp=0;
	stack[rsp++]=END;
	stack[rsp++]=E;
}

// 初始化预测分析表
void analyse_table_init() {
	// ... 根据产生式的FIRST集和FOLLOW集填充预测分析表 ...
	for(int i=0; ~productions[i].left; i++) {
		for(int j=0; ~productions[i].first[j]; j++) {
			analyse_table[productions[i].left][productions[i].first[j]]=productions[i];
			if(productions[i].first[j]==EPLISON) {
				int k=0;
				while(follows[k].non_terminate!=productions[i].left) {
					k++;
				};
				for(int l=0; ~follows[k].follow[l]; l++) {
					analyse_table[productions[i].left][follows[k].follow[l]]=productions[i];
				}
			}
		}
	}
	//在预测分析表空白处填synch和error
	for(int i=0; ~follows[i].non_terminate; i++) {
		for(int j=0; ~follows[i].follow[j]; j++) {
			if(!analyse_table[follows[i].non_terminate][follows[i].follow[j]].left) {
				analyse_table[follows[i].non_terminate][follows[i].follow[j]].left=SYNCH;
			}
		}
	}
}

// 错误处理函数
void error(int type) {
	// 这里可以进行错误处理
	if(type==0) {       // 发生错误时，栈顶为终结符
		fprintf(out,"ERROR: 分析栈栈顶为终结符%s，与当前输入符号%s不匹配\n",symbol_name[stack[rsp-1]],symbol_name[buffer[ip]]);
		fprintf(out,"SOLUTION: 弹出栈顶符号%s\n",symbol_name[stack[rsp-1]]);
		rsp--;          // 应急式错误处理方法，紧急弹出栈顶终结符
	} else {             // 发生错误时，栈顶为非终结符
		fprintf(out,"ERROR: 分析栈栈顶为%s，当前输入符号为%s，分析表为空\n",symbol_name[stack[rsp-1]],symbol_name[buffer[ip]]);
		//跳过若干个符号，直到可以继续分析为止
		while(analyse_table[stack[rsp-1]][buffer[ip]].left==ERROR) {
			ip++;
			fprintf(out,"SOLUTION: 右移输入缓冲区指针，当前输入缓冲区为:");
			for(int i=ip; buffer[i]!=0&&buffer[i]!=END; i++) {
				fprintf(out,"%s ",symbol_name[buffer[i]]);
			}
			fprintf(out,"END \n");
		}
		//若为synch，弹出栈顶符号
		if(analyse_table[stack[rsp-1]][buffer[ip]].left==SYNCH) {
			fprintf(out,"SOLUTION: 弹出栈顶符号%s\n",symbol_name[stack[rsp-1]]);
			rsp--;
		}
	}
	fprintf(out,"\n");
}

// 打印当前的栈、输入
void print_info() {
	// ... 打印逻辑 ...
	fprintf(out,"--------Times of operations:%d-------\n",step++);
	fprintf(out,"[stack]:\n");
	for(int i=0; i<rsp; i++) {
		if(i==rsp-1) {
			fprintf(out,"%s\n",symbol_name[stack[i]]);
		} else {
			fprintf(out,"%s ",symbol_name[stack[i]]);
		}
	}
	fprintf(out,"[input]:\n");
	for(int i=ip; buffer[i]!=0&&buffer[i]!=END; i++) {
		fprintf(out,"%s ",symbol_name[buffer[i]]);
	}
	fprintf(out,"END \n");
}

// 打印当前所使用的产生式
void print_production(struct production pd) {
	fprintf(out,"[output]:\n");
	fprintf(out,"%s -> ",symbol_name[pd.left]);
	for(int i=0; ~pd.right[i]; i++) {
		fprintf(out,"%s ",symbol_name[pd.right[i]]);
	}
	fprintf(out,"\n\n");
}

int main(int argc,char **argv) {
	// 打开输入和输出文件
	if(argc>=2) {
		in=fopen(argv[1],"r");
		if(argc>=3) {
			out=fopen(argv[2],"w");
		}
	}

	// 初始化输入缓冲区、解析栈和预测分析表
	buf_init();
	stack_init();
	analyse_table_init();
	//打印初始状态
	print_info();
	fprintf(out,"\n");

	// 主解析循环
	do {
		int X=stack[rsp-1];    // 取栈顶符号
		int a=buffer[ip];      // 取当前输入符号

		// 如果栈顶是终结符
		if(is_termainate(stack[rsp-1])) {
			// ... 处理终结符 ...
			if(X==a) {
				rsp--;
				ip++;
				print_info();
				fprintf(out,"\n");
			} else {
				error(0);
			};
		} else {
			// 如果栈顶是非终结符
			// ... 根据预测分析表进行处理 ...
			if(analyse_table[X][a].left>=2+NUM_OF_TERMINATE) {
				rsp--;
				int end=0;
				while(~analyse_table[X][a].right[end]) {
					end++;
				}
				end--;
				for(; end>=0; end--) {
					if(analyse_table[X][a].right[end]!=EPLISON) {
						stack[rsp++]=analyse_table[X][a].right[end];
					}
				}
				print_info();
				print_production(analyse_table[X][a]);
			} else {
				error(1);
			}
		}
	} while(rsp);  // 当栈非空时继续
	return 0;
}
