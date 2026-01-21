#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbols.h"

typedef struct {
    Lexer *lexer;
    SymbolTable symbols;
    int error_count;
} Parser;

void parser_init(Parser *p, Lexer *lexer);
void parser_free(Parser *p);
int parser_compile(Parser *p, const char *output_path);

#endif
