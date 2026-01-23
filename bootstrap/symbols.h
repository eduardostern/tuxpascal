#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TYPE_INTEGER,
    TYPE_CHAR,
    TYPE_BOOLEAN,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_VOID,
    TYPE_TEXT,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    // For arrays
    int64_t array_lo;
    int64_t array_hi;
    struct Type *array_elem;
} Type;

typedef enum {
    SYM_VAR,
    SYM_CONST,
    SYM_PROCEDURE,
    SYM_FUNCTION,
    SYM_PARAM,
} SymbolKind;

typedef struct Symbol {
    char *name;
    SymbolKind kind;
    Type *type;
    int level;          // Scope nesting level
    int offset;         // Stack offset for variables/params
    int64_t const_val;  // For constants
    int param_count;    // For procedures/functions
    struct Symbol *params; // Parameter list for procedures/functions
    int label;          // Code label for procedures/functions
    int defined;        // Whether proc/func body has been defined
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol *symbols;
    struct Scope *parent;
    int level;
    int local_offset;   // Next available stack offset
} Scope;

typedef struct {
    Scope *current;
    int level;
} SymbolTable;

void symtab_init(SymbolTable *tab);
void symtab_enter_scope(SymbolTable *tab);
void symtab_leave_scope(SymbolTable *tab);

Symbol *symtab_add(SymbolTable *tab, const char *name, SymbolKind kind, Type *type);
Symbol *symtab_lookup(SymbolTable *tab, const char *name);
Symbol *symtab_lookup_local(SymbolTable *tab, const char *name);

Type *type_integer(void);
Type *type_char(void);
Type *type_boolean(void);
Type *type_string(void);
Type *type_void(void);
Type *type_text(void);
Type *type_array(int64_t lo, int64_t hi, Type *elem);

int type_size(Type *t);

#endif
