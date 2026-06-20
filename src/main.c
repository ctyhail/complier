/**
 * main.c - PL/0 Compiler Main Program
 *
 * Compiler Design Course Project
 * PL/0 language: lexical, syntax, semantic analysis + intermediate code
 *
 * Compile (MinGW GCC):
 *   gcc -o pl0_compiler.exe main.c lexical_analyzer.c syntax_analyzer.c
 *       semantic_analyzer.c symbol_table.c four_address_code.c
 *       nfa_dfa.c lr_parser.c -I.
 *
 * Usage:
 *   pl0_compiler.exe <source.pl0> [option]
 *   Options:
 *     -lex     Lexical analysis only
 *     -parse   Lexical + syntax analysis
 *     -sem     Lexical + syntax + semantic (default)
 *     -lr      LR parsing
 *     -nfa     NFA->DFA->minimization demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "lexical_analyzer.h"
#include "syntax_analyzer.h"
#include "semantic_analyzer.h"
#include "lr_parser.h"
#include "nfa_dfa.h"
#include "symbol_table.h"
#include "four_address_code.h"

#define VERSION "1.0.0"

void print_banner(void)
{
    printf("+------------------------------------------+\n");
    printf("|  PL/0 Compiler -- Compiler Design Project |\n");
    printf("|  Guilin Univ. of Electronic Technology    |\n");
    printf("|  Version %s                               |\n", VERSION);
    printf("+------------------------------------------+\n\n");
}

void print_usage(void)
{
    printf("Usage: pl0_compiler.exe <source_file> [option]\n");
    printf("Options:\n");
    printf("  -lex     Lexical analysis only\n");
    printf("  -parse   Lexical + syntax analysis\n");
    printf("  -sem     Lexical + syntax + semantic (default)\n");
    printf("  -lr      LR parsing demo\n");
    printf("  -nfa     NFA->DFA->minimization demo\n");
    printf("  -h, -?   Help\n");
}

/* ========== Lexical analysis mode ========== */
int run_lexer(const char *filename)
{
    printf("===== LEXICAL ANALYSIS =====\n\n");
    printf("Source: %s\n\n", filename);

    lex_init_file(filename);

    printf("Results:\n");
    printf("--------------------------------------\n");

    int token_count = 0;
    Token t;
    do {
        t = lex_get_next_token();
        if (t.type != TOK_EOF && t.type != TOK_ERROR) {
            lex_print_token(&t);
            token_count++;
        } else if (t.type == TOK_ERROR) {
            lex_print_token(&t);
        }
    } while (t.type != TOK_EOF);

    printf("--------------------------------------\n");
    printf("Total: %d tokens, %d errors\n\n", token_count, get_error_count());

    lex_close();
    return get_error_count() == 0;
}

/* ========== Syntax analysis mode ========== */
int run_parser(const char *filename)
{
    printf("===== SYNTAX ANALYSIS =====\n\n");

    lex_init_file(filename);
    TokenQueue tokens = lex_analyze_all();
    lex_close();

    printf("Lexical analysis done: %d tokens\n\n", tokens.count);

    syntax_init(&tokens);
    reset_error_count();

    printf("Starting recursive descent parsing...\n\n");

    int result = syntax_parse();

    if (result) {
        printf("\n语法正确\n");
    } else {
        printf("\n语法错误\n");
    }

    return result;
}

/* ========== Semantic analysis mode ========== */
int run_semantic(const char *filename)
{
    printf("===== SEMANTIC ANALYSIS (Lex+Syn+Sem) =====\n\n");

    lex_init_file(filename);
    TokenQueue tokens = lex_analyze_all();
    lex_close();

    printf("Lexical analysis done: %d tokens\n\n", tokens.count);

    /* Print lexical results */
    printf("Lexical output:\n");
    token_queue_reset(&tokens);
    int tc = 0;
    Token t;
    do {
        t = token_queue_next(&tokens);
        if (t.type != TOK_EOF) {
            lex_print_token(&t);
            tc++;
        }
    } while (t.type != TOK_EOF);
    printf("Total %d tokens\n\n", tc);

    /* Semantic analysis */
    token_queue_reset(&tokens);
    reset_error_count();

    int sem_ok = semantic_analyze(&tokens);
    if (!sem_ok) {
        printf("\n语法错误\n");
    }

    return get_error_count() == 0;
}

