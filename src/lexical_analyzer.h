/**
 * lexical_analyzer.h - PL/0词法分析器接口
 */
#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include "common.h"

/* 初始化词法分析器（从文件读取） */
void lex_init_file(const char *filename);

/* 初始化词法分析器（从字符串读取） */
void lex_init_string(const char *input);

/* 关闭词法分析器 */
void lex_close(void);

/* 获取下一个Token */
Token lex_get_next_token(void);

/* 打印Token的标准格式 */
void lex_print_token(Token *t);

/* 完整词法分析，返回Token队列 */
TokenQueue lex_analyze_all(void);

#endif /* LEXICAL_ANALYZER_H */
