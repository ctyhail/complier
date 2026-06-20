#include "LRParser.h"
#include <queue>
#include <iomanip>

LRParser::LRParser() {
    // 构造文法
    // S' -> S
    productions.push_back({{"S'", SymbolKindLR::NONTERMINAL}, {{"S", SymbolKindLR::NONTERMINAL}}, 0});
    // S -> E
    productions.push_back({{"S", SymbolKindLR::NONTERMINAL}, {{"E", SymbolKindLR::NONTERMINAL}}, 1});
    // E -> E + T
    productions.push_back({{"E", SymbolKindLR::NONTERMINAL},
                          {{"E", SymbolKindLR::NONTERMINAL}, {"+", SymbolKindLR::TERMINAL}, {"T", SymbolKindLR::NONTERMINAL}}, 2});
    // E -> T
    productions.push_back({{"E", SymbolKindLR::NONTERMINAL}, {{"T", SymbolKindLR::NONTERMINAL}}, 3});
    // T -> T * F
    productions.push_back({{"T", SymbolKindLR::NONTERMINAL},
                          {{"T", SymbolKindLR::NONTERMINAL}, {"*", SymbolKindLR::TERMINAL}, {"F", SymbolKindLR::NONTERMINAL}}, 4});
    // T -> F
    productions.push_back({{"T", SymbolKindLR::NONTERMINAL}, {{"F", SymbolKindLR::NONTERMINAL}}, 5});
    // F -> ( E )
    productions.push_back({{"F", SymbolKindLR::NONTERMINAL},
                          {{"(", SymbolKindLR::TERMINAL}, {"E", SymbolKindLR::NONTERMINAL}, {")", SymbolKindLR::TERMINAL}}, 6});
    // F -> id
    productions.push_back({{"F", SymbolKindLR::NONTERMINAL}, {{"id", SymbolKindLR::TERMINAL}}, 7});

    computeFollowSets();
    buildDFA();
    buildSLRTable();
}

// 计算 CLOSURE
std::set<LR0Item> LRParser::closure(const std::set<LR0Item>& items) {
    std::set<LR0Item> result = items;
    bool changed = true;
    while (changed) {
        changed = false;
        std::set<LR0Item> toAdd;
        for (const auto& item : result) {
            const Production& prod = productions[item.prodId];
            if (item.dotPos >= (int)prod.rhs.size()) continue;
            GrammarSymbol B = prod.rhs[item.dotPos];
            if (B.kind != SymbolKindLR::NONTERMINAL) continue;

            // 对 B -> . gamma 的所有产生式加入项目集
            for (int i = 0; i < (int)productions.size(); ++i) {
                if (productions[i].lhs == B) {
                    LR0Item newItem{i, 0};
                    if (result.find(newItem) == result.end()) {
                        toAdd.insert(newItem);
                    }
                }
            }
        }
        if (!toAdd.empty()) {
            result.insert(toAdd.begin(), toAdd.end());
            changed = true;
        }
    }
    return result;
}

// 计算 GOTO(I, X)
std::set<LR0Item> LRParser::gotoState(const std::set<LR0Item>& items, const GrammarSymbol& sym) {
    std::set<LR0Item> moved;
    for (const auto& item : items) {
        const Production& prod = productions[item.prodId];
        if (item.dotPos >= (int)prod.rhs.size()) continue;
        if (prod.rhs[item.dotPos] == sym) {
            moved.insert({item.prodId, item.dotPos + 1});
        }
    }
    if (moved.empty()) return moved;
    return closure(moved);
}

// 查找状态是否已存在
int LRParser::findState(const std::set<LR0Item>& items) {
    for (const auto& s : states) {
        if (s.items == items) return s.id;
    }
    return -1;
}

// 构造识别活前缀的 DFA
void LRParser::buildDFA() {
    // 初始状态：{ S' -> . S } 的闭包
    std::set<LR0Item> initItems = {{0, 0}};
    std::set<LR0Item> initClosure = closure(initItems);
    LR0State initState{0, initClosure};
    states.push_back(initState);

    // 收集所有文法符号
    std::vector<GrammarSymbol> allSymbols;
    std::set<std::string> seen;
    for (const auto& p : productions) {
        if (seen.find(p.lhs.name) == seen.end()) {
            allSymbols.push_back(p.lhs);
            seen.insert(p.lhs.name);
        }
        for (const auto& s : p.rhs) {
            std::string key = (s.kind == SymbolKindLR::TERMINAL ? "T_" : "N_") + s.name;
            if (seen.find(key) == seen.end()) {
                allSymbols.push_back(s);
                seen.insert(key);
            }
        }
    }

    // BFS 构造 DFA
    std::queue<int> workQueue;
    workQueue.push(0);
    while (!workQueue.empty()) {
        int stateId = workQueue.front();
        workQueue.pop();

        // 注意：不能用引用，因为 states.push_back 会使引用失效
        std::set<LR0Item> curItems = states[stateId].items;

        for (const auto& sym : allSymbols) {
            std::set<LR0Item> next = gotoState(curItems, sym);
            if (next.empty()) continue;

            int existingId = findState(next);
            if (existingId == -1) {
                int newId = states.size();
                states.push_back({newId, next});
                workQueue.push(newId);

                // 记录转移
                if (sym.kind == SymbolKindLR::TERMINAL) {
                    actionTable[{stateId, sym.name}] = {ActionType::SHIFT, newId};
                } else {
                    gotoTable[{stateId, sym.name}] = newId;
                }
            } else {
                if (sym.kind == SymbolKindLR::TERMINAL) {
                    actionTable[{stateId, sym.name}] = {ActionType::SHIFT, existingId};
                } else {
                    gotoTable[{stateId, sym.name}] = existingId;
                }
            }
        }
    }
}

