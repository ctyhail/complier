#include "Parser.h"

Parser::Parser(Lexer& lex) : lexer(lex), hasError(false), panicMode(false) {
    advance(); // 预读第一个 Token
}

void Parser::advance() {
    current = lexer.getNextToken();
    // 如果遇到词法错误，也记录为语法/词法错误，但继续分析
    if (current.type == TokenType::ERROR && !panicMode) {
        reportError("Lexical error: " + current.errorMsg);
    }
}

bool Parser::check(TokenType type) const {
    return current.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

// 消费同步符号并退出恐慌模式
void Parser::consumeSyncAndReset(const std::set<TokenType>& syncSet) {
    synchronize(syncSet);
    if (syncSet.count(current.type) && current.type != TokenType::END_OF_FILE) {
        advance();
    }
    panicMode = false;
}

bool Parser::expect(TokenType type, const std::string& errMsg) {
    if (check(type)) {
        advance();
        // 成功匹配到同步符号，退出恐慌模式
        if (type == TokenType::SEMI || type == TokenType::END ||
            type == TokenType::DOT || type == TokenType::RPAREN) {
            panicMode = false;
        }
        return true;
    }
    reportError(errMsg);
    // 跳到下一个同步符号但不消费，由上层决定如何恢复
    synchronize({TokenType::SEMI, TokenType::END, TokenType::DOT, TokenType::RPAREN, TokenType::END_OF_FILE});
    return false;
}

void Parser::reportError(const std::string& msg) {
    if (!panicMode) {
        hasError = true;
        errors.push_back({current.line, msg});
        panicMode = true;
    }
}

void Parser::synchronize(const std::set<TokenType>& syncSet) {
    while (current.type != TokenType::END_OF_FILE) {
        if (syncSet.count(current.type)) {
            return;
        }
        advance();
    }
}

// <程序> ::= <分程序> .
void Parser::program() {
    block();
    if (!expect(TokenType::DOT, "Expected '.' at the end of program")) {
        consumeSyncAndReset({TokenType::DOT});
    }
}

// <分程序> ::= [<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句>
void Parser::block() {
    if (check(TokenType::CONST)) {
        constDecl();
    }
    if (check(TokenType::VAR)) {
        varDecl();
    }
    if (check(TokenType::PROCEDURE)) {
        procDecl();
    }
    statement();
}

// <常量说明部分> ::= const <常量定义>{,<常量定义>};
void Parser::constDecl() {
    advance(); // consume const
    constDef();
    while (check(TokenType::COMMA)) {
        advance();
        constDef();
    }
    if (!expect(TokenType::SEMI, "Expected ';' after const declaration")) {
        consumeSyncAndReset({TokenType::SEMI});
    }
}

// <常量定义> ::= <标识符>=<无符号整数>
void Parser::constDef() {
    if (!expect(TokenType::ID, "Expected identifier in const definition")) return;
    if (!expect(TokenType::EQ, "Expected '=' in const definition")) return;
    expect(TokenType::NUM, "Expected number in const definition");
}

// <变量说明部分> ::= var <标识符>{,<标识符>};
void Parser::varDecl() {
    advance(); // consume var
    if (!expect(TokenType::ID, "Expected identifier in var declaration")) {
        consumeSyncAndReset({TokenType::SEMI});
        return;
    }
    while (check(TokenType::COMMA)) {
        advance();
        if (!expect(TokenType::ID, "Expected identifier after ',' in var declaration")) {
            consumeSyncAndReset({TokenType::SEMI});
            return;
        }
    }
    if (!expect(TokenType::SEMI, "Expected ';' after var declaration")) {
        consumeSyncAndReset({TokenType::SEMI});
    }
}

// <过程说明部分> ::= <过程首部><分程序>{;<过程说明部分>};
// 实际实现为：procedure id ; block ; { procedure id ; block ; }
void Parser::procDecl() {
    advance(); // consume procedure
    if (!expect(TokenType::ID, "Expected procedure name")) {
        consumeSyncAndReset({TokenType::SEMI});
        return;
    }
    if (!expect(TokenType::SEMI, "Expected ';' after procedure name")) {
        consumeSyncAndReset({TokenType::SEMI});
    }
    block();
    if (!expect(TokenType::SEMI, "Expected ';' after procedure body")) {
        consumeSyncAndReset({TokenType::SEMI});
    }

    // 递归处理多个过程声明
    if (check(TokenType::PROCEDURE)) {
        procDecl();
    }
}

// <语句> ::=
//   <赋值语句> | <条件语句> | <当型循环语句> | <过程调用语句> |
//   <复合语句> | <读语句> | <写语句> | ε
void Parser::statement() {
    // 空语句：遇到语句结束符直接返回
    if (check(TokenType::SEMI) || check(TokenType::END) ||
        check(TokenType::DOT) || check(TokenType::END_OF_FILE)) {
        return;
    }

    switch (current.type) {
        case TokenType::ID:
            // <赋值语句> ::= <标识符>:=<表达式>
            advance();
            if (!expect(TokenType::ASSIGN, "Expected ':=' in assignment")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                return;
            }
            expression();
            break;

        case TokenType::IF: {
            // <条件语句> ::= if <条件> then <语句>
            advance();
            condition();
            if (check(TokenType::BEGIN)) {
                // 错误恢复：缺少 then，但下一个是 begin，继续解析语句体
                reportError("Expected 'then' after condition");
                statement();
            } else {
                if (!expect(TokenType::THEN, "Expected 'then' after condition")) {
                    consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                    return;
                }
                statement();
            }
            break;
        }

        case TokenType::WHILE: {
            // <当型循环语句> ::= while <条件> do <语句>
            advance();
            condition();
            if (check(TokenType::BEGIN)) {
                // 错误恢复：缺少 do，但下一个是 begin，继续解析语句体
                reportError("Expected 'do' after condition");
                statement();
            } else {
                if (!expect(TokenType::DO, "Expected 'do' after condition")) {
                    consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                    return;
                }
                statement();
            }
            break;
        }

        case TokenType::CALL:
            // <过程调用语句> ::= call <标识符>
            advance();
            if (!expect(TokenType::ID, "Expected procedure name after call")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                return;
            }
            break;

        case TokenType::BEGIN: {
            // <复合语句> ::= begin <语句>{;<语句>} end
            // 允许末尾多余分号，如 begin a := 1; end
            advance();
            if (!check(TokenType::END)) {
                statement();
                while (check(TokenType::SEMI)) {
                    advance();
                    if (check(TokenType::END)) break; // 容忍末尾分号
                    statement();
                }
            }
            if (!expect(TokenType::END, "Expected 'end' after begin")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
            }
            break;
        }

        case TokenType::READ:
            // <读语句> ::= read(<标识符>{,<标识符>})
            advance();
            if (!expect(TokenType::LPAREN, "Expected '(' after read")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                return;
            }
            if (!expect(TokenType::ID, "Expected identifier in read")) {
                consumeSyncAndReset({TokenType::RPAREN, TokenType::SEMI, TokenType::END});
                return;
            }
            while (check(TokenType::COMMA)) {
                advance();
                if (!expect(TokenType::ID, "Expected identifier after ',' in read")) {
                    consumeSyncAndReset({TokenType::RPAREN, TokenType::SEMI, TokenType::END});
                    return;
                }
            }
            if (!expect(TokenType::RPAREN, "Expected ')' after read arguments")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                return;
            }
            break;

        case TokenType::WRITE:
            // <写语句> ::= write(<表达式>{,<表达式>})
            advance();
            if (!expect(TokenType::LPAREN, "Expected '(' after write")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                return;
            }
            expression();
            while (check(TokenType::COMMA)) {
                advance();
                expression();
            }
            if (!expect(TokenType::RPAREN, "Expected ')' after write arguments")) {
                consumeSyncAndReset({TokenType::SEMI, TokenType::END});
                return;
            }
            break;

        default:
            reportError("Unexpected token at start of statement");
            consumeSyncAndReset({TokenType::SEMI, TokenType::END});
            break;
    }
}

// <条件> ::= <表达式><关系运算符><表达式> | odd <表达式>
void Parser::condition() {
    if (match(TokenType::ODD)) {
        expression();
    } else {
        expression();
        relOp();
        expression();
    }
}

// <关系运算符> ::= =|#|<>|<|<=|>|>=
void Parser::relOp() {
    if (check(TokenType::EQ) || check(TokenType::NEQ) ||
        check(TokenType::LT) || check(TokenType::LE) ||
        check(TokenType::GT) || check(TokenType::GE)) {
        advance();
    } else {
        reportError("Expected relational operator");
        synchronize({TokenType::PLUS, TokenType::MINUS, TokenType::ID,
                     TokenType::NUM, TokenType::LPAREN, TokenType::THEN,
                     TokenType::DO, TokenType::SEMI, TokenType::END});
    }
}

// <表达式> ::= [+|-]<项>{(+|-)<项>}
void Parser::expression() {
    if (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        advance(); // 一元正负号
    }
    term();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        advance();
        term();
    }
}

