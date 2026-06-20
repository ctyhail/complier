/**
 * syntax_analyzer.c - PL/0递归下降语法分析器
 *
 * 基于PL/0文法实现递归下降语法分析：
 * <程序> ::= <分程序>.
 * <分程序> ::= [<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句>
 * <常量说明部分> ::= const <常量定义>{,<常量定义>};
 * ...
 * 集成语义动作，生成四元式中间代码
 */

#include "syntax_analyzer.h"
#include "lexical_analyzer.h"
#include "symbol_table.h"
#include "four_address_code.h"
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

/* 全局状态 */
static TokenQueue *tok_q;
static Token current;
static SymTable symtab;
static QuadList quads;
static int has_syntax_error;
static jmp_buf syntax_error_jmp;

/* 前向声明 */
static void parse_program(void);
static void parse_block(void);
static void parse_const_decl(void);
static void parse_var_decl(void);
static void parse_proc_decl(void);
static void parse_statement(void);
static void parse_condition(void);
static void parse_expression(void);
static void parse_term(void);
static void parse_factor(void);

/* 递归下降函数声明 */
static void RD_program(void);
static void RD_block(void);
static void RD_const_declaration(void);
static void RD_var_declaration(void);
static void RD_proc_declaration(void);
static void RD_statement(void);
static void RD_condition(void);
static void RD_expression(void);
static void RD_term(void);
static void RD_factor(void);

/* ========== Token操作 ========== */
static void next_token(void)
{
    current = token_queue_next(tok_q);
}

static int match(TokenType type)
{
    if (current.type == type) {
        next_token();
        return 1;
    }
    return 0;
}

static int expect(TokenType type)
{
    if (current.type == type) {
        next_token();
        return 1;
    }
    report_error(current.line, "Syntax error: expected %s, got %s",
                 token_type_str[type], token_type_str[current.type]);
    has_syntax_error = 1;
    return 0;
}

/* 同步到语句开始 */
static void sync_to_statement(void)
{
    while (current.type != TOK_EOF) {
        if (current.type == TOK_IDENT || current.type == TOK_BEGIN ||
            current.type == TOK_IF || current.type == TOK_WHILE ||
            current.type == TOK_CALL || current.type == TOK_READ ||
            current.type == TOK_WRITE || current.type == TOK_END ||
            current.type == TOK_SEMICOLON || current.type == TOK_PERIOD) {
            return;
        }
        next_token();
    }
}

/* ========== 语义动作相关 ========== */
static SymTable *get_symtab(void) { return &symtab; }
static QuadList *get_quads(void) { return &quads; }

/* 表达式求值追踪 */
static char last_value[16];   /* 最后一个表达式的值（标识符名/数字/临时变量） */
static char expr_target[16];  /* 赋值目标变量名（为空表示无需存储到变量） */

/* ========== 初始化语法分析 ========== */
void syntax_init(TokenQueue *q)
{
    tok_q = q;
    token_queue_reset(tok_q);
    next_token();
    has_syntax_error = 0;
    symtab_init(&symtab);
    quad_init(&quads);
    quad_reset_temp();
    reset_error_count();
}

/* ========== 递归下降语法分析 ========== */

/* <程序> ::= <分程序>. */
static void RD_program(void)
{
    quad_emit(get_quads(), QUAD_SYSS, "_", "_", "_");

    RD_block();

    if (current.type == TOK_PERIOD) {
        next_token();
    } else if (current.type != TOK_EOF) {
        report_error(current.line, "Syntax error: expected '.' at end");
        has_syntax_error = 1;
    }

    quad_emit(get_quads(), QUAD_SYSE, "_", "_", "_");
}

/* <分程序> ::= [<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句> */
static void RD_block(void)
{
    symtab_enter_scope(get_symtab());

    /* 允许多个 const/var/procedure 声明 */
    while (1) {
        if (current.type == TOK_CONST) {
            RD_const_declaration();
        } else if (current.type == TOK_VAR) {
            RD_var_declaration();
        } else if (current.type == TOK_PROCEDURE) {
            RD_proc_declaration();
        } else {
            break;
        }
    }

    RD_statement();

    symtab_exit_scope(get_symtab());
}

