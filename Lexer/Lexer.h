// Lexer.h
#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <cctype>

// 1. 单词分类表 (TokenType)
enum class TokenType {
    // 关键字
    CONST, VAR, PROCEDURE, BEGIN, END, IF, THEN, WHILE, DO, CALL, READ, WRITE, ODD,
    // 标识符与数字
    ID, NUM,
    // 运算符与界符
    ASSIGN, EQ, NEQ, LT, LE, GT, GE, PLUS, MINUS, MUL, DIV,
    LPAREN, RPAREN, COMMA, SEMI, DOT,
    // 控制与错误
    END_OF_FILE, ERROR
};

// 2. Token 数据结构
struct Token {
    TokenType type;
    std::string lexeme;   // 原始字符串
    int line;             // 所在行
    int column;           // 所在列
    std::string errorMsg; // 仅在 type == ERROR 时使用
};

// 3. Lexer 类声明
class Lexer {
private:
    std::string source;
    int cursor;
    int line;
    int column;
    std::unordered_map<std::string, TokenType> keywords;

    char peek(int offset = 0);
    char advance();

public:
    Lexer(const std::string& src);
    Token getNextToken(); 
};