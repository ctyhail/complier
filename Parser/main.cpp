#include <iostream>
#include <string>
#include "../Lexer/Lexer.h"
#include "Parser.h"

void testParse(const std::string& code, const std::string& title) {
    std::cout << "==============================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "------------------------------" << std::endl;

    Lexer lexer(code);
    Parser parser(lexer);

    bool ok = parser.parse();
    if (ok) {
        std::cout << "语法正确" << std::endl;
    } else {
        parser.printErrors();
    }
    std::cout << std::endl;
}

int main() {
    // 测试用例 1：正确的 PL/0 程序
    std::string test1 =
        "const a = 10;\n"
        "var   b, c;\n"
        "\n"
        "procedure p;\n"
        "    if a <= 10 then\n"
        "        begin\n"
        "            c := b + a;\n"
        "        end;\n"
        "begin\n"
        "    read(b);\n"
        "    while b # 0 do\n"
        "        begin\n"
        "            call p;\n"
        "            write(2 * c);\n"
        "            read(b);\n"
        "        end\n"
        "end.\n";

    // 测试用例 2：const 后使用 := 错误（应报第 1 行）
    std::string test2 =
        "const a := 10;\n"
        "var   b, c;\n"
        "procedure p;\n"
        "    if a <= 10 then\n"
        "        begin\n"
        "            c := b + a;\n"
        "        end;\n"
        "begin\n"
        "    read(b);\n"
        "    while b # 0 do\n"
        "        begin\n"
        "            call p;\n"
        "            write(2 * c);\n"
        "            read(b);\n"
        "        end\n"
        "end.\n";

    // 测试用例 3：while 缺少 do（应报第 17 行）
    std::string test3 =
        "const a = 10;\n"
        "var   b, c;\n"
        "\n"
        "//单行注释\n"
        "\n"
        "/*\n"
        "* 多行注释\n"
        "*/\n"
        "\n"
        "procedure p;\n"
        "    if a <= 10 then\n"
        "        begin\n"
        "            c := b + a;\n"
        "        end;\n"
        "begin\n"
        "    read(b);\n"
        "    while b # 0\n"
        "        begin\n"
        "            call p;\n"
        "            write(2 * c);\n"
        "            read(b);\n"
        "        end\n"
        "end.\n";

    // 测试用例 4：多个语法错误（应报 1,2,10,11,13,16,17,20 行）
    std::string test4 =
        "const a := 10;\n"
        "var   b, c d;\n"
        "\n"
        "//单行注释\n"
        "\n"
        "/*\n"
        "* 多行注释\n"
        "*/\n"
        "\n"
        "procedure procedure fun1;\n"
        "    if a <= 10\n"
        "        begin\n"
        "            c = b + a\n"
        "        end;\n"
        "begin\n"
        "    read(b;\n"
        "    while b # 0\n"
        "        begin\n"
        "            call fun1;\n"
        "            write 2 * c);\n"
        "            read(b);\n"
        "        end\n"
        "end.\n";

    testParse(test1, "测试用例 1：正确程序");
    testParse(test2, "测试用例 2：const 使用 :=");
    testParse(test3, "测试用例 3：while 缺少 do");
    testParse(test4, "测试用例 4：多处语法错误");

    return 0;
}