/* <常量说明部分> ::= const <常量定义>{,<常量定义>}; */
static void RD_const_declaration(void)
{
    expect(TOK_CONST);

    do {
        if (current.type != TOK_IDENT) {
            report_error(current.line, "Syntax error: expected identifier");
            has_syntax_error = 1;
            sync_to_statement();
            return;
        }

        char name[MAX_IDENT_LEN + 1];
        strncpy(name, current.lexeme, MAX_IDENT_LEN);
        next_token();

        if (!expect(TOK_EQ)) {
            sync_to_statement();
            return;
        }

        if (current.type != TOK_NUMBER) {
            report_error(current.line, "Syntax error: expected number");
            has_syntax_error = 1;
            sync_to_statement();
            return;
        }

        int val = current.value;
        next_token();

        /* 语义动作：添加常量到符号表 */
        if (symtab_add(get_symtab(), name, SYM_CONST, val) < 0) {
            report_error(current.line, "Duplicate: %s", name);
        } else {
            quad_emit(get_quads(), QUAD_CONST, name, "_", "_");
            char val_str[16];
            snprintf(val_str, sizeof(val_str), "%d", val);
            quad_emit(get_quads(), QUAD_ASSIGN, val_str, "_", name);
        }

    } while (current.type == TOK_COMMA && (next_token(), 1));

    expect(TOK_SEMICOLON);
}

/* <变量说明部分> ::= var <标识符>{,<标识符>}; */
/* 也允许 var 单独出现（空变量声明） */
static void RD_var_declaration(void)
{
    expect(TOK_VAR);

    /* 允许空变量声明：var 后没有标识符 */
    if (current.type != TOK_IDENT) {
        return;
    }

    do {
        if (current.type != TOK_IDENT) {
            report_error(current.line, "Syntax error: expected identifier");
            has_syntax_error = 1;
            sync_to_statement();
            return;
        }

        char name[MAX_IDENT_LEN + 1];
        strncpy(name, current.lexeme, MAX_IDENT_LEN);
        next_token();

        /* 语义动作：添加变量到符号表 */
        if (symtab_add(get_symtab(), name, SYM_VAR, 0) < 0) {
            report_error(current.line, "Duplicate: %s", name);
        } else {
            quad_emit(get_quads(), QUAD_VAR, name, "_", "_");
        }

    } while (current.type == TOK_COMMA && (next_token(), 1));

    expect(TOK_SEMICOLON);
}

/* <过程说明部分> ::= <过程首部><分程序> {;<过程说明部分>} */
static void RD_proc_declaration(void)
{
    while (1) {
        /* <过程首部> ::= procedure <标识符>; */
        if (current.type != TOK_PROCEDURE) break;
        next_token();  /* consume procedure */

        if (current.type == TOK_IDENT) {
            char name[MAX_IDENT_LEN + 1];
            strncpy(name, current.lexeme, MAX_IDENT_LEN);
            next_token();

            if (symtab_add(get_symtab(), name, SYM_PROCEDURE, 0) < 0) {
                report_error(current.line, "Duplicate procedure: %s", name);
            } else {
                quad_emit(get_quads(), QUAD_PROC, name, "_", "_");
            }

            symtab_set_proc(get_symtab(), name);
            expect(TOK_SEMICOLON);

            /* 分程序 */
            RD_block();

            /* 过程返回 */
            quad_emit(get_quads(), QUAD_RET, "_", "_", "_");

            /* 过程后可能有 ; 分隔下一个过程 */
            if (current.type == TOK_SEMICOLON) {
                next_token();
            }
        } else {
            report_error(current.line, "Syntax error: expected procedure name");
            has_syntax_error = 1;
            sync_to_statement();
            return;
        }
    }
}

