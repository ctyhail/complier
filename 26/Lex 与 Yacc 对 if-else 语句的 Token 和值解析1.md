# Lex 与 Yacc 对 if-else 语句的 Token 和值解析

下面我们详细分析这个 if-else 语句案例在 Lex 和 Yacc 交互中的处理过程：

## 示例代码

```c
if (ix>5){
    a=a+b;
}
else{
    printf("%d\n",a);
}
```

## 1. Yacc 文件中的定义 (parser.y)

首先需要在 Yacc 中定义所有可能的 token 和值类型：

```yacc
%{
#include <stdio.h>
#include <string.h>
%}

/* 定义 yylval 的联合类型 */
%union {
    int num;          // 用于整数常量
    char id[32];      // 用于标识符名称
    char str[100];    // 用于字符串字面量
}

/* Token 定义 */
%token <id> IDENTIFIER
%token <num> NUMBER
%token IF ELSE 
%token LPAREN RPAREN LBRACE RBRACE
%token GT LT GE LE EQ NE  //运算符
%token ASSIGN PLUS SEMICOLON
%token PRINTF STRING

/* 运算符优先级 */
%left GT LT GE LE EQ NE
%left PLUS
%left ASSIGN

%%
/* 语法规则 */
statement: 
    IF LPAREN condition RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
    | /* 其他语句规则 */
;

condition: expression;
statements: statement | statements statement;

expression:
    IDENTIFIER
    | NUMBER
    | expression GT expression
    | expression PLUS expression
    | IDENTIFIER ASSIGN expression
    | PRINTF LPAREN STRING COMMA IDENTIFIER RPAREN
;
%%
```

## 2. Lex 文件中的规则 (lexer.l)

```lex
%{
#include "y.tab.h"
%}

%%

"if"        { return IF; }
"else"      { return ELSE; }
"printf"    { return PRINTF; }
[0-9]+      { yylval.num = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { strcpy(yylval.id, yytext); return IDENTIFIER; }
">"         { return GT; }
"+"         { return PLUS; }
"="         { return ASSIGN; }
"("         { return LPAREN; }
")"         { return RPAREN; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }
";"         { return SEMICOLON; }
","         { return COMMA; }
\"[^"]*\"   { strcpy(yylval.str, yytext); return STRING; }
[ \t\n]     ; /* 忽略空白字符 */
.           { printf("未知字符: %s\n", yytext); }

%%
```

## 3. Token 和值的详细解析过程

### 输入代码分解：

```
if (ix>5){
    a=a+b;
}
else{
    printf("%d\n",a);
}
```

### 词法分析器(Lex)处理过程：

1. `if`

   * Token: `IF` (关键字，无关联值)

   * 动作: `return IF;`

2. `(`

   * Token: `LPAREN` (无关联值)

   * 动作: `return LPAREN;`

3. `ix`

   * Token: `IDENTIFIER`

   * 值: `yylval.id = "ix"`

   * 动作: `strcpy(yylval.id, "ix"); return IDENTIFIER;`

4. `>`

   * Token: `GT` (大于运算符)

   * 动作: `return GT;`

5. `5`

   * Token: `NUMBER`

   * 值: `yylval.num = 5`

   * 动作: `yylval.num = 5; return NUMBER;`

6. `)`

   * Token: `RPAREN`

   * 动作: `return RPAREN;`

7. `{`

   * Token: `LBRACE`

   * 动作: `return LBRACE;`

8. `a`

   * Token: `IDENTIFIER`

   * 值: `yylval.id = "a"`

   * 动作: `strcpy(yylval.id, "a"); return IDENTIFIER;`

9. `=`

   * Token: `ASSIGN`

   * 动作: `return ASSIGN;`

10. `a`

    * Token: `IDENTIFIER`

    * 值: `yylval.id = "a"`

    * 动作: `strcpy(yylval.id, "a"); return IDENTIFIER;`

11. `+`

    * Token: `PLUS`

    * 动作: `return PLUS;`

12. `b`

    * Token: `IDENTIFIER`

    * 值: `yylval.id = "b"`

    * 动作: `strcpy(yylval.id, "b"); return IDENTIFIER;`

13. `;`

    * Token: `SEMICOLON`

    * 动作: `return SEMICOLON;`

14. `}`

    * Token: `RBRACE`

    * 动作: `return RBRACE;`

15. `else`

    * Token: `ELSE`

    * 动作: `return ELSE;`

16. `{`

    * Token: `LBRACE`

    * 动作: `return LBRACE;`

17. `printf`

    * Token: `PRINTF`

    * 动作: `return PRINTF;`

18. `(`

    * Token: `LPAREN`

    * 动作: `return LPAREN;`

19. `"%d\n"`

    * Token: `STRING`

    * 值: `yylval.str = "\"%d\n\""`

    * 动作: `strcpy(yylval.str, "\"%d\n\""); return STRING;`

20. `,`

    * Token: `COMMA`

    * 动作: `return COMMA;`

21. `a`

    * Token: `IDENTIFIER`

    * 值: `yylval.id = "a"`

    * 动作: `strcpy(yylval.id, "a"); return IDENTIFIER;`

22. `)`

    * Token: `RPAREN`

    * 动作: `return RPAREN;`

23. `;`

    * Token: `SEMICOLON`

    * 动作: `return SEMICOLON;`

24. `}`

    * Token: `RBRACE`

    * 动作: `return RBRACE;`

### 语法分析器(Yacc)接收的 token 序列：

```
IF LPAREN IDENTIFIER(ix) GT NUMBER(5) RPAREN LBRACE
IDENTIFIER(a) ASSIGN IDENTIFIER(a) PLUS IDENTIFIER(b) SEMICOLON
RBRACE ELSE LBRACE
PRINTF LPAREN STRING("%d\n") COMMA IDENTIFIER(a) RPAREN SEMICOLON
RBRACE
```

## 4. 语法树构建示例

Yacc 会根据语法规则构建如下的抽象语法树结构：

```
IF_STATEMENT
├── CONDITION (GT_EXPR)
│   ├── IDENTIFIER(ix)
│   └── NUMBER(5)
├── THEN_BLOCK
│   └── ASSIGN_STATEMENT
│       ├── IDENTIFIER(a)
│       └── PLUS_EXPR
│           ├── IDENTIFIER(a)
│           └── IDENTIFIER(b)
└── ELSE_BLOCK
    └── PRINTF_CALL
        ├── STRING("%d\n")
        └── IDENTIFIER(a)
```



## 参考：Lex 与 Yacc 对 if-else 语句的语法树构建完整程序.md

***



## 5. 关键点总结

1. **关键字 token**(if, else, printf)：

   * 只有 token 类型，不携带值

   * 用于确定语法结构

2. **标识符 token**：

   * 既需要 token 类型(IDENTIFIER)

   * 也需要传递标识符名称(通过 yylval.id)

3. **常量 token**：

   * NUMBER 类型 token

   * 传递具体的数值(通过 yylval.num)

4. **字符串字面量**：

   * STRING 类型 token

   * 传递完整的字符串内容(通过 yylval.str)

5. **标点符号和运算符**：

   * 只需要 token 类型

   * 不需要传递额外值

这种分离方式使得语法分析器可以专注于程序结构的正确性，而语义分析阶段则可以处理具体的值和标识符。



