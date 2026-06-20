@echo off
REM build.bat - 编译PL/0编译器 (Windows/MinGW)

REM 设置编译器和标志
set CC=gcc
set CFLAGS=-Wall -Wextra -std=c99 -I.
set LDFLAGS=-lm
set SRC=main.c lexical_analyzer.c syntax_analyzer.c semantic_analyzer.c symbol_table.c four_address_code.c nfa_dfa.c lr_parser.c
set TARGET=pl0_compiler.exe

echo ===== 编译PL/0编译器 =====
echo.
echo 源文件: %SRC%
echo.

%CC% %CFLAGS% %SRC% -o %TARGET% %LDFLAGS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✓ 编译成功！生成: %TARGET%
    echo.
    echo 使用方法:
    echo   %TARGET%                  - 运行内置测试
    echo   %TARGET% -nfa             - 演示NFA-DFA最小化
    echo   %TARGET% source.pl0 -lex  - 词法分析
    echo   %TARGET% source.pl0 -parse - 语法分析
    echo   %TARGET% source.pl0 -sem  - 语义分析(默认)
    echo.
) else (
    echo.
    echo ✗ 编译失败！
)
