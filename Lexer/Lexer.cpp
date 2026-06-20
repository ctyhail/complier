#include "Lexer.h"

// 构造函数实现
Lexer::Lexer(const std::string& src) : source(src), cursor(0), line(1), column(1) {
    keywords = {
        {"const", TokenType::CONST}, {"var", TokenType::VAR}, 
        {"procedure", TokenType::PROCEDURE}, {"begin", TokenType::BEGIN}, 
        {"end", TokenType::END}, {"if", TokenType::IF}, {"then", TokenType::THEN}, 
        {"while", TokenType::WHILE}, {"do", TokenType::DO}, {"call", TokenType::CALL}, 
        {"read", TokenType::READ}, {"write", TokenType::WRITE}, {"odd", TokenType::ODD}
    };
}

// 辅助函数实现:安全地往前看字符
char Lexer::peek(int offset) {
    if (cursor + offset >= source.length()) return '\0';
    return source[cursor + offset];
}
// 基础辅助函数：吃掉当前字符，并更新行号列号
char Lexer::advance() {
    char c = source[cursor++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

Token Lexer::getNextToken() {
    while (cursor < source.length()) {
        char c = peek();
        int startCol = column; // 记录 Token 起始列号

        // 1. 跳过空白字符
        if (isspace(c)) {
            advance();
            continue;
        }

        // 2. 处理注释 (多行与单行)
        if (c == '/') {
            if (peek(1) == '/') { // 单行注释 //
                advance(); advance();
                while (peek() != '\n' && peek() != '\0') advance();
                continue; // 注释处理完，重新拿下一个 Token
            } 
            else if (peek(1) == '*') { // 多行注释 /* */
                advance(); advance();
                while (peek() != '\0') {
                    if (peek() == '*' && peek(1) == '/') {
                        advance(); advance();
                        break;
                    }
                    advance();
                }
                continue;
            }
        }

        // 3. 识别标识符或关键字 (修复下划线支持)
        // 标识符必须以字母或下划线开头
        if (isalpha(c) || c == '_') {
            std::string lexeme;
            // 后续字符可以是字母、数字或下划线
            while (isalnum(peek()) || peek() == '_') {
                lexeme += advance();
            }
            
            // 错误检查：长度超过 8 位
            if (lexeme.length() > 8) {
                return {TokenType::ERROR, lexeme, line, startCol, "Identifier length exceeds 8 characters"};
            }

            // 查表看是否是关键字
            if (keywords.find(lexeme) != keywords.end()) {
                return {keywords[lexeme], lexeme, line, startCol, ""};
            }
            return {TokenType::ID, lexeme, line, startCol, ""};
        }

        // 4. 识别数字与“非法单词”错误
        if (isdigit(c)) {
            std::string lexeme;
            while (isdigit(peek())) {
                lexeme += advance();
            }

            // 错误检查：非法单词（数字开头混杂字母，如 123abc）
            if (isalpha(peek())) {
                while (isalnum(peek())) lexeme += advance(); // 把剩下的吃完，防止陷入死循环
                return {TokenType::ERROR, lexeme, line, startCol, "Invalid word: starts with a digit"};
            }

            // 错误检查：无符号整数长度超过 8 位
            if (lexeme.length() > 8) {
                return {TokenType::ERROR, lexeme, line, startCol, "Number length exceeds 8 digits"};
            }

            return {TokenType::NUM, lexeme, line, startCol, ""};
        }

        // 5. 识别多字符运算符 (:=, <=, >=, <>)
        if (c == ':' && peek(1) == '=') { advance(); advance(); return {TokenType::ASSIGN, ":=", line, startCol, ""}; }
        if (c == '<' && peek(1) == '=') { advance(); advance(); return {TokenType::LE, "<=", line, startCol, ""}; }
        if (c == '>' && peek(1) == '=') { advance(); advance(); return {TokenType::GE, ">=", line, startCol, ""}; }
        if (c == '<' && peek(1) == '>') { advance(); advance(); return {TokenType::NEQ, "<>", line, startCol, ""}; }

        // 6. 识别单字符界符与运算符
        c = advance(); // 注意：这里才真正吃掉字符
        switch (c) {
            case '=': return {TokenType::EQ, "=", line, startCol, ""};
            case '<': return {TokenType::LT, "<", line, startCol, ""};
            case '>': return {TokenType::GT, ">", line, startCol, ""};
            case '+': return {TokenType::PLUS, "+", line, startCol, ""};
            case '-': return {TokenType::MINUS, "-", line, startCol, ""};
            case '*': return {TokenType::MUL, "*", line, startCol, ""};
            case '/': return {TokenType::DIV, "/", line, startCol, ""};
            case '(': return {TokenType::LPAREN, "(", line, startCol, ""};
            case ')': return {TokenType::RPAREN, ")", line, startCol, ""};
            case ',': return {TokenType::COMMA, ",", line, startCol, ""};
            case ';': return {TokenType::SEMI, ";", line, startCol, ""};
            case '.': return {TokenType::DOT, ".", line, startCol, ""};
            
            // 7. 错误恢复：非法字符 (如 @, &, !)
            default:
                std::string errStr(1, c);
                // 这里是恐慌模式的核心：我们返回了 ERROR，但游标已经 advance 了。
                // 这样下一次调用 getNextToken 时，词法分析器不会卡死在这个错误字符上，而是继续分析下一个合法单词。
                return {TokenType::ERROR, errStr, line, startCol, "Illegal character"};
        }
    }
    
    return {TokenType::END_OF_FILE, "EOF", line, column, ""};
}



