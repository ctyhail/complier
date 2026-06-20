/**
 * symbol_table.c - PL/0符号表实现
 * 管理常量、变量和过程的符号信息
 */

#include "symbol_table.h"
#include <stdio.h>
#include <string.h>

void symtab_init(SymTable *st)
{
    st->count = 0;
    st->scope_depth = 0;
    st->current_proc[0] = '\0';
}

int symtab_lookup(SymTable *st, const char *name)
{
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int symtab_lookup_scope(SymTable *st, const char *name, int depth)
{
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0 &&
            st->entries[i].scope == depth) {
            return i;
        }
    }
    return -1;
}

int symtab_add(SymTable *st, const char *name, SymKind kind, int value)
{
    /* 检查是否已存在（同作用域） */
    if (symtab_lookup_scope(st, name, st->scope_depth) >= 0) {
        return -1; /* 重复声明 */
    }

    if (st->count >= MAX_SYMBOL_TABLE) return -1;

    SymEntry *e = &st->entries[st->count];
    strncpy(e->name, name, MAX_IDENT_LEN);
    e->name[MAX_IDENT_LEN] = '\0';
    e->kind = kind;
    e->value = (kind == SYM_CONST) ? value : 0;
    e->scope = st->scope_depth;
    strncpy(e->proc_name, st->current_proc, 32);
    st->count++;
    return st->count - 1;
}

void symtab_print(SymTable *st)
{
    printf("Symbol table:\n");
    for (int i = 0; i < st->count; i++) {
        SymEntry *e = &st->entries[i];
        const char *kind_str[] = {"const", "var", "procedure"};
        printf("%s %s", kind_str[e->kind], e->name);
        if (e->kind == SYM_CONST) {
            printf(" %d", e->value);
        }
        printf("\n");
    }
}

void symtab_enter_scope(SymTable *st)
{
    st->scope_depth++;
}

void symtab_exit_scope(SymTable *st)
{
    /* 删除当前作用域的所有符号（跳过顶层 scope_depth=1，保留程序级声明） */
    if (st->scope_depth <= 1) {
        st->scope_depth--;
        return;
    }
    int i = 0;
    while (i < st->count) {
        if (st->entries[i].scope == st->scope_depth) {
            /* 用最后一个元素覆盖 */
            st->entries[i] = st->entries[st->count - 1];
            st->count--;
        } else {
            i++;
        }
    }
    st->scope_depth--;
}

void symtab_set_proc(SymTable *st, const char *proc_name)
{
    strncpy(st->current_proc, proc_name, 32);
    st->current_proc[31] = '\0';
}
