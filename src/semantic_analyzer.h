/**
 * semantic_analyzer.h - PL/0语义分析器接口
 */
#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "common.h"
#include "symbol_table.h"
#include "four_address_code.h"

/* 执行语义分析，返回0表示出错 */
int semantic_analyze(TokenQueue *tokens);

/* 获取四元式列表 */
QuadList* sem_get_quads(void);

/* 获取符号表 */
SymTable* sem_get_symtab(void);

#endif /* SEMANTIC_ANALYZER_H */
