#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<stack>
#include<queue>
#include<unordered_map>
#include<algorithm>

// 定义了一些常量，如终结符和非终结符的数量
#define NUM_OF_TERMINATE 9
#define NUM_OF_NONTERMINATE 4
#define NUM_OF_PRODUCTION 8
#define NUM_OF_STATE 16
#define BUFFER_SIZE 100

using namespace std;

// 文件输入输出流
fstream fin;
fstream fout;

// 动作类型: 错误、移进、规约、接受
enum action_type {
    ERROR,SHIFT,REDUCE,ACC
};

// 符号类型：运算符、括号、数字等
enum symbol_type {
    PLUS,MINUS,MULTIPLY,DIVIDE,LEFT_PAREN,RIGHT_PAREN,NUMBER,END,
    E_PRIME,E,T,F 
};

// 符号名称
vector<string> symbol_name= {
	"+","-","*","/","(",")","NUM","END",
	"E_PRIME","E","T","F"
};

// 产生式结构
struct production {
    int left;
    vector<int> right;
}productions[NUM_OF_PRODUCTION+1]={
    {E_PRIME,{E}},
    {E,{E,PLUS,T}},
    {E,{E,MINUS,T}},
    {E,{T}},
    {T,{T,MULTIPLY,F}},
    {T,{T,DIVIDE,F}},
    {T,{F}},
    {F,{LEFT_PAREN,E,RIGHT_PAREN}},
    {F,{NUMBER}}
};

// LR动作表
pair<int,int> action_table[NUM_OF_STATE+1][NUM_OF_TERMINATE+1]={
    {{ERROR,0},{ERROR,0},{ERROR,0},{ERROR,0},{SHIFT,4},{ERROR,1},{SHIFT,5},{ERROR,0}},
    {{SHIFT,6},{SHIFT,7},{},{},{ERROR,2},{ERROR,1},{ERROR,2},{ACC,0}},
    {{REDUCE,3},{REDUCE,3},{SHIFT,8},{SHIFT,9},{ERROR,2},{REDUCE,3},{ERROR,2},{REDUCE,3}},
    {{REDUCE,6},{REDUCE,6},{REDUCE,6},{REDUCE,6},{REDUCE,6},{REDUCE,6},{REDUCE,6},{REDUCE,6}},
    {{ERROR,0},{ERROR,0},{ERROR,0},{ERROR,0},{SHIFT,4},{ERROR,1},{SHIFT,5},{ERROR,0}},
    {{REDUCE,8},{REDUCE,8},{REDUCE,8},{REDUCE,8},{REDUCE,8},{REDUCE,8},{REDUCE,8},{REDUCE,8}},
    {{ERROR,0},{ERROR,0},{ERROR,0},{ERROR,0},{SHIFT,4},{ERROR,1},{SHIFT,5},{ERROR,0}},
    {{ERROR,0},{ERROR,0},{ERROR,0},{ERROR,0},{SHIFT,4},{ERROR,1},{SHIFT,5},{ERROR,0}},
    {{ERROR,0},{ERROR,0},{ERROR,0},{ERROR,0},{SHIFT,4},{ERROR,1},{SHIFT,5},{ERROR,0}},
    {{ERROR,0},{ERROR,0},{ERROR,0},{ERROR,0},{SHIFT,4},{ERROR,1},{SHIFT,5},{ERROR,0}},
    {{SHIFT,6},{SHIFT,7},{ERROR,3},{ERROR,3},{ERROR,3},{SHIFT,15},{ERROR,3},{ERROR,3}},
    {{REDUCE,1},{REDUCE,1},{SHIFT,8},{SHIFT,9},{ERROR,2},{REDUCE,1},{ERROR,2},{REDUCE,1}},
    {{REDUCE,2},{REDUCE,2},{SHIFT,8},{SHIFT,9},{ERROR,2},{REDUCE,2},{ERROR,2},{REDUCE,2}},
    {{REDUCE,4},{REDUCE,4},{REDUCE,4},{REDUCE,4},{REDUCE,4},{REDUCE,4},{REDUCE,4},{REDUCE,4}},
    {{REDUCE,5},{REDUCE,5},{REDUCE,5},{REDUCE,5},{REDUCE,5},{REDUCE,5},{REDUCE,5},{REDUCE,5}},
    {{REDUCE,7},{REDUCE,7},{REDUCE,7},{REDUCE,7},{REDUCE,7},{REDUCE,7},{REDUCE,7},{REDUCE,7}}
};

