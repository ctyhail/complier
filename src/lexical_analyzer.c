/**
 * lexical_analyzer.c - PL/0 Lexical Analyzer
 *
 * PL/0 lexical analyzer based on hand-crafted DFA.
 * Recognizes keywords, identifiers, numbers, operators, delimiters.
 * Skips whitespace and block/line comments.
 * Reports illegal chars, illegal words, and length overflow errors.
 */

#include "common.h"
#include "lexical_analyzer.h"
#include <string.h>
#include <ctype.h>

/* Input source */
static FILE *source_file = NULL;
static const char *source_str = NULL;
static int str_pos;
static int line_number = 1;
static int col_number = 0;
static char current_char;
static int is_string_mode = 0;
static int lookahead_char = -1;

/* Get next raw character */
static int get_next_char_raw(void)
{
    int ch;
    if (lookahead_char >= 0) {
        ch = lookahead_char;
        lookahead_char = -1;
        return ch;
    }
    if (source_file) {
        ch = fgetc(source_file);
    } else if (source_str) {
        if (source_str[str_pos] == '\0') ch = EOF;
        else ch = (unsigned char)source_str[str_pos++];
    } else {
        ch = EOF;
    }
    return ch;
}

static int lex_get_char(void)
{
    int ch = get_next_char_raw();
    if (ch == '\n') {
        line_number++;
        col_number = 0;
    } else if (ch != EOF) {
        col_number++;
    }
    current_char = (char)ch;
    return ch;
}

static void lex_unget_char(void)
{
    if (current_char == '\n') line_number--;
    lookahead_char = (unsigned char)current_char;
}

/* ========== Initialize ========== */
void lex_init_file(const char *filename)
{
    source_file = fopen(filename, "r");
    if (!source_file) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        exit(1);
    }
    source_str = NULL;
    str_pos = 0;
    line_number = 1;
    col_number = 0;
    is_string_mode = 0;
    lookahead_char = -1;
    lex_get_char();
}

void lex_init_string(const char *input)
{
    source_file = NULL;
    source_str = input;
    str_pos = 0;
    line_number = 1;
    col_number = 0;
    is_string_mode = 1;
    lookahead_char = -1;
    lex_get_char();
}

void lex_close(void)
{
    if (source_file) {
        fclose(source_file);
        source_file = NULL;
    }
    source_str = NULL;
}

/* ========== Skip whitespace and comments ========== */
static void skip_whitespace_and_comments(void)
{
    while (1) {
        /* Skip whitespace */
        while (current_char == ' ' || current_char == '\t' ||
               current_char == '\n' || current_char == '\r') {
            lex_get_char();
        }

        /* Check for comments */
        if (current_char == '/') {
            int saved_line = line_number;
            int saved_col  = col_number;
            (void)saved_col;
            lex_get_char();

            if (current_char == '/') {
                /* Line comment: skip to end of line */
                while (current_char != '\n' && current_char != EOF) {
                    lex_get_char();
                }
                continue;
            } else if (current_char == '*') {
                /* Block comment: skip until */
                while (1) {
                    lex_get_char();
                    if (current_char == '*') {
                        lex_get_char();
                        if (current_char == '/') {
                            lex_get_char();
                            break;
                        }
                    }
                    if (current_char == EOF) {
                        report_error(saved_line, "Unclosed block comment");
                        return;
                    }
                }
                continue;
            } else {
                /* Not a comment, put back the char after '/' */
                if (current_char == '\n') line_number--;
                lookahead_char = current_char;
                current_char = '/';
                col_number = saved_col;
                return;
            }
        }
        break;
    }
}

