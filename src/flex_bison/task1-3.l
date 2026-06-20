/**
 * task1-3.l - Flex词法文件：简单计算器
 *
 * 与 task1-3.y 联合使用，实现加法和乘法计算器
 *
 * 编译: flex task1-3.l && bison -d task1-3.y
 *       然后 gcc lex.yy.c task1-3.tab.c -o calc.exe -lfl
 * 使用: calc.exe
 *       输入: 5*7+2
 *       输出: 37
 */

%{
#include "task1-3.tab.h"
#include <stdio.h>

%}

%%

[0-9]+    { yylval = atoi(yytext); return NUMBER; }

"+"       { return PLUS; }
"*"       { return MUL; }
"("       { return LPAREN; }
")"       { return RPAREN; }

[ \t\n]   { /* 忽略空白 */ }

.         { fprintf(stderr, "忽略非法字符: %c\n", yytext[0]); }

%%

int yywrap(void)
{
    return 1;
}
