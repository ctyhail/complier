/* task1_3_calc.y
 * Bison 描述文件 - 简单计算器（加法和乘法）
 * 功能：解析包含加法和乘法的算术表达式，并计算结果
 * 用法：
 *   bison -d task1_3_calc.y
 *   flex task1_3_calc.l
 *   gcc task1_3_calc.tab.c lex.yy.c -o calculator
 *   ./calculator
 */
%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char *s);
extern int yylex();
%}

%token NUMBER
%token ADD MUL
%token LPAREN RPAREN

%left ADD
%left MUL

%%

input:
    /* 空 */
    | input line
    ;

line:
    expr '\n'   { printf("结果: %d\n", $1); }
    | '\n'
    | error '\n' { yyerror("表达式错误，请重新输入"); yyerrok; }
    ;

expr:
    expr ADD term   { $$ = $1 + $3; }
    | term          { $$ = $1; }
    ;

term:
    term MUL factor { $$ = $1 * $3; }
    | factor        { $$ = $1; }
    ;

factor:
    NUMBER          { $$ = $1; }
    | LPAREN expr RPAREN  { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "错误: %s\n", s);
}

int main() {
    printf("简单计算器（支持加法+、乘法*、括号，输入表达式后回车计算）\n");
    printf("输入表达式: ");
    yyparse();
    return 0;
}
