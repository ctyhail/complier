#include <string>
#include <vector>

// 1. 词法 Token 定义
enum class TokenType {
    // 关键字
    KW_IF, KW_ELSE, KW_WHILE, KW_INT, KW_RETURN,
    // 标识符与数字
    IDENTIFIER, NUMBER,
    // 运算符
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_ASSIGN, OP_EQ,
    // 界符
    DELIM_LPAREN, DELIM_RPAREN, DELIM_LBRACE, DELIM_RBRACE, DELIM_SEMI,
    // 特殊状态
    ERROR, END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme; // 原始字符串，如 "123", "myVar", "+="
    int line;           // 行号
    int column;         // 列号
    std::string errorMsg; // 如果是 ERROR 类型，存储具体错误信息
};

// 2. 符号表定义 (语义分析用)
enum class SymbolKind { CONSTANT, VARIABLE, PROCEDURE };
struct Symbol {
    std::string name;
    SymbolKind kind;
    int val;       // 常量的值
    int level;     // 嵌套层级
    int address;   // 在运行栈中的相对地址
};

// 3. 目标代码指令 (P-Code)
enum class OpCode { LIT, OPR, LOD, STO, CAL, INT, JMP, JPC };
struct Instruction {
    OpCode f; // 功能码
    int l;    // 层次差
    int a;    // 操作数 (常量值、地址或跳转目标)
};