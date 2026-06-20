# PL/0 编译器 — 编译原理课程设计

**桂林电子科技大学 · 计算机与信息安全学院 · 计算机科学与技术**

## 项目简介

本项目实现了一个完整的 PL/0 语言编译器前端，包含：
- **词法分析器** — 基于手工实现 DFA，支持注释跳过和错误恢复
- **语法分析器** — 递归下降法 + LR 分析法双实现
- **语义分析器** — L-翻译模式，符号表管理，四元式中间代码生成
- **正规式→NFA→DFA→最小化** 完整演示
- **Flex/Bison** 使用示例（3个任务）

## 项目结构

```
src/
├── common.h               # 公共头文件（Token类型、数据结构）
├── main.c                 # 主程序入口
├── lexical_analyzer.c/h   # 词法分析器
├── syntax_analyzer.c/h    # 递归下降语法分析器（集成语义动作）
├── lr_parser.c/h          # LR(0)/SLR(1)语法分析器
├── semantic_analyzer.c/h  # 语义分析器
├── symbol_table.c/h       # 符号表管理
├── four_address_code.c/h  # 四元式中间代码
├── nfa_dfa.c/h            # 正规式→NFA→DFA→最小化演示
├── flex_bison/
│   ├── task1-1.l          # 凯撒密码字符频率统计（Flex）
│   ├── task1-2.l          # 单词/数字/符号识别（Flex）
│   ├── task1-3.l          # 计算器词法（Flex）
│   └── task1-3.y          # 计算器语法（Bison）
├── test_cases/
│   ├── test_correct.pl0   # 正确PL/0程序测试用例
│   └── test_error.pl0     # 含错误PL/0程序测试用例
├── Makefile               # Linux/Mac 编译
├── build.bat              # Windows/MinGW 编译
└── pl0_compiler.exe       # 编译好的可执行文件
```

## 编译方法

### Windows (MinGW GCC)
```
cd src
gcc -I. main.c lexical_analyzer.c syntax_analyzer.c semantic_analyzer.c ^
    symbol_table.c four_address_code.c nfa_dfa.c lr_parser.c -o pl0_compiler.exe
```
或双击 `build.bat`

### Linux/Mac (GCC/Clang)
```
cd src
make
```

## 使用方法

```
pl0_compiler.exe [源文件] [选项]
```

**选项：**
| 参数 | 功能 |
|------|------|
| (无参数) | 运行内置测试用例 + NFA→DFA演示 |
| `-lex` | 仅执行词法分析 |
| `-parse` | 执行词法+语法分析 |
| `-sem` | 执行词法+语法+语义分析（默认） |
| `-lr` | 执行LR语法分析 |
| `-nfa` | 演示NFA→DFA→最小化 |
| `-test1` | 运行内建测试用例1（正确程序） |
| `-test2` | 运行内建测试用例2（含错误程序） |

**示例：**
```bash
pl0_compiler.exe test_cases/test_correct.pl0 -sem
pl0_compiler.exe test_cases/test_error.pl0 -lex
pl0_compiler.exe -nfa
```

## PL/0语言文法

```
<程序>            ::= <分程序>.
<分程序>          ::= [<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句>
<常量说明部分>    ::= const <常量定义>{,<常量定义>};
<常量定义>        ::= <标识符>=<无符号整数>
<变量说明部分>    ::= var <标识符>{,<标识符>};
<过程说明部分>    ::= <过程首部><分程序>{;<过程说明部分>};
<过程首部>        ::= procedure <标识符>;
<语句>            ::= <赋值语句>|<条件语句>|<当型循环语句>
                      |<过程调用语句>|<读语句>|<写语句>|<复合语句>|<空语句>
<赋值语句>        ::= <标识符>:=<表达式>
<条件语句>        ::= if <条件> then <语句>
<当型循环语句>    ::= while <条件> do <语句>
<过程调用语句>    ::= call <标识符>
<复合语句>        ::= begin<语句>{;<语句>}end
<读语句>          ::= read (<标识符>{,<标识符>});
<写语句>          ::= write (<表达式>{,<表达式>});
<条件>            ::= <表达式><关系运算符><表达式>|odd<表达式>
<表达式>          ::= [+|-]<项>{<加减运算符><项>}
<项>              ::= <因子>{<乘除运算符><因子>}
<因子>            ::= <标识符>|<无符号整数>|'('<表达式>')'
<关系运算符>      ::= =|#|<|<=|>|>=
<加减运算符>      ::= +|-
<乘除运算符>      ::= *|/
```

## Flex/Bison 任务编译

### 任务1-1: 凯撒密码频率统计
```bash
cd flex_bison
flex task1-1.l
gcc lex.yy.c -o freq.exe -lfl
```

### 任务1-2: 单词/数字/符号识别
```bash
cd flex_bison
flex task1-2.l
gcc lex.yy.c -o tokenize.exe -lfl
```

### 任务1-3: 加减乘计算器
```bash
cd flex_bison
flex task1-3.l
bison -d task1-3.y
gcc lex.yy.c task1-3.tab.c -o calc.exe -lfl
```
