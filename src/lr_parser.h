/**
 * lr_parser.h - PL/0 LR语法分析器接口
 */
#ifndef LR_PARSER_H
#define LR_PARSER_H

#include "common.h"

#define MAX_STACK 256
#define MAX_STATE 32

/* LR分析栈 */
typedef struct {
    int states[MAX_STACK];
    Token symbols[MAX_STACK];
    int top;
} LRStack;

/* 初始化LR分析栈 */
void lr_stack_init(LRStack *s);

/* 压栈 */
void lr_stack_push(LRStack *s, int state, Token *sym);

/* 出栈 */
void lr_stack_pop(LRStack *s);

/* 获取栈顶状态 */
int lr_stack_top_state(LRStack *s);

/* 获取栈顶符号 */
Token lr_stack_top_symbol(LRStack *s);

/* 构建LR项目集 */
void lr_build_items(void);

/* 打印活前缀DFA */
void lr_print_live_prefix_dfa(void);

/* 构建LR分析表 */
void lr_build_table(void);

/* 模拟LR分析过程 */
void lr_parse_simulation(TokenQueue *tokens);

/* 完整LR语法分析入口 */
int lr_parse(TokenQueue *tokens);

#endif /* LR_PARSER_H */