// <项> ::= <因子>{(*|/)<因子>}
void Parser::term() {
    factor();
    while (check(TokenType::MUL) || check(TokenType::DIV)) {
        advance();
        factor();
    }
}

// <因子> ::= <标识符> | <无符号整数> | (<表达式>)
void Parser::factor() {
    if (check(TokenType::ID) || check(TokenType::NUM)) {
        advance();
    } else if (check(TokenType::LPAREN)) {
        advance();
        expression();
        if (!expect(TokenType::RPAREN, "Expected ')' after expression")) {
            consumeSyncAndReset({TokenType::RPAREN});
        }
    } else {
        reportError("Expected identifier, number or '('");
        synchronize({TokenType::PLUS, TokenType::MINUS, TokenType::MUL,
                     TokenType::DIV, TokenType::EQ, TokenType::NEQ,
                     TokenType::LT, TokenType::LE, TokenType::GT,
                     TokenType::GE, TokenType::RPAREN, TokenType::COMMA,
                     TokenType::SEMI, TokenType::THEN, TokenType::DO,
                     TokenType::END, TokenType::END_OF_FILE});
    }
}

bool Parser::parse() {
    program();
    if (!check(TokenType::END_OF_FILE)) {
        reportError("Unexpected token after program end");
    }
    return !hasError;
}

void Parser::printErrors() const {
    for (const auto& err : errors) {
        std::cout << "(语法错误,行号:" << err.line << ")" << std::endl;
    }
}