/* ========== Keyword lookup ========== */
static TokenType lookup_keyword(const char *word)
{
    for (int i = 0; i < keyword_count; i++) {
        if (strcmp(word, keywords[i].word) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENT;
}

/* ========== Get next token (DFA-driven) ========== */
Token lex_get_next_token(void)
{
    Token token;
    memset(&token, 0, sizeof(token));
    token.line = line_number;
    token.col = col_number;

    skip_whitespace_and_comments();

    token.line = line_number;
    token.col = col_number;

    /* EOF */
    if (current_char == EOF) {
        token.type = TOK_EOF;
        strcpy(token.lexeme, "EOF");
        return token;
    }

    /* State 0: initial */

    /* 1) Letter -> identifier or keyword */
    if (isalpha(current_char)) {
        char buf[MAX_IDENT_LEN + 4] = {0};
        int len = 0;

        while (isalnum(current_char)) {
            if (len < MAX_IDENT_LEN + 2) {
                buf[len++] = current_char;
            }
            lex_get_char();
        }
        buf[len] = '\0';

        strcpy(token.lexeme, buf);
        token.type = lookup_keyword(buf);

        /* Check length only for non-keyword identifiers */
        if (token.type == TOK_IDENT && len > MAX_IDENT_LEN) {
            token.type = TOK_ERR_IDENT_LEN;
            report_error(token.line, "Identifier too long: %s", buf);
        }
        return token;
    }

    /* 2) Digit -> number or illegal word */
    if (isdigit(current_char)) {
        char buf[MAX_NUM_LEN + 4] = {0};
        int len = 0;
        int is_illegal_word = 0;

        while (isalnum(current_char)) {
            if (!is_illegal_word && !isdigit(current_char)) {
                is_illegal_word = 1;
            }
            if (len < MAX_NUM_LEN + 2) {
                buf[len++] = current_char;
            }
            lex_get_char();
        }
        buf[len] = '\0';

        if (is_illegal_word) {
            token.type = TOK_ERR_ILLEGAL_WORD;
            snprintf(token.lexeme, sizeof(token.lexeme), "%s", buf);
            report_error(token.line, "Illegal word (digit-start): %s", buf);
            return token;
        }

        /* Check numeric overflow */
        if (len > MAX_NUM_LEN) {
            token.type = TOK_ERR_NUM_LEN;
            snprintf(token.lexeme, sizeof(token.lexeme), "%s", buf);
            report_error(token.line, "Unsigned integer overflow: %s", buf);
            return token;
        }

        strcpy(token.lexeme, buf);
        token.type = TOK_NUMBER;
        token.value = atoi(buf);
        return token;
    }

    /* 3) Operators and delimiters */
    token.type = TOK_ERROR;
    token.lexeme[0] = current_char;
    token.lexeme[1] = '\0';

    switch (current_char) {
        case '+': token.type = TOK_PLUS;     break;
        case '-': token.type = TOK_MINUS;    break;
        case '*': token.type = TOK_MUL;      break;
        case '/': token.type = TOK_DIV;      break;
        case '=': token.type = TOK_EQ;       break;
        case '(': token.type = TOK_LPAREN;   break;
        case ')': token.type = TOK_RPAREN;   break;
        case ',': token.type = TOK_COMMA;    break;
        case ';': token.type = TOK_SEMICOLON; break;
        case '.': token.type = TOK_PERIOD;   break;
        case '#':
            token.type = TOK_NEQ;
            strcpy(token.lexeme, "#");
            break;

        case '<': {
            lex_get_char();
            if (current_char == '=') {
                token.type = TOK_LE;
                strcpy(token.lexeme, "<=");
                lex_get_char();  /* consume '=', advance */
            } else {
                token.type = TOK_LT;
                strcpy(token.lexeme, "<");
                if (current_char == '\n') line_number--;
                lookahead_char = (unsigned char)current_char;
            }
            return token;
        }

        case '>': {
            lex_get_char();
            if (current_char == '=') {
                token.type = TOK_GE;
                strcpy(token.lexeme, ">=");
                lex_get_char();  /* consume '=', advance */
            } else {
                token.type = TOK_GT;
                strcpy(token.lexeme, ">");
                if (current_char == '\n') line_number--;
                lookahead_char = (unsigned char)current_char;
            }
            return token;
        }

        case ':': {
            lex_get_char();
            if (current_char == '=') {
                token.type = TOK_ASSIGN;
                strcpy(token.lexeme, ":=");
                lex_get_char();  /* consume '=', advance to next */
            } else {
                token.type = TOK_ERROR;
                strcpy(token.lexeme, ":");
                report_error(token.line, "Illegal char ':' (expect '=')");
                if (current_char == '\n') line_number--;
                lookahead_char = (unsigned char)current_char;
                return token;
            }
            return token;
        }

        default: {
            token.type = TOK_ERR_ILLEGAL_CHAR;
            snprintf(token.lexeme, sizeof(token.lexeme), "%c", current_char);
            report_error(token.line, "Illegal char: '%c'", current_char);
            break;
        }
    }

    if (token.type != TOK_EOF) {
        lex_get_char();
    }
    return token;
}

/* ========== Print token in standard format ========== */
void lex_print_token(Token *t)
{
    switch (t->type) {
        case TOK_ERR_ILLEGAL_CHAR:
        case TOK_ERR_ILLEGAL_WORD:
            printf("(非法字符(串),%s,行号:%d)\n", t->lexeme, t->line);
            break;
        case TOK_ERR_IDENT_LEN:
            printf("(标识符长度超长,%s,行号:%d)\n", t->lexeme, t->line);
            break;
        case TOK_ERR_NUM_LEN:
            printf("(无符号整数越界,%s,行号:%d)\n", t->lexeme, t->line);
            break;
        default: {
            const char *cat;
            if (is_keyword(t->type)) cat = "保留字";
            else if (is_operator(t->type)) cat = "运算符";
            else if (is_delimiter(t->type)) cat = "界符";
            else if (t->type == TOK_IDENT) cat = "标识符";
            else if (t->type == TOK_NUMBER) cat = "无符号整数";
            else cat = "其他";
            printf("(%s,%s)\n", cat, t->lexeme);
        }
    }
}

/* ========== Analyze all tokens into queue ========== */
TokenQueue lex_analyze_all(void)
{
    TokenQueue queue;
    token_queue_init(&queue);

    Token t;
    do {
        t = lex_get_next_token();
        token_queue_add(&queue, t);
    } while (t.type != TOK_EOF);

    return queue;
}
