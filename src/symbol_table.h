/**
 * symbol_table.h - PL/0符号表接口
 */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"

/* 符号种类 */
typedef enum {
    SYM_CONST,     /* 常量 */
    SYM_VAR,       /* 变量 */
    SYM_PROCEDURE  /* 过程 */
} SymKind;

/* 符号表条目 */
typedef struct {
    char name[MAX_IDENT_LEN + 1];  /* 符号名 */
    SymKind kind;                   /* 符号种类 */
    int value;                      /* 常量的值 */
    int scope;                      /* 作用域深度 */
    char proc_name[32];            /* 所属过程名 */
} SymEntry;

/* 符号表 */
typedef struct {
    SymEntry entries[MAX_SYMBOL_TABLE];
    int count;
    int scope_depth;
    char current_proc[32];
} SymTable;

/* 初始化符号表 */
void symtab_init(SymTable *st);

/* 查找符号（所有作用域），返回索引或-1 */
int symtab_lookup(SymTable *st, const char *name);

/* 在指定作用域查找 */
int symtab_lookup_scope(SymTable *st, const char *name, int depth);

/* 添加符号，成功返回索引，重复返回-1 */
int symtab_add(SymTable *st, const char *name, SymKind kind, int value);

/* 打印符号表 */
void symtab_print(SymTable *st);

/* 进入/退出作用域 */
void symtab_enter_scope(SymTable *st);
void symtab_exit_scope(SymTable *st);

/* 设置当前过程 */
void symtab_set_proc(SymTable *st, const char *proc_name);

#endif /* SYMBOL_TABLE_H */
