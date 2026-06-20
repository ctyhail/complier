// Parser.h
// PL/0 递归下降语法分析器
#pragma once

#include "../Lexer/Lexer.h"
#include <string>
#include <vector>
#include <set>
#include <iostream>

// 语法错误信息
struct SyntaxError {
    int line;
    std::string msg;
};

class Parser {
private:
    Lexer& lexer;
    Token current;                 // 当前 lookahead Token
    bool hasError;                 // 标记是否发生语法错误
    bool panicMode;                // 恐慌模式，用于抑制连续错误
    std::vector<SyntaxError> errors;

    // 词法辅助
    void advance();                // 读取下一个 Token
    bool check(TokenType type) const;
    bool match(TokenType type);    // 匹配则消费
    bool expect(TokenType type, const std::string& errMsg);

    // 错误处理
    void reportError(const std::string& msg);
    void reportError(const std::string& msg, int line);
    void synchronize(const std::set<TokenType>& syncSet);
    void consumeSyncAndReset(const std::set<TokenType>& syncSet);
    void syncAndReset(const std::set<TokenType>& syncSet);

    // 语法规则（与 PL/0 EBNF 对应）
    void program();
    void block();
    void constDecl();
    void constDef();
    void varDecl();
    void procDecl();
    void statement();
    void condition();
    void relOp();
    void expression();
    void term();
    void factor();

public:
    explicit Parser(Lexer& lex);

    // 执行语法分析，返回是否成功
    bool parse();

    // 获取错误列表
    const std::vector<SyntaxError>& getErrors() const { return errors; }

    // 打印错误
    void printErrors() const;
};
