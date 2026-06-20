/**
 * semantic_analyzer.c - PL/0语义分析器（L-翻译模式）
 *
 * 基于词法分析和递归下降语法分析的结果，
 * 执行语义检查并生成四元式中间代码。
 *
 * 功能：
 *   1. 变量/常量/过程的声明检查
 *   2. 类型匹配检查
 *   3. 四元式中间代码生成
 *   4. 符号表管理
 */

#include "semantic_analyzer.h"
#include "lexical_analyzer.h"
#include "syntax_analyzer.h"
#include "symbol_table.h"
#include "four_address_code.h"
#include <stdio.h>
#include <string.h>

/* 语义分析状态 */
typedef struct {
    SymTable symtab;
    QuadList quads;
    int has_semantic_error;
} SemState;

static SemState sem_state;

/* ========== 语义检查函数 ========== */

/* 检查标识符是否已声明 */
int sem_check_declared(const char *name, int line)
{
    if (symtab_lookup(&sem_state.symtab, name) < 0) {
        report_semantic_error(line, "未声明的标识符: %s", name);
        sem_state.has_semantic_error = 1;
        return 0;
    }
    return 1;
}

/* 检查标识符是否为变量（可赋值） */
int sem_check_assignable(const char *name, int line)
{
    int idx = symtab_lookup(&sem_state.symtab, name);
    if (idx < 0) {
        report_semantic_error(line, "未声明的标识符: %s", name);
        sem_state.has_semantic_error = 1;
        return 0;
    }
    if (sem_state.symtab.entries[idx].kind == SYM_CONST) {
        report_semantic_error(line, "Cannot assign to constant: %s", name);
        sem_state.has_semantic_error = 1;
        return 0;
    }
    return 1;
}

/* 检查标识符是否为过程 */
int sem_check_procedure(const char *name, int line)
{
    int idx = symtab_lookup(&sem_state.symtab, name);
    if (idx < 0) {
        report_semantic_error(line, "Undeclared procedure: %s", name);
        sem_state.has_semantic_error = 1;
        return 0;
    }
    if (sem_state.symtab.entries[idx].kind != SYM_PROCEDURE) {
        report_semantic_error(line, "'%s' is not a procedure", name);
        sem_state.has_semantic_error = 1;
        return 0;
    }
    return 1;
}

/* 检查重复声明 */
int sem_check_duplicate(const char *name, int line)
{
    if (symtab_lookup_scope(&sem_state.symtab, name,
                            sem_state.symtab.scope_depth) >= 0) {
        report_semantic_error(line, "Duplicate declaration: %s", name);
        sem_state.has_semantic_error = 1;
        return 0;
    }
    return 1;
}

/* 使用语法分析器进行集成语义分析 */
int semantic_analyze(TokenQueue *tokens)
{
    /* 初始化 */
    sem_state.has_semantic_error = 0;
    symtab_init(&sem_state.symtab);
    quad_init(&sem_state.quads);
    quad_reset_temp();
    reset_error_count();

    /* 初始化并运行语法分析器（集成语义动作） */
    syntax_init(tokens);
    int syntax_ok = syntax_parse();

    /* 如果有语法错误，语义分析也无法正常进行 */
    if (!syntax_ok) {
        printf("\n语义分析因语法错误中止\n");
        return 0;
    }

    /* 复制符号表和四元式结果 */
    sem_state.symtab = *syntax_get_symtab();
    sem_state.quads = *syntax_get_quads();

    printf("语义正确\n\n");
    quad_print(&sem_state.quads);
    printf("\n");
    symtab_print(&sem_state.symtab);

    return !sem_state.has_semantic_error;
}

/* 获取四元式列表 */
QuadList* sem_get_quads(void)
{
    return &sem_state.quads;
}

/* 获取符号表 */
SymTable* sem_get_symtab(void)
{
    return &sem_state.symtab;
}
