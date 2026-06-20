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
        std::cout << "Syntax correct" << std::endl;
    } else {
        parser.printErrors();
    }
    std::cout << std::endl;
}

int main() {
    // Test case 1: correct PL/0 program
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

    // Test case 2: const uses := (should report line 1)
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

    // Test case 3: while missing do (should report line 17)
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

    // Test case 4: multiple syntax errors (should report lines 1,2,10,11,13,16,17,20)
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

    testParse(test1, "Test case 1: correct program");
    testParse(test2, "Test case 2: const uses :=");
    testParse(test3, "Test case 3: while missing do");
    testParse(test4, "Test case 4: multiple syntax errors");

    return 0;
}
