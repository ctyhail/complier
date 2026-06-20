/**
 * four_address_code.c - 四元式中间代码生成与输出
 * 实现PL/0编译器的四元式中间代码生成
 */

#include "four_address_code.h"
#include <stdio.h>
#include <string.h>

void quad_init(QuadList *ql)
{
    ql->count = 0;
}

int quad_emit(QuadList *ql, QuadOp op, const char *arg1,
              const char *arg2, const char *result)
{
    if (ql->count >= MAX_QUAD_COUNT) return -1;

    int idx = ql->count;
    Quad *q = &ql->quads[idx];
    q->op = op;
    strncpy(q->arg1, arg1 ? arg1 : "_", 16);
    strncpy(q->arg2, arg2 ? arg2 : "_", 16);
    strncpy(q->result, result ? result : "_", 16);
    ql->count++;
    return idx;
}

/* 临时变量管理 */
static int temp_counter = 0;

void quad_reset_temp(void)
{
    temp_counter = 0;
}

char* quad_new_temp(char *buf, int buf_size)
{
    snprintf(buf, buf_size, "T%d", ++temp_counter);
    return buf;
}

/* 修补跳转目标 */
void quad_patch(QuadList *ql, int quad_index, int target)
{
    if (quad_index >= 0 && quad_index < ql->count) {
        char target_str[16];
        snprintf(target_str, sizeof(target_str), "$%d", target + 1);
        strncpy(ql->quads[quad_index].result, target_str, 16);
    }
}

/* 获取下一条四元式序号（用于跳转目标） */
int quad_next_idx(QuadList *ql)
{
    return ql->count;
}

/* 打印四元式 */
void quad_print(QuadList *ql)
{
    printf("\nIntermediate code (quadruples):\n");
    for (int i = 0; i < ql->count; i++) {
        Quad *q = &ql->quads[i];
        const char *op_name;

        switch (q->op) {
            case QUAD_SYSS:     op_name = "syss";   break;
            case QUAD_SYSE:     op_name = "syse";   break;
            case QUAD_CONST:    op_name = "const";  break;
            case QUAD_VAR:      op_name = "var";    break;
            case QUAD_PROC:     op_name = "procedure"; break;
            case QUAD_ADD:      op_name = "+";      break;
            case QUAD_SUB:      op_name = "-";      break;
            case QUAD_MUL:      op_name = "*";      break;
            case QUAD_DIV:      op_name = "/";      break;
            case QUAD_ASSIGN:   op_name = "=";      break;
            case QUAD_EQ:       op_name = "j=";     break;
            case QUAD_NEQ:      op_name = "j#";     break;
            case QUAD_LT:       op_name = "j<";     break;
            case QUAD_LE:       op_name = "j<=";    break;
            case QUAD_GT:       op_name = "j>";     break;
            case QUAD_GE:       op_name = "j>=";    break;
            case QUAD_JMP:      op_name = "j";      break;
            case QUAD_READ:     op_name = "read";   break;
            case QUAD_WRITE:    op_name = "write";  break;
            case QUAD_CALL:     op_name = "call";   break;
            case QUAD_RET:      op_name = "ret";    break;
            default:            op_name = "?";      break;
        }

        printf("(%d)(%s,%s,%s,%s)\n",
               i + 1, op_name, q->arg1, q->arg2, q->result);
    }
}

const char* quad_op_to_str(QuadOp op)
{
    switch (op) {
        case QUAD_SYSS:     return "syss";
        case QUAD_SYSE:     return "syse";
        case QUAD_CONST:    return "const";
        case QUAD_VAR:      return "var";
        case QUAD_PROC:     return "procedure";
        case QUAD_ADD:      return "+";
        case QUAD_SUB:      return "-";
        case QUAD_MUL:      return "*";
        case QUAD_DIV:      return "/";
        case QUAD_ASSIGN:   return "=";
        case QUAD_EQ:       return "j=";
        case QUAD_NEQ:      return "j#";
        case QUAD_LT:       return "j<";
        case QUAD_LE:       return "j<=";
        case QUAD_GT:       return "j>";
        case QUAD_GE:       return "j>=";
        case QUAD_JMP:      return "j";
        case QUAD_READ:     return "read";
        case QUAD_WRITE:    return "write";
        case QUAD_CALL:     return "call";
        case QUAD_RET:      return "ret";
        default:            return "?";
    }
}
