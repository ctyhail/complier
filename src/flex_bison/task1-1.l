/**
 * task1-1.l - Flex描述文件：凯撒密码字符频率统计
 *
 * 功能：读取密文，只统计大写字母，
 *       小写字母转大写统计，忽略空格、标点、数字
 *
 * 编译: flex task1-1.l && gcc lex.yy.c -o freq.exe -lfl
 * 使用: freq.exe < input.txt
 * 输入: KHOOR ZRUOG
 * 输出: K:10%, H:10%, O:30%, R:20%, Z:10%, U:10%, G:10%
 */

%{
#include <stdio.h>
#include <ctype.h>

/* 字母频率统计数组 (A-Z) */
int freq[26] = {0};
int total = 0;

%}

%%

/* 匹配大写字母 [A-Z] */
[A-Z] {
    freq[yytext[0] - 'A']++;
    total++;
}

/* 匹配小写字母 [a-z]，转换为大写统计 */
[a-z] {
    freq[toupper(yytext[0]) - 'A']++;
    total++;
}

/* 忽略所有其他字符（数字、标点、空格、换行等） */
.|\n {
    /* 忽略 */
}

%%

int main(int argc, char *argv[])
{
    printf("===== 凯撒密码字符频率统计 =====\n\n");

    yylex();

    if (total == 0) {
        printf("没有检测到字母字符。\n");
        return 0;
    }

    printf("输入文本按字母频率分析:\n");
    for (int i = 0; i < 26; i++) {
        if (freq[i] > 0) {
            double percent = (double)freq[i] / total * 100;
            printf("%c:%.0f%%, ", 'A' + i, percent);
        }
    }
    printf("\n\n总计 %d 个字母\n", total);

    return 0;
}

int yywrap(void)
{
    return 1;
}
