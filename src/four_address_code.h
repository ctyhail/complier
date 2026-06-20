/**
 * four_address_code.h - 四元式中间代码接口
 */
#ifndef FOUR_ADDRESS_CODE_H
#define FOUR_ADDRESS_CODE_H

#include "common.h"

/* 四元式操作码 */
typedef enum {
    QUAD_SYSS,      /* 程序开始 syss */
    QUAD_SYSE,      /* 程序结束 syse */
    QUAD_CONST,     /* 常量声明 const */
    QUAD_VAR,       /* 变量声明 var */
    QUAD_PROC,      /* 过程声明 procedure */
    QUAD_ADD,       /* + 加法 */
    QUAD_SUB,       /* - 减法 */
    QUAD_MUL,       /* * 乘法 */
    QUAD_DIV,       /* / 除法 */
    QUAD_ASSIGN,    /* = 赋值 */
    QUAD_EQ,        /* j= 条件跳转(相等) */
    QUAD_NEQ,       /* j# 条件跳转(不等) */
    QUAD_LT,        /* j< 条件跳转(小于) */
    QUAD_LE,        /* j<= 条件跳转(小于等于) */
    QUAD_GT,        /* j> 条件跳转(大于) */
    QUAD_GE,        /* j>= 条件跳转(大于等于) */
    QUAD_JMP,       /* j 无条件跳转 */
    QUAD_READ,      /* read 读入 */
    QUAD_WRITE,     /* write 输出 */
    QUAD_CALL,      /* call 过程调用 */
    QUAD_RET        /* ret 过程返回 */
} QuadOp;

/* 四元式结构 */
typedef struct {
    QuadOp op;
    char arg1[16];
    char arg2[16];
    char result[16];
} Quad;

/* 四元式列表 */
typedef struct {
    Quad quads[MAX_QUAD_COUNT];
    int count;
} QuadList;

/* 初始化 */
void quad_init(QuadList *ql);

/* 发射一条四元式，返回索引 */
int quad_emit(QuadList *ql, QuadOp op, const char *arg1,
              const char *arg2, const char *result);

/* 临时变量管理 */
void quad_reset_temp(void);
char* quad_new_temp(char *buf, int buf_size);

/* 修补跳转目标 */
void quad_patch(QuadList *ql, int quad_index, int target);

/* 获取下一条四元式序号 */
int quad_next_idx(QuadList *ql);

/* 打印四元式列表 */
void quad_print(QuadList *ql);

/* 操作码转字符串 */
const char* quad_op_to_str(QuadOp op);

#endif /* FOUR_ADDRESS_CODE_H */
