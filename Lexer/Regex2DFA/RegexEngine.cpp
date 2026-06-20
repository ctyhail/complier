#include "RegexEngine.h"
#include <set>
#include <stack>
#include <fstream>

// 计算状态集合 T 的 epsilon 闭包
std::set<int> RegexEngine::epsilonClosure(const std::set<int>& T, const Automaton& nfa) {
    std::stack<int> stack;
    std::set<int> closure = T; // 初始状态集合本身必定在闭包中

    // 将初始集合中的所有状态压入栈中，准备进行深度优先搜索
    for (int stateId : T) {
        stack.push(stateId);
    }

    // DFS 主循环
    while (!stack.empty()) {
        int currentState = stack.top();
        stack.pop();

        // 查找当前状态是否存在 epsilon 转移 (我们约定用 '\0' 表示 epsilon)
        auto it = nfa.states[currentState].transitions.find('\0');
        
        if (it != nfa.states[currentState].transitions.end()) {
            // 遍历所有可以通过 epsilon 免费到达的目标状态
            for (int nextState : it->second) {
                // 如果该目标状态还没有被加入闭包，则加入并压栈继续往下搜
                if (closure.find(nextState) == closure.end()) {
                    closure.insert(nextState);
                    stack.push(nextState);
                }
            }
        }
    }

    return closure;
}

// 计算状态集合 T 在输入字符 c 下的转移结果
std::set<int> RegexEngine::move(const std::set<int>& T, char c, const Automaton& nfa) {
    std::set<int> result;

    // 遍历当前状态集合中的每一个状态
    for (int stateId : T) {
        // 查找该状态在吃入字符 c 后，能走到哪些状态
        auto it = nfa.states[stateId].transitions.find(c);
        
        if (it != nfa.states[stateId].transitions.end()) {
            // 将所有目标状态加入结果集
            for (int nextState : it->second) {
                result.insert(nextState);
            }
        }
    }

    // 注意：这里返回的仅仅是 move 一步的结果，不包含 epsilon 闭包！
    return result;
}

// 1
std::string RegexEngine::insertExplicitConcat(const std::string& regex) {
    std::string res = "";
    for (size_t i = 0; i < regex.length(); i++) {
        char c1 = regex[i];
        res += c1;
        
        if (i + 1 < regex.length()) {
            char c2 = regex[i + 1];
            
            // 规则：如果前一个字符是 字母/数字、')' 或 '*' 
            // 且 后一个字符是 字母/数字 或 '('
            // 则它们之间存在隐式的连接符
            bool isC1Concat = (isalnum(c1) || c1 == ')' || c1 == '*');
            bool isC2Concat = (isalnum(c2) || c2 == '(');
            
            if (isC1Concat && isC2Concat) {
                res += '.';
            }
        }
    }
    return res;
}

// 2
std::string RegexEngine::toPostfix(const std::string& regex) {
    std::string postfix;
    std::stack<char> opStack;
    
    // 定义操作符优先级
    auto precedence = [](char c) {
        if (c == '*') return 3;
        if (c == '.') return 2;
        if (c == '|') return 1;
        return 0; // 应对栈底的 '('
    };

    for (char c : regex) {
        if (isalnum(c)) { // 遇到操作数，直接输出
            postfix += c;
        } else if (c == '(') { // 左括号入栈
            opStack.push(c);
        } else if (c == ')') { // 右括号：不断弹栈直到遇到左括号
            while (!opStack.empty() && opStack.top() != '(') {
                postfix += opStack.top();
                opStack.pop();
            }
            if (!opStack.empty()) opStack.pop(); // 弹出 '('
        } else { // 遇到操作符
            while (!opStack.empty() && precedence(opStack.top()) >= precedence(c)) {
                postfix += opStack.top();
                opStack.pop();
            }
            opStack.push(c);
        }
    }
    
    // 将栈中剩余的操作符全部弹出
    while (!opStack.empty()) {
        postfix += opStack.top();
        opStack.pop();
    }
    
    return postfix;
}