/* <语句> ::= <赋值语句>|<条件语句>|<当型循环语句>|<过程调用语句>|<读语句>|<写语句>|<复合语句>|<空语句> */
static void RD_statement(void)
{
    switch (current.type) {
        case TOK_IDENT: {
            /* <赋值语句> ::= <标识符>:=<表达式> */
            char name[MAX_IDENT_LEN + 1];
            strncpy(name, current.lexeme, MAX_IDENT_LEN);
            name[MAX_IDENT_LEN] = '\0';
            int line = current.line;
            next_token();

            if (current.type == TOK_ASSIGN) {
                next_token();

                /* 语义检查：变量是否声明 */
                if (symtab_lookup(get_symtab(), name) < 0) {
                    report_semantic_error(line, "Undeclared ident: %s", name);
                } else if (symtab_lookup(get_symtab(), name) >= 0) {
                    SymKind k = get_symtab()->entries[symtab_lookup(get_symtab(), name)].kind;
                    if (k != SYM_VAR) {
                        report_semantic_error(line, "'%s' is not a variable", name);
                    }
                }

                /* 设置赋值目标，使表达式结果直接存入目标变量 */
                strncpy(expr_target, name, 15);
                expr_target[15] = '\0';
                last_value[0] = '\0';

                RD_expression();

                /* 如果表达式结果未直接存入目标变量，生成赋值四元式 */
                if (strcmp(last_value, name) != 0 && last_value[0] != '\0') {
                    quad_emit(get_quads(), QUAD_ASSIGN, last_value, "_", name);
                }
                expr_target[0] = '\0';
            } else {
                report_error(current.line, "Syntax error: expected ':='");
                has_syntax_error = 1;
            }
            break;
        }

        case TOK_IF: {
            /* <条件语句> ::= if <条件> then <语句> */
            next_token();
            RD_condition();

            int jump_quad = quad_emit(get_quads(), QUAD_JMP, "_", "_", "?");

            if (current.type == TOK_THEN) {
                next_token();
            } else {
                report_error(current.line, "Syntax error: expected 'then'");
                has_syntax_error = 1;
            }

            RD_statement();

            quad_patch(get_quads(), jump_quad, quad_next_idx(get_quads()));
            break;
        }

        case TOK_WHILE: {
            /* <当型循环语句> ::= while <条件> do <语句> */
            next_token();

            int loop_start = quad_next_idx(get_quads());
            RD_condition();

            int jump_out = quad_emit(get_quads(), QUAD_JMP, "_", "_", "?");

            if (current.type == TOK_DO) {
                next_token();
            } else {
                report_error(current.line, "Syntax error: expected 'do'");
                has_syntax_error = 1;
            }

            RD_statement();

            quad_emit(get_quads(), QUAD_JMP, "_", "_", NULL);
            /* 修补跳转目标 */
            /* 实际应修补到loop_start，这里简化处理 */

            quad_patch(get_quads(), jump_out, quad_next_idx(get_quads()));
            break;
        }

        case TOK_CALL: {
            /* <过程调用语句> ::= call <标识符> */
            next_token();
            if (current.type == TOK_IDENT) {
                char name[MAX_IDENT_LEN + 1];
                strncpy(name, current.lexeme, MAX_IDENT_LEN);
                int line = current.line;
                next_token();

                if (symtab_lookup(get_symtab(), name) < 0) {
                    report_semantic_error(line, "Undeclared procedure: %s", name);
                }

                quad_emit(get_quads(), QUAD_CALL, name, "_", "_");
            } else {
                report_error(current.line, "Syntax error: expected procedure name");
                has_syntax_error = 1;
            }
            break;
        }

        case TOK_READ: {
            /* <读语句> ::= read (<标识符>{,<标识符>}); */
            next_token();
            expect(TOK_LPAREN);

            do {
                if (current.type == TOK_IDENT) {
                    char name[MAX_IDENT_LEN + 1];
                    strncpy(name, current.lexeme, MAX_IDENT_LEN);
                    int line = current.line;
                    next_token();

                    quad_emit(get_quads(), QUAD_READ, name, "_", "_");
                } else {
                    report_error(current.line, "Syntax error: expected identifier");
                    has_syntax_error = 1;
                    break;
                }
            } while (current.type == TOK_COMMA && (next_token(), 1));

            expect(TOK_RPAREN);
            expect(TOK_SEMICOLON);
            break;
        }

        case TOK_WRITE: {
            /* <写语句> ::= write (<表达式>{,<表达式>}); */
            next_token();
            expect(TOK_LPAREN);

            do {
                /* 临时关闭 expr_target */
                char saved_target[16];
                strncpy(saved_target, expr_target, 15);
                saved_target[15] = '\0';
                expr_target[0] = '\0';

                RD_expression();
                quad_emit(get_quads(), QUAD_WRITE, last_value, "_", "_");

                strncpy(expr_target, saved_target, 15);
                expr_target[15] = '\0';
            } while (current.type == TOK_COMMA && (next_token(), 1));

            expect(TOK_RPAREN);
            expect(TOK_SEMICOLON);
            break;
        }

        case TOK_BEGIN: {
            /* <复合语句> ::= begin<语句>{;<语句>}end */
            next_token();
            RD_statement();

            while (1) {
                if (current.type == TOK_END) break;
                if (current.type == TOK_SEMICOLON) {
                    next_token();
                    if (current.type == TOK_END) break;
                    RD_statement();
                } else if (current.type == TOK_IDENT ||
                           current.type == TOK_IF ||
                           current.type == TOK_WHILE ||
                           current.type == TOK_CALL ||
                           current.type == TOK_READ ||
                           current.type == TOK_WRITE ||
                           current.type == TOK_BEGIN) {
                    /* 语句后无分号，但下一个是新的语句 */
                    RD_statement();
                } else {
                    break;
                }
            }

            if (current.type == TOK_END) {
                next_token();
            } else {
                report_error(current.line, "Syntax error: expected 'end'");
                has_syntax_error = 1;
            }
            break;
        }

        /* <空语句> ::= ε — 什么都不做 */
        default:
            break;
    }
}

