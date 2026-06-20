#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <set>

// 定义状态节点
struct State {
    int id;
    bool isAccept;
    // 状态转移表：字符 -> 目标状态集合 (NFA用多值，DFA只会有一个值)
    // 使用 '\0' 代表 Epsilon (空转移)
    std::map<char, std::vector<int>> transitions; 
};

// 封装一个自动机 (NFA 或 DFA)
struct Automaton {
    int startState;
    int acceptState; // 对于 NFA 通常只有一个唯一的接收态
    std::vector<State> states; // 存储所有状态的数组
    
    int addState(bool accept = false) {
        int id = states.size();
        states.push_back({id, accept, {}});
        return id;
    }
};

class RegexEngine {
public:
    // 流水线主入口
    Automaton compileToDFA(const std::string& regex);
    // draw
    void generateDotFile(const Automaton& a, const std::string& filename);

private:
    // 第一步：预处理正规式，插入显式的连接符 '.' (例如 ab -> a.b)
    std::string insertExplicitConcat(const std::string& regex);
    
    // 第二步：中缀转后缀 (Shunting-yard 算法)
    std::string toPostfix(const std::string& regex);
    
    // 第三步：Thompson 构造法 (后缀表达式转 NFA)
    Automaton buildNFA(const std::string& postfix);
    
    // 第四步：子集构造法 (NFA 转 DFA)
    Automaton buildDFA(Automaton& nfa);
    
    // 第五步：Hopcroft 算法 (DFA 最小化)
    Automaton minimizeDFA(Automaton& dfa);

    // 辅助函数：计算 epsilon 闭包
    std::set<int> epsilonClosure(const std::set<int>& T, const Automaton& nfa);
    // 辅助函数：计算集合在输入字符 c 下的转移并求闭包
    std::set<int> move(const std::set<int>& T, char c, const Automaton& nfa);
    // 
    std::set<char> collectAlphabet(const Automaton& nfa);
    void printAutomaton(const Automaton& a, const std::string& name);
    
};