// 计算 FOLLOW 集
void LRParser::computeFollowSets() {
    // FOLLOW(S') = {$}
    followSets["S'"].insert("$");
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& prod : productions) {
            const std::string& A = prod.lhs.name;
            for (int i = 0; i < (int)prod.rhs.size(); ++i) {
                const GrammarSymbol& B = prod.rhs[i];
                if (B.kind != SymbolKindLR::NONTERMINAL) continue;

                std::set<std::string> addSet;
                // beta = prod.rhs[i+1..end]
                bool betaDerivesEpsilon = true;
                for (int j = i + 1; j < (int)prod.rhs.size(); ++j) {
                    const GrammarSymbol& s = prod.rhs[j];
                    if (s.kind == SymbolKindLR::TERMINAL) {
                        addSet.insert(s.name);
                        betaDerivesEpsilon = false;
                        break;
                    }
                }
                // 若 beta 为空或可推导空，加入 FOLLOW(A)
                if (betaDerivesEpsilon) {
                    for (const auto& f : followSets[A]) {
                        if (followSets[B.name].find(f) == followSets[B.name].end()) {
                            followSets[B.name].insert(f);
                            changed = true;
                        }
                    }
                }
                for (const auto& f : addSet) {
                    if (followSets[B.name].find(f) == followSets[B.name].end()) {
                        followSets[B.name].insert(f);
                        changed = true;
                    }
                }
            }
        }
    }
}

// 构造 SLR(1) 分析表
void LRParser::buildSLRTable() {
    for (const auto& state : states) {
        for (const auto& item : state.items) {
            const Production& prod = productions[item.prodId];

            // 接受项目：S' -> S .
            if (item.prodId == 0 && item.dotPos == 1) {
                actionTable[{state.id, "$"}] = {ActionType::ACCEPT, 0};
                continue;
            }

            // 归约项目：A -> alpha .
            if (item.dotPos == (int)prod.rhs.size()) {
                for (const auto& a : followSets[prod.lhs.name]) {
                    // SLR(1)：用 A -> alpha 归约，对 a in FOLLOW(A)
                    if (actionTable.find({state.id, a}) == actionTable.end()) {
                        actionTable[{state.id, a}] = {ActionType::REDUCE, item.prodId};
                    }
                }
            }
            // 移进项目：A -> alpha . a beta，已在 buildDFA 中处理 SHIFT
        }
    }
}

