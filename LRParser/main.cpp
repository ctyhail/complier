#include <iostream>
#include <vector>
#include <string>
#include "LRParser.h"

void runTest(const std::vector<std::string>& input, const std::string& title) {
    std::cout << "==============================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "------------------------------" << std::endl;

    LRParser parser;
    parser.parse(input);
    std::cout << std::endl;
}

int main() {
    LRParser parser;

    // 1. 打印文法
    parser.printGrammar();

    // 2. 打印 FOLLOW 集
    parser.printFirstFollow();

    // 3. 打印识别活前缀的 DFA
    parser.printDFA();

    // 4. 打印 SLR(1) 分析表
    parser.printTable();

    // 5. 测试用例
    // 测试用例 1：正确输入 id + id * id
    runTest({"id", "+", "id", "*", "id"}, "Test 1: id + id * id (correct)");

    // 测试用例 2：正确输入 ( id + id )
    runTest({"(", "id", "+", "id", ")"}, "Test 2: ( id + id ) (correct)");

    // 测试用例 3：正确输入 id * id + id
    runTest({"id", "*", "id", "+", "id"}, "Test 3: id * id + id (correct)");

    // 测试用例 4：错误输入 id + * id
    runTest({"id", "+", "*", "id"}, "Test 4: id + * id (error)");

    return 0;
}
