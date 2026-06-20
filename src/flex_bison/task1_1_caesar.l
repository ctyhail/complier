/* task1_1_caesar.l
 * Flex 描述文件 - 凯撒密码字符频率统计
 * 功能：读取一行或多行密文，统计大写字母频率
 * 忽略小写字母（转换为大写统计）、空格、标点符号及数字
 */
%{
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int letter_count[26] = {0};  /* A-Z 计数 */
int total_letters = 0;       /* 字母总数 */
%}

%option noyywrap

%%
[A-Z] {
    int idx = yytext[0] - 'A';
    letter_count[idx]++;
    total_letters++;
}

[a-z] {
    /* 小写字母转换为大写进行统计 */
    int idx = toupper(yytext[0]) - 'A';
    letter_count[idx]++;
    total_letters++;
}

.|\n {
    /* 忽略空格、标点、数字、换行等所有其他字符 */
    /* 不做任何处理 */
}
%%

int main(int argc, char *argv[]) {
    int i;
    
    printf("请输入密文（Ctrl+Z 或 Ctrl+D 结束输入）：\n");
    yylex();
    
    printf("\n字符频率统计结果：\n");
    if (total_letters == 0) {
        printf("未检测到任何字母字符。\n");
        return 0;
    }
    
    for (i = 0; i < 26; i++) {
        if (letter_count[i] > 0) {
            double percent = (double)letter_count[i] / total_letters * 100;
            printf("%c:%.0f%%, ", 'A' + i, percent);
        }
    }
    printf("\n");
    
    return 0;
}
