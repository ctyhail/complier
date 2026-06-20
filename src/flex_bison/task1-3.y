/**
 * task1-3.y - Bison语法文件：简单计算器
 *
 * 实现加法和乘法的计算器，正确处理优先级
 * 乘法优先级高于加法
 *
 * 编译: bison -d task1-3.y
 * 然后联合编译: gcc lex.yy.c task1-3.tab.c -o calc.exe -lfl
 */

%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char *s);

%}

%token NUMBER
%token PLUS MUL LPAREN RPAREN
%token EOL

%left PLUS
%left MUL

%%

input:
    /* 空 */
    | input expr EOL {
        printf("结果: %d\n", $2);
    }
    | input EOL
    | input error EOL {
        yyerror("语法错误，请重新输入");
        yyerrok;
    }
    ;

expr:
    NUMBER              { $$ = $1; }
    | expr PLUS expr    { $$ = $1 + $3; }
    | expr MUL expr     { $$ = $1 * $3; }
    | LPAREN expr RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "错误: %s\n", s);
}

int main(void)
{
    printf("===== 简单计算器(加法和乘法) =====\n");
    printf("请输入表达式 (如 5*7+2)，回车计算结果\n");
    printf("Ctrl+Z 退出\n\n");

    return yyparse();
}
