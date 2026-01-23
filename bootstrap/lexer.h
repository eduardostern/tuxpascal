#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    // End of file
    TOK_EOF = 0,

    // Literals
    TOK_INTEGER,
    TOK_STRING,
    TOK_IDENT,

    // Keywords
    TOK_PROGRAM,
    TOK_VAR,
    TOK_CONST,
    TOK_PROCEDURE,
    TOK_FUNCTION,
    TOK_BEGIN,
    TOK_END,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_WHILE,
    TOK_DO,
    TOK_REPEAT,
    TOK_UNTIL,
    TOK_FOR,
    TOK_TO,
    TOK_DOWNTO,
    TOK_CASE,
    TOK_OF,
    TOK_ARRAY,
    TOK_INTEGER_TYPE,
    TOK_CHAR_TYPE,
    TOK_BOOLEAN_TYPE,
    TOK_STRING_TYPE,
    TOK_TEXT_TYPE,
    TOK_TRUE,
    TOK_FALSE,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_DIV,
    TOK_MOD,
    TOK_FORWARD,

    // Operators and punctuation
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_ASSIGN,      // :=
    TOK_EQ,          // =
    TOK_NEQ,         // <>
    TOK_LT,          // <
    TOK_GT,          // >
    TOK_LE,          // <=
    TOK_GE,          // >=
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_DOT,
    TOK_DOTDOT,      // ..
} TokenType;

typedef struct {
    TokenType type;
    int line;
    int col;
    union {
        int64_t int_val;
        char *str_val;      // for identifiers and strings
    };
} Token;

typedef struct {
    const char *source;
    const char *filename;
    int pos;
    int line;
    int col;
    Token current;
} Lexer;

void lexer_init(Lexer *lex, const char *source, const char *filename);
void lexer_next(Lexer *lex);
const char *token_name(TokenType type);

#endif