// 3
Automaton RegexEngine::buildNFA(const std::string& postfix) {
    std::stack<Automaton> stack;

    // 核心辅助 Lambda：将 src 自动机的所有状态合并到 dest 中，并自动更新状态 ID 和转移目标
    auto appendNFA = [](Automaton& dest, const Automaton& src) -> int {
        int offset = dest.states.size();
        for (const auto& state : src.states) {
            State newState = { state.id + offset, state.isAccept, {} };
            for (const auto& pair : state.transitions) {
                for (int target : pair.second) {
                    // 目标状态的 ID 也必须加上偏移量
                    newState.transitions[pair.first].push_back(target + offset); 
                }
            }
            dest.states.push_back(newState);
        }
        return offset; // 返回偏移量，方便后续手动连线
    };

    for (char c : postfix) {
        if (c == '.') { // 连接操作
            Automaton right = stack.top(); stack.pop();
            Automaton left = stack.top(); stack.pop();
            
            int rightOffset = appendNFA(left, right);
            
            // 将 left 原本的终态 通过 epsilon('\0') 连接到 right 的初态
            left.states[left.acceptState].transitions['\0'].push_back(right.startState + rightOffset);
            left.states[left.acceptState].isAccept = false;
            left.acceptState = right.acceptState + rightOffset;
            
            stack.push(left);
        } 
        else if (c == '|') { // 选择操作
            Automaton right = stack.top(); stack.pop();
            Automaton left = stack.top(); stack.pop();
            
            Automaton merged;
            merged.startState = merged.addState(false); // 新建唯一初态
            
            int leftOffset = appendNFA(merged, left);
            int rightOffset = appendNFA(merged, right);
            
            merged.acceptState = merged.addState(true); // 新建唯一终态
            
            // 新初态 分支到 原 left 和 原 right 初态
            merged.states[merged.startState].transitions['\0'].push_back(left.startState + leftOffset);
            merged.states[merged.startState].transitions['\0'].push_back(right.startState + rightOffset);
            
            // 原 left 和 原 right 终态 汇聚到 新终态
            merged.states[left.acceptState + leftOffset].transitions['\0'].push_back(merged.acceptState);
            merged.states[left.acceptState + leftOffset].isAccept = false;
            
            merged.states[right.acceptState + rightOffset].transitions['\0'].push_back(merged.acceptState);
            merged.states[right.acceptState + rightOffset].isAccept = false;
            
            stack.push(merged);
        } 
        else if (c == '*') { // 闭包操作
            Automaton nfa = stack.top(); stack.pop();
            
            Automaton merged;
            merged.startState = merged.addState(false);
            
            int offset = appendNFA(merged, nfa);
            
            merged.acceptState = merged.addState(true);
            
            // 构造 4 条 epsilon 边
            merged.states[merged.startState].transitions['\0'].push_back(nfa.startState + offset); // 1. 起点入子图
            merged.states[merged.startState].transitions['\0'].push_back(merged.acceptState);     // 2. 绕过子图 (0次)
            
            merged.states[nfa.acceptState + offset].transitions['\0'].push_back(nfa.startState + offset); // 3. 循环回溯
            merged.states[nfa.acceptState + offset].transitions['\0'].push_back(merged.acceptState);      // 4. 终点出子图
            merged.states[nfa.acceptState + offset].isAccept = false;
            
            stack.push(merged);
        } 
        else { 
            // 基础操作：单字符构建最简单的 NFA (状态 A -> 状态 B)
            Automaton basic;
            basic.startState = basic.addState(false);
            basic.acceptState = basic.addState(true);
            basic.states[basic.startState].transitions[c].push_back(basic.acceptState);
            stack.push(basic);
        }
    }
    
    return stack.empty() ? Automaton() : stack.top();
}


// 4
std::set<char> RegexEngine::collectAlphabet(const Automaton& nfa) {
    std::set<char> alphabet;
    for (const auto& state : nfa.states) {
        for (const auto& pair : state.transitions) {
            if (pair.first != '\0') {
                alphabet.insert(pair.first);
            }
        }
    }
    return alphabet;
}

