#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const struct { const char *name; TokenType type; } keywords[] = {
    {"program", TOK_PROGRAM},
    {"var", TOK_VAR},
    {"const", TOK_CONST},
    {"procedure", TOK_PROCEDURE},
    {"function", TOK_FUNCTION},
    {"begin", TOK_BEGIN},
    {"end", TOK_END},
    {"if", TOK_IF},
    {"then", TOK_THEN},
    {"else", TOK_ELSE},
    {"while", TOK_WHILE},
    {"do", TOK_DO},
    {"repeat", TOK_REPEAT},
    {"until", TOK_UNTIL},
    {"for", TOK_FOR},
    {"to", TOK_TO},
    {"downto", TOK_DOWNTO},
    {"case", TOK_CASE},
    {"of", TOK_OF},
    {"array", TOK_ARRAY},
    {"integer", TOK_INTEGER_TYPE},
    {"char", TOK_CHAR_TYPE},
    {"boolean", TOK_BOOLEAN_TYPE},
    {"string", TOK_STRING_TYPE},
    {"true", TOK_TRUE},
    {"false", TOK_FALSE},
    {"and", TOK_AND},
    {"or", TOK_OR},
    {"not", TOK_NOT},
    {"div", TOK_DIV},
    {"mod", TOK_MOD},
    {"forward", TOK_FORWARD},
    {NULL, TOK_EOF}
};

void lexer_init(Lexer *lex, const char *source, const char *filename) {
    lex->source = source;
    lex->filename = filename;
    lex->pos = 0;
    lex->line = 1;
    lex->col = 1;
    lexer_next(lex);
}

static char peek(Lexer *lex) {
    return lex->source[lex->pos];
}

static char peek_next(Lexer *lex) {
    if (lex->source[lex->pos] == '\0') return '\0';
    return lex->source[lex->pos + 1];
}

static char advance(Lexer *lex) {
    char c = lex->source[lex->pos++];
    if (c == '\n') {
        lex->line++;
        lex->col = 1;
    } else {
        lex->col++;
    }
    return c;
}

static void skip_whitespace(Lexer *lex) {
    while (1) {
        char c = peek(lex);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lex);
        } else if (c == '{') {
            // Pascal comment { ... }
            advance(lex);
            while (peek(lex) != '}' && peek(lex) != '\0') {
                advance(lex);
            }
            if (peek(lex) == '}') advance(lex);
        } else if (c == '(' && peek_next(lex) == '*') {
            // Pascal comment (* ... *)
            advance(lex);
            advance(lex);
            while (!(peek(lex) == '*' && peek_next(lex) == ')') && peek(lex) != '\0') {
                advance(lex);
            }
            if (peek(lex) == '*') {
                advance(lex);
                advance(lex);
            }
        } else {
            break;
        }
    }
}

static void error(Lexer *lex, const char *msg) {
    fprintf(stderr, "%s:%d:%d: error: %s\n", lex->filename, lex->line, lex->col, msg);
    exit(1);
}

static TokenType check_keyword(const char *ident) {
    // Convert to lowercase for comparison (Pascal is case-insensitive)
    char lower[256];
    int i;
    for (i = 0; ident[i] && i < 255; i++) {
        lower[i] = tolower(ident[i]);
    }
    lower[i] = '\0';

    for (int j = 0; keywords[j].name; j++) {
        if (strcmp(lower, keywords[j].name) == 0) {
            return keywords[j].type;
        }
    }
    return TOK_IDENT;
}

void lexer_next(Lexer *lex) {
    skip_whitespace(lex);

    lex->current.line = lex->line;
    lex->current.col = lex->col;

    char c = peek(lex);

    if (c == '\0') {
        lex->current.type = TOK_EOF;
        return;
    }

    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        int start = lex->pos;
        while (isalnum(peek(lex)) || peek(lex) == '_') {
            advance(lex);
        }
        int len = lex->pos - start;
        char *ident = malloc(len + 1);
        memcpy(ident, lex->source + start, len);
        ident[len] = '\0';

        lex->current.type = check_keyword(ident);
        if (lex->current.type == TOK_IDENT) {
            lex->current.str_val = ident;
        } else {
            free(ident);
        }
        return;
    }

    // Numbers
    if (isdigit(c)) {
        int64_t val = 0;
        while (isdigit(peek(lex))) {
            val = val * 10 + (advance(lex) - '0');
        }
        lex->current.type = TOK_INTEGER;
        lex->current.int_val = val;
        return;
    }

    // String literals
    if (c == '\'') {
        advance(lex);
        int start = lex->pos;
        while (peek(lex) != '\'' && peek(lex) != '\0') {
            if (peek(lex) == '\n') {
                error(lex, "unterminated string");
            }
            advance(lex);
        }
        int len = lex->pos - start;
        char *str = malloc(len + 1);
        memcpy(str, lex->source + start, len);
        str[len] = '\0';
        if (peek(lex) == '\'') advance(lex);
        lex->current.type = TOK_STRING;
        lex->current.str_val = str;
        return;
    }

    // Operators
    advance(lex);
    switch (c) {
        case '+': lex->current.type = TOK_PLUS; break;
        case '-': lex->current.type = TOK_MINUS; break;
        case '*': lex->current.type = TOK_STAR; break;
        case '/': lex->current.type = TOK_SLASH; break;
        case '=': lex->current.type = TOK_EQ; break;
        case '(': lex->current.type = TOK_LPAREN; break;
        case ')': lex->current.type = TOK_RPAREN; break;
        case '[': lex->current.type = TOK_LBRACKET; break;
        case ']': lex->current.type = TOK_RBRACKET; break;
        case ',': lex->current.type = TOK_COMMA; break;
        case ';': lex->current.type = TOK_SEMICOLON; break;
        case ':':
            if (peek(lex) == '=') {
                advance(lex);
                lex->current.type = TOK_ASSIGN;
            } else {
                lex->current.type = TOK_COLON;
            }
            break;
        case '.':
            if (peek(lex) == '.') {
                advance(lex);
                lex->current.type = TOK_DOTDOT;
            } else {
                lex->current.type = TOK_DOT;
            }
            break;
        case '<':
            if (peek(lex) == '=') {
                advance(lex);
                lex->current.type = TOK_LE;
            } else if (peek(lex) == '>') {
                advance(lex);
                lex->current.type = TOK_NEQ;
            } else {
                lex->current.type = TOK_LT;
            }
            break;
        case '>':
            if (peek(lex) == '=') {
                advance(lex);
                lex->current.type = TOK_GE;
            } else {
                lex->current.type = TOK_GT;
            }
            break;
        default:
            error(lex, "unexpected character");
    }
}

const char *token_name(TokenType type) {
    static const char *names[] = {
        "EOF", "INTEGER", "STRING", "IDENT",
        "program", "var", "const", "procedure", "function",
        "begin", "end", "if", "then", "else",
        "while", "do", "repeat", "until",
        "for", "to", "downto", "case", "of",
        "array", "integer", "char", "boolean", "string",
        "true", "false", "and", "or", "not", "div", "mod", "forward",
        "+", "-", "*", "/", ":=", "=", "<>", "<", ">", "<=", ">=",
        "(", ")", "[", "]", ",", ";", ":", ".", ".."
    };
    return names[type];
}