// 打印文法
void LRParser::printGrammar() {
    std::cout << "=== Grammar ===" << std::endl;
    for (const auto& p : productions) {
        std::cout << "(" << p.id << ") " << p.lhs.name << " ->";
        if (p.rhs.empty()) std::cout << " ε";
        for (const auto& s : p.rhs) std::cout << " " << s.name;
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// 打印 FIRST/FOLLOW 集
void LRParser::printFirstFollow() {
    std::cout << "=== FOLLOW Sets ===" << std::endl;
    std::vector<std::string> nonterms = {"S'", "S", "E", "T", "F"};
    for (const auto& nt : nonterms) {
        std::cout << "FOLLOW(" << nt << ") = { ";
        for (const auto& s : followSets[nt]) std::cout << s << " ";
        std::cout << "}" << std::endl;
    }
    std::cout << std::endl;
}

// 打印 DFA
void LRParser::printDFA() {
    std::cout << "=== DFA (LR(0) Item Sets) ===" << std::endl;
    for (const auto& state : states) {
        std::cout << "State I" << state.id << ":" << std::endl;
        for (const auto& item : state.items) {
            const Production& prod = productions[item.prodId];
            std::cout << "  " << prod.lhs.name << " ->";
            for (int i = 0; i < (int)prod.rhs.size(); ++i) {
                if (i == item.dotPos) std::cout << " .";
                std::cout << " " << prod.rhs[i].name;
            }
            if (item.dotPos == (int)prod.rhs.size()) std::cout << " .";
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "=== DFA Transitions ===" << std::endl;
    for (const auto& state : states) {
        for (const auto& kv : actionTable) {
            if (kv.first.first == state.id && kv.second.type == ActionType::SHIFT) {
                std::cout << "  I" << state.id << " --" << kv.first.second << "--> I" << kv.second.target << std::endl;
            }
        }
        for (const auto& kv : gotoTable) {
            if (kv.first.first == state.id) {
                std::cout << "  I" << state.id << " --" << kv.first.second << "--> I" << kv.second << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

// 打印分析表
void LRParser::printTable() {
    std::cout << "=== SLR(1) Action/Goto Table ===" << std::endl;
    std::vector<std::string> terms = {"id", "+", "*", "(", ")", "$"};
    std::vector<std::string> nonterms = {"S", "E", "T", "F"};

    std::cout << "State | ACTION";
    for (const auto& t : terms) std::cout << "  " << std::setw(8) << t;
    std::cout << " | GOTO";
    for (const auto& nt : nonterms) std::cout << "  " << std::setw(8) << nt;
    std::cout << std::endl;

    for (const auto& state : states) {
        std::cout << std::setw(5) << state.id << " |";
        for (const auto& t : terms) {
            auto it = actionTable.find({state.id, t});
            if (it != actionTable.end()) {
                std::string s;
                if (it->second.type == ActionType::SHIFT) s = "S" + std::to_string(it->second.target);
                else if (it->second.type == ActionType::REDUCE) s = "R" + std::to_string(it->second.target);
                else if (it->second.type == ActionType::ACCEPT) s = "ACC";
                std::cout << "  " << std::setw(8) << s;
            } else {
                std::cout << "  " << std::setw(8) << "";
            }
        }
        std::cout << " |";
        for (const auto& nt : nonterms) {
            auto it = gotoTable.find({state.id, nt});
            if (it != gotoTable.end()) {
                std::cout << "  " << std::setw(8) << it->second;
            } else {
                std::cout << "  " << std::setw(8) << "";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// 执行 LR 分析
bool LRParser::parse(const std::vector<std::string>& input) {
    std::cout << "=== LR Parsing Process ===" << std::endl;
    std::cout << "Input: ";
    for (const auto& s : input) std::cout << s << " ";
    std::cout << std::endl << std::endl;

    std::vector<int> stateStack;
    std::vector<std::string> symbolStack;
    stateStack.push_back(0);
    symbolStack.push_back("$");

    std::vector<std::string> inputBuf = input;
    inputBuf.push_back("$");
    int inputPtr = 0;

    std::cout << std::left;
    std::cout << std::setw(30) << "Stack" << std::setw(20) << "Input" << "Action" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    int step = 0;
    while (true) {
        int state = stateStack.back();
        std::string a = inputBuf[inputPtr];

        // 打印状态
        std::string stackStr;
        for (size_t i = 0; i < stateStack.size(); ++i) {
            stackStr += std::to_string(stateStack[i]);
            if (i < symbolStack.size()) stackStr += symbolStack[i];
        }
        std::string inputStr;
        for (size_t i = inputPtr; i < inputBuf.size(); ++i) inputStr += inputBuf[i] + " ";

        auto it = actionTable.find({state, a});
        if (it == actionTable.end()) {
            std::cout << std::setw(30) << stackStr << std::setw(20) << inputStr << "ERROR" << std::endl;
            std::cout << "\nSyntax error at input symbol: " << a << std::endl;
            return false;
        }

        ActionEntry action = it->second;
        std::string actionStr;
        if (action.type == ActionType::SHIFT) {
            actionStr = "Shift to " + std::to_string(action.target);
        } else if (action.type == ActionType::REDUCE) {
            const Production& p = productions[action.target];
            actionStr = "Reduce by " + p.lhs.name + " ->";
            for (const auto& s : p.rhs) actionStr += " " + s.name;
        } else if (action.type == ActionType::ACCEPT) {
            actionStr = "ACCEPT";
        }

        std::cout << std::setw(30) << stackStr << std::setw(20) << inputStr << actionStr << std::endl;

        if (action.type == ActionType::ACCEPT) {
            std::cout << "\nSyntax correct" << std::endl;
            return true;
        } else if (action.type == ActionType::SHIFT) {
            stateStack.push_back(action.target);
            symbolStack.push_back(a);
            inputPtr++;
        } else if (action.type == ActionType::REDUCE) {
            const Production& p = productions[action.target];
            int popCount = p.rhs.size();
            for (int i = 0; i < popCount; ++i) {
                stateStack.pop_back();
                symbolStack.pop_back();
            }
            symbolStack.push_back(p.lhs.name);
            int topState = stateStack.back();
            auto git = gotoTable.find({topState, p.lhs.name});
            if (git == gotoTable.end()) {
                std::cout << "\nGoto error" << std::endl;
                return false;
            }
            stateStack.push_back(git->second);
        }
        step++;
        if (step > 1000) {
            std::cout << "\nToo many steps, possibly infinite loop" << std::endl;
            return false;
        }
    }
}