Automaton RegexEngine::buildDFA(Automaton& nfa) {
    Automaton dfa;
    // 映射表：将 NFA 的状态集合（Set）映射为 DFA 的一个唯一状态 ID
    std::map<std::set<int>, int> set2dfaState;
    std::vector<std::set<int>> dfaStatesList; // 记录每个 DFA 状态对应的 NFA 集合

    // 1. 获取 DFA 起始状态：{NFA起始态} 的 epsilon 闭包
    std::set<int> startSet = {nfa.startState};
    std::set<int> startClosure = epsilonClosure(startSet, nfa);
    
    dfa.startState = dfa.addState(false);
    set2dfaState[startClosure] = dfa.startState;
    dfaStatesList.push_back(startClosure);

    // 2. 调度场 (队列) 处理 DFA 状态
    std::stack<int> workStack;
    workStack.push(dfa.startState);

    while (!workStack.empty()) {
        int currentDfaId = workStack.top();
        workStack.pop();
        std::set<int> currentSet = dfaStatesList[currentDfaId];

        // 检查是否为接受态 (如果集合中包含 NFA 的终态)
        for (int nfaId : currentSet) {
            if (nfa.states[nfaId].isAccept) {
                dfa.states[currentDfaId].isAccept = true;
                break;
            }
        }

        // 3. 对所有可能的输入字符进行转移
        // 建议收集 NFA 中出现的所有字母/数字
        std::set<char> alphabet = collectAlphabet(nfa); // 示例：实际应从NFA扫描获取
        for (char c : alphabet) {
            std::set<int> nextSet = epsilonClosure(move(currentSet, c, nfa), nfa);
            
            if (nextSet.empty()) continue;

            // 如果该集合是新状态，则加入 DFA
            if (set2dfaState.find(nextSet) == set2dfaState.end()) {
                int newStateId = dfa.addState(false);
                set2dfaState[nextSet] = newStateId;
                dfaStatesList.push_back(nextSet);
                workStack.push(newStateId);
            }
            
            // 添加 DFA 转移边
            dfa.states[currentDfaId].transitions[c].push_back(set2dfaState[nextSet]);
        }
    }
    return dfa;
}

// 5
Automaton RegexEngine::minimizeDFA(Automaton& dfa) {
    std::set<char> alphabet = collectAlphabet(dfa);
    
    // 1. 初始划分：将状态分为 "终态集" 和 "非终态集"
    std::vector<std::set<int>> partitions;
    std::set<int> acceptGroup, nonAcceptGroup;
    
    for (const auto& state : dfa.states) {
        if (state.isAccept) acceptGroup.insert(state.id);
        else nonAcceptGroup.insert(state.id);
    }
    
    if (!nonAcceptGroup.empty()) partitions.push_back(nonAcceptGroup);
    if (!acceptGroup.empty()) partitions.push_back(acceptGroup);

    // 2. 迭代分裂 (Hopcroft 算法的核心逻辑)
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<std::set<int>> newPartitions;

        for (const auto& group : partitions) {
            // 如果集合里只有一个状态，绝不可能再分裂，直接保留
            if (group.size() <= 1) {
                newPartitions.push_back(group);
                continue;
            }

            // 使用一个特征签名(Behavior)来给组内的状态进行分类
            // map的键是特征签名，值是拥有该签名的状态集合
            std::map<std::vector<int>, std::set<int>> splitMap;
            
            for (int stateId : group) {
                std::vector<int> behavior; // 记录该状态遇到不同字符时，跳到了哪个旧集合中
                
                for (char c : alphabet) {
                    int targetPartition = -1; // -1 代表死胡同(没有转移边)
                    auto it = dfa.states[stateId].transitions.find(c);
                    
                    if (it != dfa.states[stateId].transitions.end() && !it->second.empty()) {
                        int destNode = it->second[0]; // DFA对于确定字符只有1个去向
                        // 查找目标节点属于哪一个集合
                        for (size_t p = 0; p < partitions.size(); ++p) {
                            if (partitions[p].count(destNode)) {
                                targetPartition = p;
                                break;
                            }
                        }
                    }
                    behavior.push_back(targetPartition);
                }
                splitMap[behavior].insert(stateId);
            }

            // 如果生成了大于1个分类，说明这个组产生了分裂！
            for (const auto& pair : splitMap) {
                newPartitions.push_back(pair.second);
            }
            if (splitMap.size() > 1) changed = true;
        }
        partitions = newPartitions; // 更新划分结果
    }

    // 3. 根据最终的划分结果，重构出一个全新的最小化 DFA
    Automaton minDfa;
    std::map<int, int> oldToNew; // 旧状态ID 到 新状态ID(即所属集合的索引) 的映射
    
    // 创建新状态
    for (size_t i = 0; i < partitions.size(); ++i) {
        bool isAccept = false;
        for (int oldId : partitions[i]) {
            oldToNew[oldId] = i;
            if (dfa.states[oldId].isAccept) isAccept = true;
        }
        int newId = minDfa.addState(isAccept);
        
        // 标记起始状态
        if (partitions[i].count(dfa.startState)) {
            minDfa.startState = newId;
        }
    }

    // 建立新状态之间的转移边
    for (size_t i = 0; i < partitions.size(); ++i) {
        // 从集合中随便挑一个代表即可，因为集合内所有状态的转移行为必定一致
        int repOldId = *partitions[i].begin(); 
        
        for (char c : alphabet) {
            auto it = dfa.states[repOldId].transitions.find(c);
            if (it != dfa.states[repOldId].transitions.end() && !it->second.empty()) {
                int oldTarget = it->second[0];
                int newTarget = oldToNew[oldTarget];
                minDfa.states[i].transitions[c].push_back(newTarget);
            }
        }
    }

    return minDfa;
}


