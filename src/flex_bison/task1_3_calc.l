/* task1_3_calc.l
 * Flex 描述文件 - 简单计算器（加法和乘法）的词法分析器
 * 与 task1_3_calc.y 配合使用
 */
%{
#include "task1_3_calc.tab.h"
%}

%option noyywrap

%%
[0-9]+      { yylval = atoi(yytext); return NUMBER; }
"+"         { return ADD; }
"*"         { return MUL; }
"("         { return LPAREN; }
")"         { return RPAREN; }
[ \t\n\r]   { /* 空白字符不做处理 */ }
.           { printf("未知字符: %s\n", yytext); return 0; }
%%
