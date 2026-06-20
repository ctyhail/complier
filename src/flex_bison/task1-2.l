/**
 * task1-2.l - Flex描述文件：识别单词、数字和符号
 *
 * 功能：识别输入文本中的单词、数字和符号，
 *       并分类输出
 *
 * 编译: flex task1-2.l && gcc lex.yy.c -o tokenize.exe -lfl
 * 使用: tokenize.exe < input.txt
 */

%{
#include <stdio.h>
#include <string.h>

%}

/* 定义正则规则 */
LETTER  [a-zA-Z]
DIGIT   [0-9]
WORD    {LETTER}({LETTER}|{DIGIT})*
NUMBER  {DIGIT}+
SYMBOL  [;:,.(){}<>\[\]!@#$%^&*?+=|/~`"'\-]

%%

%{
    printf("===== 单词、数字和符号识别 =====\n\n");
%}

{WORD}      { printf("%s 单词\n", yytext); }

{NUMBER}    { printf("%s 数字\n", yytext); }

{SYMBOL}    { printf("%s 符号\n", yytext); }

[ \t\n\r]+  { /* 忽略空白 */ }

.           { printf("%s 其他\n", yytext); }

%%

int main(int argc, char *argv[])
{
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "无法打开文件: %s\n", argv[1]);
            return 1;
        }
    }

    yylex();
    return 0;
}

int yywrap(void)
{
    return 1;
}