// 在控制台打印自动机的详细状态
void RegexEngine::printAutomaton(const Automaton& a, const std::string& name) {
    std::cout << "\n========== " << name << " ==========" << std::endl;
    std::cout << "Start State: S" << a.startState << std::endl;
    
    for (const auto& state : a.states) {
        std::cout << "S" << state.id;
        if (state.isAccept) std::cout << " [ACCEPT]";
        std::cout << " transitions:" << std::endl;
        
        for (const auto& pair : state.transitions) {
            std::string edge = (pair.first == '\0') ? "ε (epsilon)" : std::string(1, pair.first);
            std::cout << "  --(" << edge << ")--> { ";
            for (int target : pair.second) {
                std::cout << "S" << target << " ";
            }
            std::cout << "}" << std::endl;
        }
        if (state.transitions.empty()) {
            std::cout << "  (Dead end)" << std::endl;
        }
    }
    std::cout << "====================================\n" << std::endl;
}

// 加分神器：生成 Graphviz DOT 文件
void RegexEngine::generateDotFile(const Automaton& a, const std::string& filename) {
    std::ofstream out(filename);
    out << "digraph Automaton {\n";
    out << "  rankdir=LR;\n"; // 从左到右布局
    out << "  node [shape = doublecircle]; ";
    
    // 标记所有接受态为双圈
    for (const auto& state : a.states) {
        if (state.isAccept) out << "\"S" << state.id << "\" ";
    }
    out << ";\n  node [shape = circle];\n";
    
    // 添加一个隐形的起始箭头
    out << "  secret_start [shape=point];\n";
    out << "  secret_start -> \"S" << a.startState << "\";\n";

    // 绘制所有边
    for (const auto& state : a.states) {
        for (const auto& pair : state.transitions) {
            std::string label = (pair.first == '\0') ? "ε" : std::string(1, pair.first);
            for (int target : pair.second) {
                out << "  \"S" << state.id << "\" -> \"S" << target 
                    << "\" [label=\"" << label << "\"];\n";
            }
        }
    }
    out << "}\n";
    std::cout << "[Visualizer] Saved graph to " << filename << std::endl;
}

Automaton RegexEngine::compileToDFA(const std::string& regex) {
    std::cout << "[Step 1] Normalizing Regex: " << regex << std::endl;
    std::string normalized = insertExplicitConcat(regex);
    
    std::cout << "[Step 2] Converting to Postfix: " << normalized << std::endl;
    std::string postfix = toPostfix(normalized);
    
    std::cout << "[Step 3] Building NFA..." << std::endl;
    Automaton nfa = buildNFA(postfix);
    //printAutomaton(nfa, "NFA");

    
    std::cout << "[Step 4] Building DFA (Subset Construction)..." << std::endl;
    Automaton dfa = buildDFA(nfa);
    printAutomaton(dfa, "Unminimized DFA");

    std::cout << "[Step 5] Minimizing DFA (Hopcroft Algorithm)..." << std::endl;
    Automaton minDfa = minimizeDFA(dfa);
    printAutomaton(minDfa, "Minimized DFA");
    
    std::cout << "[Success] DFA compiled with " << minDfa.states.size() << " states." << std::endl;
    return minDfa;
}