/* ========== LR analysis mode ========== */
int run_lr_parser(const char *filename)
{
    printf("===== LR PARSING =====\n\n");

    lex_init_file(filename);
    TokenQueue tokens = lex_analyze_all();
    lex_close();

    printf("Lexical analysis done: %d tokens\n\n", tokens.count);

    lr_parse(&tokens);

    return 1;
}

/* ========== Built-in test cases ========== */

const char *test_correct = 
    "const a = 10;\n"
    "var b, c;\n"
    "procedure fun1;\n"
    "if a <= 10 then\n"
    "begin\n"
    "    c := b + a;\n"
    "end;\n"
    "begin\n"
    "    read(b);\n"
    "    while b # 0 do\n"
    "    begin\n"
    "        call fun1;\n"
    "        write(2 * c);\n"
    "        read(b);\n"
    "    end\n"
    "end.\n";

const char *test_error =
    "const 2a = 123456789;\n"
    "var b, c;\n"
    "//single line comment\n"
    "/*\n"
    "multi line comment\n"
    "*/\n"
    "procedure function1;\n"
    "if 2a <= 10 then\n"
    "begin\n"
    "    c := b + a;\n"
    "end;\n"
    "begin\n"
    "    read(b);\n"
    "    while b @ 0 do\n"
    "    begin\n"
    "        call function1;\n"
    "        write(2 * c);\n"
    "        read(b);\n"
    "    end\n"
    "end.\n";

void run_builtin_test(int test_id)
{
    const char *source;
    if (test_id == 1) {
        source = test_correct;
        printf("Test case 1: Correct PL/0 program\n\n");
        printf("%s\n", source);
    } else {
        source = test_error;
        printf("Test case 2: PL/0 program with errors\n\n");
        printf("%s\n", source);
    }

    printf("--------------------------------------\n");

    lex_init_string(source);
    TokenQueue tokens = lex_analyze_all();
    lex_close();

    printf("Lexical output:\n");
    token_queue_reset(&tokens);
    Token t;
    while (1) {
        t = token_queue_next(&tokens);
        if (t.type == TOK_EOF) break;
        lex_print_token(&t);
    }

    printf("\nSyntax + Semantic analysis:\n");
    token_queue_reset(&tokens);
    reset_error_count();

    int sem_ok = semantic_analyze(&tokens);
    if (!sem_ok) {
        printf("\n--- 发现错误 ---\n");
    }
    printf("--------------------------------------\n\n");
}

/* ========== Main ========== */
int main(int argc, char *argv[])
{
    print_banner();

    if (argc < 2) {
        printf("No source file specified. Running built-in tests...\n\n");
        run_builtin_test(1);
        run_builtin_test(2);

        printf("===== NFA->DFA->Minimization Demo =====\n");
        nfa_dfa_demo();
        return 0;
    }

    const char *filename = argv[1];

    if (strcmp(filename, "-h") == 0 || strcmp(filename, "-?") == 0) {
        print_usage();
        return 0;
    }

    if (strcmp(filename, "-nfa") == 0) {
        nfa_dfa_demo();
        return 0;
    }

    if (strcmp(filename, "-test1") == 0) {
        run_builtin_test(1);
        return 0;
    }

    if (strcmp(filename, "-test2") == 0) {
        run_builtin_test(2);
        return 0;
    }

    const char *mode = "-sem";
    if (argc >= 3) mode = argv[2];

    int result = 0;
    if (strcmp(mode, "-lex") == 0) {
        result = run_lexer(filename);
    } else if (strcmp(mode, "-parse") == 0) {
        result = run_parser(filename);
    } else if (strcmp(mode, "-sem") == 0) {
        result = run_semantic(filename);
    } else if (strcmp(mode, "-lr") == 0) {
        result = run_lr_parser(filename);
    } else {
        printf("Unknown option: %s\n\n", mode);
        print_usage();
        return 1;
    }

    return result ? 0 : 1;
}