/* <条件> ::= <表达式><关系运算符><表达式>|odd<表达式> */
static void RD_condition(void)
{
    if (current.type == TOK_ODD) {
        next_token();
        /* 临时关闭 expr_target */
        char saved_target[16];
        strncpy(saved_target, expr_target, 15);
        saved_target[15] = '\0';
        expr_target[0] = '\0';

        RD_expression();

        /* odd 条件的四元式 */
        strncpy(expr_target, saved_target, 15);
    } else {
        /* 临时关闭 expr_target，条件表达式结果不存到变量 */
        char saved_target[16];
        strncpy(saved_target, expr_target, 15);
        saved_target[15] = '\0';
        expr_target[0] = '\0';

        RD_expression();
        char left_val[16];
        strncpy(left_val, last_value, 15);
        left_val[15] = '\0';

        TokenType rel_op = current.type;
        if (rel_op >= TOK_EQ && rel_op <= TOK_GE) {
            next_token();
        } else {
            report_error(current.line, "Syntax error: expected relational op");
            has_syntax_error = 1;
            expr_target[0] = '\0';
            return;
        }

        RD_expression();

        /* 生成条件跳转四元式 */
        QuadOp qop;
        switch (rel_op) {
            case TOK_EQ: qop = QUAD_EQ; break;
            case TOK_NEQ: qop = QUAD_NEQ; break;
            case TOK_LT: qop = QUAD_LT; break;
            case TOK_LE: qop = QUAD_LE; break;
            case TOK_GT: qop = QUAD_GT; break;
            case TOK_GE: qop = QUAD_GE; break;
            default: qop = QUAD_JMP;
        }
        quad_emit(get_quads(), qop, left_val, last_value, "?");

        strncpy(expr_target, saved_target, 15);
        expr_target[15] = '\0';
    }
}