// 用于散列std::pair的自定义hash函数
struct pair_hash {
    template <class T1, class T2>
    size_t operator () (const pair<T1, T2>& p) const {
        auto h1 = hash<T1>{}(p.first);
        auto h2 = hash<T2>{}(p.second);
        return h1 ^ (h2 << 4);
    }
};

// LR转换表
unordered_map <pair<int,int>,int,pair_hash>goto_table{
    {{0,E},1},{{0,T},2},{{0,F},3},
    {{4,E},10},{{4,T},2},{{4,F},3},
    {{6,T},11},{{6,F},3},
    {{7,T},12},{{7,F},3},
    {{8,F},13},
    {{9,F},14}
};

// 状态堆栈、符号堆栈和输入缓冲
stack<int> state_stk,symbol_stk;
queue<int> buffer;
int step;

// 打印当前的栈、输入
void print_info() {
    fout<<"--------Times of operations:"<<step++<<"-------\n";
	// ... 打印逻辑 ...
	fout<<"[state]:\n";
	stack<int> state_stk_copy=state_stk;
    while(!state_stk_copy.empty()){
        fout<<state_stk_copy.top()<<" ";
        state_stk_copy.pop();
    }
    fout<<"END\n[symbol]:\n";
    stack<int> symbol_stk_copy=symbol_stk;
    while(!symbol_stk_copy.empty()){
        fout<<symbol_name[symbol_stk_copy.top()]<<" ";
        symbol_stk_copy.pop();
    }
	fout<<"END\n[input]:\n";
	queue<int> buffer_copy=buffer;
    while(!buffer_copy.empty()){
        fout<<symbol_name[buffer_copy.front()]<<" ";
        buffer_copy.pop();
    }
    fout<<"\n";
}

// 打印动作信息
void print_action(int type,int value){
    fout<<"[action]"<<endl;
    if(type==SHIFT){
        fout<<"SHIFT "<<value<<endl;
    }else if(type==REDUCE){
        fout<<"REDUCE by ";
        fout<<symbol_name[productions[value].left]<<"->";
        for(auto item:productions[value].right){
            fout<<symbol_name[item]<<" ";
        }
        fout<<endl;
    }else{
        switch(value){
            case 0:
                fout<<"ERROR:缺少运算对象\nSOLUTION:把一个假象的NUM压入栈\n"<<endl;
                break;
            case 1:
                fout<<"ERROR:括号不匹配\nSOLUTION:跳过当前右括号\n";
                break;
            case 2:
                fout<<"ERROR:缺少运算符号\nSOLUTION:把一个假象的+压入栈\n"<<endl;
                break;
            case 3:
                fout<<"ERROR:缺少右括号\nSOLUTION:把一个假象的右括号加入栈\n"<<endl;
        }
    }
}

// 错误处理函数
void error(int error_type){
    switch(error_type){
        case 0:
            state_stk.push(5);
            symbol_stk.push(NUMBER);
            break;
        case 1:
            buffer.pop();
            break;
        case 2:
            state_stk.push(6);
            symbol_stk.push(PLUS);
            break;
        case 3:
            state_stk.push(15);
            symbol_stk.push(RIGHT_PAREN);
    }
}

int main(int argc,char **argv){
    // 处理输入输出文件
    if(argc>=3){
        fin.open(argv[1],ios::in);
        fout.open(argv[2],ios::out);
    }else{
        cout<<"参数数量不足"<<endl;
        exit(1);
    }
    // 初始化分析器的状态
    state_stk.push(0);
    string str;
    while(fin>>str){
        transform(str.begin(),str.end(),str.begin(),::toupper);
        buffer.push(find(symbol_name.begin(),symbol_name.end(),str)-symbol_name.begin());
    }
    buffer.push(END);
    // 主循环，处理输入
    do{
        int X=state_stk.top();
        int a=buffer.front();
        switch(action_table[X][a].first){
            case SHIFT:
                state_stk.push(action_table[X][a].second);
                symbol_stk.push(a);
                buffer.pop();
                break;
            case REDUCE:
                for(int i=0;i<productions[action_table[X][a].second].right.size();i++){
                    state_stk.pop();
                    symbol_stk.pop();
                }
                state_stk.push(goto_table[{state_stk.top(),productions[action_table[X][a].second].left}]);
                symbol_stk.push(productions[action_table[X][a].second].left);
                break;
            case ACC:
                fout<<"ACC"<<endl;
                cout<<"分析完毕"<<endl;
                return 0;
                break;
            default:
                error(action_table[X][a].second);
        }
        print_info();
        print_action(action_table[X][a].first,action_table[X][a].second);
    }while(1);
}