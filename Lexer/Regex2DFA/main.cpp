#include <iostream>
#include "RegexEngine.h"

int main() {
    RegexEngine engine;
    std::string regex = "(a|b)*abb";
    
    std::cout << "Testing Regex: " << regex << "\n" << std::endl;
    
    // 1. 运行流水线
    Automaton minDfa = engine.compileToDFA(regex);
    
    // 2. 导出为可视化文件 (可选)
    // 你可以在 Linux 终端使用 dot -Tpng dfa.dot -o dfa.png 命令将其渲染为图片
    engine.generateDotFile(minDfa, "minDfa_output.dot");
    
    std::cout << "\nCompilation Pipeline Finished Successfully!" << std::endl;
    return 0;
}