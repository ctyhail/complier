/* task1_2_lex.l
 * Flex 描述文件 - 识别单词、数字和符号
 * 功能：读取源程序文本，识别并分类输出单词、数字和符号
 * 空格不做处理
 */
%{
#include <stdio.h>
#include <string.h>
%}

%option noyywrap

/* 定义正则表达式模式 */
LETTER          [a-zA-Z]
DIGIT           [0-9]
WORD            {LETTER}({LETTER}|{DIGIT})*
NUMBER          {DIGIT}+
SYMBOL          [;(){}<>=",+*/%!&#|~^\[\]\\:?.@_`]

%%
{WORD}      { printf("%s 单词\n", yytext); }
{NUMBER}    { printf("%s 数字\n", yytext); }
{SYMBOL}    { printf("%s 符号\n", yytext); }
[ \t\n\r]   { /* 空白字符不做处理 */ }
.           { printf("%s 符号\n", yytext); }
%%

int main(int argc, char *argv[]) {
    printf("请输入源程序文本（Ctrl+Z 或 Ctrl+D 结束输入）：\n");
    yylex();
    return 0;
}
