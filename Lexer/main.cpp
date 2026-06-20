#include <iostream>
#include "Lexer.h"

// 将上面的 enum TokenType 转换为字符串，用于打印
std::string tokenTypeToString(TokenType type) {
    if (type == TokenType::ID) return "ID";
    if (type == TokenType::NUM) return "NUM";
    if (type == TokenType::VAR) return "KW_VAR";
    if (type == TokenType::ERROR) return "ERROR";
    if (type == TokenType::ASSIGN) return "ASSIGN";
    // ... 可以自己补全其他的打印映射 ...
    return "SYMBOL"; 
}

int main() {
    std::string testCode = 
        "var abc, 123abcX;\n"
        "/* multi-line \n comment */\n"
        "abc := 100 + @;\n";

    Lexer lexer(testCode);
    Token t;
    
    std::cout << "Line\tCol\tType\t\tLexeme\t\tError/Note" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;

    do {
        t = lexer.getNextToken();
        
        std::cout << t.line << "\t" 
                  << t.column << "\t" 
                  << tokenTypeToString(t.type) << "\t\t" 
                  << t.lexeme;
                  
        if (t.type == TokenType::ERROR) {
            std::cout << "\t\t[!] " << t.errorMsg;
        }
        std::cout << std::endl;
        
    } while (t.type != TokenType::END_OF_FILE);

    return 0;
}