/* <表达式> ::= [+|-]<项>{<加减运算符><项>} */
static void RD_expression(void)
{
    int unary = 0;
    if (current.type == TOK_PLUS || current.type == TOK_MINUS) {
        if (current.type == TOK_MINUS) unary = 1;
        next_token();
    }

    RD_term();

    if (unary) {
        /* 生成取负: 0 - term */
        char r_buf[16];
        char *r = quad_new_temp(r_buf, sizeof(r_buf));
        quad_emit(get_quads(), QUAD_SUB, "0", last_value, r);
        strncpy(last_value, r, 15);
        last_value[15] = '\0';
    }

    while (current.type == TOK_PLUS || current.type == TOK_MINUS) {
        TokenType op = current.type;
        next_token();

        char left_val[16];
        strncpy(left_val, last_value, 15);
        left_val[15] = '\0';

        RD_term();

        /* 决定结果存储位置 */
        char r_buf[16];
        int is_last = (current.type != TOK_PLUS && current.type != TOK_MINUS);
        const char *result;
        if (is_last && expr_target[0] != '\0') {
            result = expr_target;  /* 直接存入目标变量 */
        } else {
            result = quad_new_temp(r_buf, sizeof(r_buf));
        }

        if (op == TOK_PLUS) {
            quad_emit(get_quads(), QUAD_ADD, left_val, last_value, result);
        } else {
            quad_emit(get_quads(), QUAD_SUB, left_val, last_value, result);
        }
        strncpy(last_value, result, 15);
        last_value[15] = '\0';
    }
}

/* <项> ::= <因子>{<乘除运算符><因子>} */
static void RD_term(void)
{
    RD_factor();

    while (current.type == TOK_MUL || current.type == TOK_DIV) {
        TokenType op = current.type;
        next_token();

        char left_val[16];
        strncpy(left_val, last_value, 15);
        left_val[15] = '\0';

        RD_factor();

        char r_buf[16];
        char *r = quad_new_temp(r_buf, sizeof(r_buf));
        if (op == TOK_MUL) {
            quad_emit(get_quads(), QUAD_MUL, left_val, last_value, r);
        } else {
            quad_emit(get_quads(), QUAD_DIV, left_val, last_value, r);
        }
        strncpy(last_value, r, 15);
        last_value[15] = '\0';
    }
}

/* <因子> ::= <标识符>|<无符号整数>|'('<表达式>')' */
static void RD_factor(void)
{
    if (current.type == TOK_IDENT) {
        char name[MAX_IDENT_LEN + 1];
        strncpy(name, current.lexeme, MAX_IDENT_LEN);
        name[MAX_IDENT_LEN] = '\0';
        int line = current.line;
        next_token();

        /* 语义检查 */
        int idx = symtab_lookup(get_symtab(), name);
        if (idx < 0) {
            report_semantic_error(line, "Undeclared ident: %s", name);
        }

        /* 记录因子值 */
        strncpy(last_value, name, 15);
        last_value[15] = '\0';
    } else if (current.type == TOK_NUMBER) {
        snprintf(last_value, sizeof(last_value), "%d", current.value);
        next_token();
    } else if (current.type == TOK_LPAREN) {
        next_token();
        RD_expression();
        if (current.type == TOK_RPAREN) {
            next_token();
        } else {
            report_error(current.line, "Syntax error: expected ')'");
            has_syntax_error = 1;
        }
    } else {
        report_error(current.line, "Syntax error: expected identifier, number or '('");
        has_syntax_error = 1;
        strcpy(last_value, "?");
    }
}

/* ========== 主分析入口 ========== */
int syntax_parse(void)
{
    token_queue_reset(tok_q);
    next_token();
    has_syntax_error = 0;

    RD_program();

    return !has_syntax_error && get_error_count() == 0;
}

int syntax_has_error(void)
{
    return has_syntax_error;
}

SymTable* syntax_get_symtab(void)
{
    return &symtab;
}

QuadList* syntax_get_quads(void)
{
    return &quads;
}
