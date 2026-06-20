/**
 * common.h - 编译原理课程设计：PL/0编译器公共头文件
 * 定义所有模块共享的数据结构、令牌类型和常量
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_IDENT_LEN 8      /* 标识符最大长度 */
#define MAX_NUM_LEN 8        /* 无符号整数最大位数 */
#define MAX_LINE_LEN 1024    /* 单行最大长度 */
#define MAX_TOKEN_COUNT 4096 /* 最大Token数量 */
#define MAX_SYMBOL_TABLE 256 /* 符号表最大容量 */
#define MAX_QUAD_COUNT 1024  /* 四元式最大数量 */

/* ========== 令牌(Token)类型定义 ========== */
typedef enum {
    /* 保留字/关键字 (按字母序) */
    TOK_BEGIN = 0,      /* begin */
    TOK_CALL,           /* call */
    TOK_CONST,          /* const */
    TOK_DO,             /* do */
    TOK_ELSE,           /* else (扩展) */
    TOK_END,            /* end */
    TOK_IF,             /* if */
    TOK_ODD,            /* odd */
    TOK_PROCEDURE,      /* procedure */
    TOK_READ,           /* read */
    TOK_THEN,           /* then */
    TOK_VAR,            /* var */
    TOK_WHILE,          /* while */
    TOK_WRITE,          /* write */

    /* 运算符 */
    TOK_PLUS,           /* + */
    TOK_MINUS,          /* - */
    TOK_MUL,            /* * */
    TOK_DIV,            /* / */
    TOK_EQ,             /* = */
    TOK_NEQ,            /* # */
    TOK_LT,             /* < */
    TOK_LE,             /* <= */
    TOK_GT,             /* > */
    TOK_GE,             /* >= */
    TOK_ASSIGN,         /* := */

    /* 界符 */
    TOK_LPAREN,         /* ( */
    TOK_RPAREN,         /* ) */
    TOK_COMMA,          /* , */
    TOK_SEMICOLON,      /* ; */
    TOK_PERIOD,         /* . */

    /* 其他 */
    TOK_IDENT,          /* 标识符 */
    TOK_NUMBER,         /* 无符号整数 */
    TOK_EOF,            /* 文件结束 */
    TOK_ERROR,          /* 错误/非法字符 */
    TOK_ERR_IDENT_LEN,  /* 标识符长度超长 */
    TOK_ERR_NUM_LEN,    /* 无符号整数越界 */
    TOK_ERR_ILLEGAL_CHAR, /* 非法字符 */
    TOK_ERR_ILLEGAL_WORD  /* 非法单词(数字开头) */
} TokenType;

/* ========== Token结构体 ========== */
typedef struct {
    TokenType type;        /* 令牌类型 */
    char lexeme[32];       /* 单词原文 */
    int line;              /* 所在行号 */
    int col;               /* 所在列号 */
    int value;             /* 如果是数字，存数值 */
} Token;

/* ========== 关键字映射 ========== */
typedef struct {
    const char *word;
    TokenType type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"begin",     TOK_BEGIN},
    {"call",      TOK_CALL},
    {"const",     TOK_CONST},
    {"do",        TOK_DO},
    {"else",      TOK_ELSE},
    {"end",       TOK_END},
    {"if",        TOK_IF},
    {"odd",       TOK_ODD},
    {"procedure", TOK_PROCEDURE},
    {"read",      TOK_READ},
    {"then",      TOK_THEN},
    {"var",       TOK_VAR},
    {"while",     TOK_WHILE},
    {"write",     TOK_WRITE},
    {NULL,        TOK_EOF}
};

static const int keyword_count = 14;

/* ========== Token类型名称映射（用于输出） ========== */
static const char *token_type_name[] = {
    "保留字", "保留字", "保留字", "保留字", "保留字",  /* begin~else */
    "保留字", "保留字", "保留字", "保留字", "保留字",  /* end~procedure */
    "保留字", "保留字", "保留字", "保留字",             /* read~write */
    "运算符", "运算符", "运算符", "运算符",             /* + - * / */
    "运算符", "运算符", "运算符", "运算符",             /* = # < <= */
    "运算符", "运算符", "运算符",                       /* > >= := */
    "界符", "界符", "界符", "界符", "界符",             /* ( ) , ; . */
    "标识符", "无符号整数",
    "文件结束", "错误",
    "非法单词", "非法字符", "数字越界"
};

/* ========== Token类型到保留字串的映射 ========== */
static const char *token_type_str[] = {
    "begin","call","const","do","else","end","if",
    "odd","procedure","read","then","var","while","write",
    "+","-","*","/","=","#","<","<=",">",">=",":=",
    "(",")",",",";",".",
    "标识符","无符号整数",
    "文件结束","错误",
    "标识符长度超长","非法字符(串)","无符号整数越界"
};

/* ========== 标识符Token类型判断 ========== */
static inline int is_keyword(TokenType t)
{
    return t >= TOK_BEGIN && t <= TOK_WRITE;
}

static inline int is_operator(TokenType t)
{
    return t >= TOK_PLUS && t <= TOK_ASSIGN;
}

static inline int is_delimiter(TokenType t)
{
    return t >= TOK_LPAREN && t <= TOK_PERIOD;
}

/* ========== Token队列 ========== */
typedef struct {
    Token tokens[MAX_TOKEN_COUNT];
    int count;
    int pos;
} TokenQueue;

static inline void token_queue_init(TokenQueue *q)
{
    q->count = 0;
    q->pos = 0;
}

static inline int token_queue_add(TokenQueue *q, Token t)
{
    if (q->count >= MAX_TOKEN_COUNT) return 0;
    q->tokens[q->count++] = t;
    return 1;
}

static inline Token token_queue_peek(TokenQueue *q)
{
    if (q->pos < q->count) return q->tokens[q->pos];
    Token eof = {TOK_EOF, "", 0, 0, 0};
    return eof;
}

static inline Token token_queue_next(TokenQueue *q)
{
    if (q->pos < q->count) return q->tokens[q->pos++];
    Token eof = {TOK_EOF, "", 0, 0, 0};
    return eof;
}

static inline void token_queue_reset(TokenQueue *q)
{
    q->pos = 0;
}

/* ========== 错误处理 ========== */
static int error_count = 0;

static inline void report_error(int line, const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "[ERROR line:%d] ", line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    error_count++;
}

static inline void report_semantic_error(int line, const char *fmt, ...)
{
    va_list args;
    printf("(SEMANTIC_ERROR, line:%d)\n", line);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    error_count++;
}

static inline int get_error_count(void) { return error_count; }
static inline void reset_error_count(void) { error_count = 0; }

#endif /* COMMON_H */
