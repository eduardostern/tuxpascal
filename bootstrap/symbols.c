#include "symbols.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Singleton types
static Type t_integer = {TYPE_INTEGER, 0, 0, NULL};
static Type t_char = {TYPE_CHAR, 0, 0, NULL};
static Type t_boolean = {TYPE_BOOLEAN, 0, 0, NULL};
static Type t_string = {TYPE_STRING, 0, 0, NULL};
static Type t_void = {TYPE_VOID, 0, 0, NULL};
static Type t_text = {TYPE_TEXT, 0, 0, NULL};

Type *type_integer(void) { return &t_integer; }
Type *type_char(void) { return &t_char; }
Type *type_boolean(void) { return &t_boolean; }
Type *type_string(void) { return &t_string; }
Type *type_void(void) { return &t_void; }
Type *type_text(void) { return &t_text; }

Type *type_array(int64_t lo, int64_t hi, Type *elem) {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_ARRAY;
    t->array_lo = lo;
    t->array_hi = hi;
    t->array_elem = elem;
    return t;
}

int type_size(Type *t) {
    switch (t->kind) {
        case TYPE_INTEGER: return 8;
        case TYPE_CHAR: return 1;
        case TYPE_BOOLEAN: return 1;
        case TYPE_STRING: return 256; // Fixed size for simplicity
        case TYPE_ARRAY:
            return (t->array_hi - t->array_lo + 1) * type_size(t->array_elem);
        case TYPE_VOID: return 0;
        case TYPE_TEXT: return 272; // fd(8) + mode(8) + filename(256)
    }
    return 0;
}

void symtab_init(SymbolTable *tab) {
    tab->level = 0;
    tab->current = malloc(sizeof(Scope));
    tab->current->symbols = NULL;
    tab->current->parent = NULL;
    tab->current->level = 0;
    tab->current->local_offset = 0;
}

void symtab_enter_scope(SymbolTable *tab) {
    Scope *scope = malloc(sizeof(Scope));
    scope->symbols = NULL;
    scope->parent = tab->current;
    scope->level = ++tab->level;
    scope->local_offset = 0;
    tab->current = scope;
}

void symtab_leave_scope(SymbolTable *tab) {
    Scope *old = tab->current;
    tab->current = old->parent;
    tab->level--;
    // Note: not freeing symbols for simplicity
}

// Case-insensitive string comparison
static int strcasecmp_pascal(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 1;
        a++; b++;
    }
    return *a != *b;
}

Symbol *symtab_add(SymbolTable *tab, const char *name, SymbolKind kind, Type *type) {
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = kind;
    sym->type = type;
    sym->level = tab->level;
    sym->const_val = 0;
    sym->param_count = 0;
    sym->params = NULL;
    sym->label = 0;
    sym->defined = 0;

    if (kind == SYM_VAR || kind == SYM_PARAM) {
        int size = type_size(type);
        // Align to 8 bytes
        size = (size + 7) & ~7;
        tab->current->local_offset += size;
        sym->offset = -tab->current->local_offset;
    } else {
        sym->offset = 0;
    }

    sym->next = tab->current->symbols;
    tab->current->symbols = sym;
    return sym;
}

Symbol *symtab_lookup(SymbolTable *tab, const char *name) {
    for (Scope *s = tab->current; s; s = s->parent) {
        for (Symbol *sym = s->symbols; sym; sym = sym->next) {
            if (strcasecmp_pascal(sym->name, name) == 0) {
                return sym;
            }
        }
    }
    return NULL;
}

Symbol *symtab_lookup_local(SymbolTable *tab, const char *name) {
    for (Symbol *sym = tab->current->symbols; sym; sym = sym->next) {
        if (strcasecmp_pascal(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}
