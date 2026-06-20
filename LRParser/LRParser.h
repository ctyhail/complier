// LRParser.h
// LR(0)/SLR(1) 语法分析器
// 文法（经典表达式文法，用于演示 LR 分析方法）：
//   (0) S' -> S
//   (1) S  -> E
//   (2) E  -> E + T
//   (3) E  -> T
//   (4) T  -> T * F
//   (5) T  -> F
//   (6) F  -> ( E )
//   (7) F  -> id
#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>

// 文法符号类型
enum class SymbolKindLR { TERMINAL, NONTERMINAL, END };

struct GrammarSymbol {
    std::string name;
    SymbolKindLR kind;
    bool operator==(const GrammarSymbol& o) const { return name == o.name && kind == o.kind; }
    bool operator<(const GrammarSymbol& o) const {
        if (kind != o.kind) return kind < o.kind;
        return name < o.name;
    }
};

// 产生式
struct Production {
    GrammarSymbol lhs;                  // 左部
    std::vector<GrammarSymbol> rhs;      // 右部
    int id;                              // 编号
};

// LR(0) 项目
struct LR0Item {
    int prodId;     // 产生式编号
    int dotPos;     // 点的位置
    bool operator==(const LR0Item& o) const { return prodId == o.prodId && dotPos == o.dotPos; }
    bool operator<(const LR0Item& o) const {
        if (prodId != o.prodId) return prodId < o.prodId;
        return dotPos < o.dotPos;
    }
};

// LR(0) 项目集（DFA 状态）
struct LR0State {
    int id;
    std::set<LR0Item> items;
};

// 分析表动作类型
enum class ActionType { SHIFT, REDUCE, ACCEPT, ERROR };

struct ActionEntry {
    ActionType type;
    int target;  // SHIFT: 目标状态; REDUCE: 产生式编号; ACCEPT/ERROR: 无意义
};

class LRParser {
private:
    std::vector<Production> productions;              // 产生式表
    std::map<std::string, std::set<std::string>> firstSets;  // FIRST 集
    std::map<std::string, std::set<std::string>> followSets; // FOLLOW 集
    std::vector<LR0State> states;                     // DFA 状态集
    std::map<std::pair<int, std::string>, ActionEntry> actionTable; // ACTION 表
    std::map<std::pair<int, std::string>, int> gotoTable;          // GOTO 表
    std::map<std::string, int> nonterminalIndex;      // 非终结符名 -> 产生式起始索引

    // 辅助函数
    std::set<LR0Item> closure(const std::set<LR0Item>& items);
    std::set<LR0Item> gotoState(const std::set<LR0Item>& items, const GrammarSymbol& sym);
    int findState(const std::set<LR0Item>& items);
    void buildDFA();
    void computeFollowSets();
    void buildSLRTable();

public:
    LRParser();
    bool parse(const std::vector<std::string>& input);
    void printGrammar();
    void printFirstFollow();
    void printDFA();
    void printTable();
};
