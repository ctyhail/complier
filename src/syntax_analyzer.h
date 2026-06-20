/**
 * syntax_analyzer.h - PL/0递归下降语法分析器接口
 *
 * 基于递归下降法的PL/0语法分析器
 * 集成语义动作，生成四元式中间代码
 */

#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "common.h"
#include "symbol_table.h"
#include "four_address_code.h"

/* 初始化语法分析器 */
void syntax_init(TokenQueue *q);

/* 执行语法分析，返回0表示出错 */
int syntax_parse(void);

/* 检查是否有语法错误 */
int syntax_has_error(void);

/* 获取符号表 */
SymTable* syntax_get_symtab(void);

/* 获取四元式列表 */
QuadList* syntax_get_quads(void);

#endif /* SYNTAX_ANALYZER_H */
