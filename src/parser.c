#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>

static FILE *out;
static int label_count = 0;

// String table for data section
static struct {
    char *str;
    int len;
} strings[256];
static int num_strings = 0;

static void error(Parser *p, const char *msg) {
    fprintf(stderr, "%s:%d:%d: error: %s (got '%s')\n",
            p->lexer->filename, p->lexer->current.line, p->lexer->current.col,
            msg, token_name(p->lexer->current.type));
    p->error_count++;
    exit(1);
}

static Token *current(Parser *p) {
    return &p->lexer->current;
}

static void advance(Parser *p) {
    lexer_next(p->lexer);
}

static bool check(Parser *p, TokenType type) {
    return current(p)->type == type;
}

static bool match(Parser *p, TokenType type) {
    if (check(p, type)) {
        advance(p);
        return true;
    }
    return false;
}

static void expect(Parser *p, TokenType type) {
    if (!match(p, type)) {
        char msg[100];
        snprintf(msg, sizeof(msg), "expected '%s'", token_name(type));
        error(p, msg);
    }
}

// Assembly output helpers
static void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(out, "    ");
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    va_end(args);
}

static void emit_label(int n) {
    fprintf(out, "L%d:\n", n);
}

static void emit_raw(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    va_end(args);
}

static int new_label(void) {
    return label_count++;
}

// Helper to load a large immediate into a register
// ARM64 mov can only handle 16-bit immediates directly
static void emit_mov_large(const char *reg, int64_t value) {
    if (value >= 0 && value <= 65535) {
        emit("mov %s, #%lld", reg, value);
    } else if (value < 0 && value >= -65535) {
        emit("mov %s, #%lld", reg, -value);
        emit("neg %s, %s", reg, reg);
    } else {
        // Use movz + movk for larger values
        uint64_t uval = (value < 0) ? (uint64_t)(-value) : (uint64_t)value;
        emit("movz %s, #%llu", reg, uval & 0xFFFF);
        if ((uval >> 16) & 0xFFFF) {
            emit("movk %s, #%llu, lsl #16", reg, (uval >> 16) & 0xFFFF);
        }
        if ((uval >> 32) & 0xFFFF) {
            emit("movk %s, #%llu, lsl #32", reg, (uval >> 32) & 0xFFFF);
        }
        if ((uval >> 48) & 0xFFFF) {
            emit("movk %s, #%llu, lsl #48", reg, (uval >> 48) & 0xFFFF);
        }
        if (value < 0) {
            emit("neg %s, %s", reg, reg);
        }
    }
}

// Helper to load from frame pointer with large offset support
// ldur x0, [x29, #offset]
static void emit_load_fp(int offset) {
    if (offset >= -255 && offset <= 255) {
        emit("ldur x0, [x29, #%d]", offset);
    } else {
        // Large offset: use temporary register
        emit_mov_large("x8", offset);
        emit("add x8, x29, x8");
        emit("ldr x0, [x8]");
    }
}

// Helper to store to frame pointer with large offset support
// stur x0, [x29, #offset]
static void emit_store_fp(int offset) {
    if (offset >= -255 && offset <= 255) {
        emit("stur x0, [x29, #%d]", offset);
    } else {
        // Large offset: use temporary register
        emit_mov_large("x8", offset);
        emit("add x8, x29, x8");
        emit("str x0, [x8]");
    }
}

// Helper to load from outer scope via static link chain
// sym_level is the level where the variable is defined
// current_level is the current scope level
static void emit_load_outer(int offset, int sym_level, int current_level) {
    // Follow static link chain
    emit("mov x8, x29");
    for (int i = current_level; i > sym_level; i--) {
        // Load static link from [x8, #-8]
        emit("ldur x8, [x8, #-8]");
    }
    // Now x8 points to the frame where the variable lives
    if (offset >= -255 && offset <= 255) {
        emit("ldur x0, [x8, #%d]", offset);
    } else {
        emit_mov_large("x10", offset);
        emit("add x8, x8, x10");
        emit("ldr x0, [x8]");
    }
}

// Helper to store to outer scope via static link chain
static void emit_store_outer(int offset, int sym_level, int current_level) {
    // Save the value to store
    emit("mov x10, x0");
    // Follow static link chain
    emit("mov x8, x29");
    for (int i = current_level; i > sym_level; i--) {
        // Load static link from [x8, #-8]
        emit("ldur x8, [x8, #-8]");
    }
    // Now x8 points to the frame where the variable lives
    if (offset >= -255 && offset <= 255) {
        emit("stur x10, [x8, #%d]", offset);
    } else {
        emit_mov_large("x11", offset);
        emit("add x8, x8, x11");
        emit("str x10, [x8]");
    }
}

// Helper to compute address in outer scope
static void emit_addr_outer(int offset, int sym_level, int current_level) {
    // Follow static link chain
    emit("mov x8, x29");
    for (int i = current_level; i > sym_level; i--) {
        // Load static link from [x8, #-8]
        emit("ldur x8, [x8, #-8]");
    }
    // Now x8 points to the frame where the variable lives
    if (offset >= -4095 && offset <= 4095) {
        if (offset >= 0) {
            emit("add x0, x8, #%d", offset);
        } else {
            emit("sub x0, x8, #%d", -offset);
        }
    } else {
        emit_mov_large("x0", offset);
        emit("add x0, x8, x0");
    }
}

// Helper to emit static link for procedure/function call
// The callee was declared at sym_level, so its static link should point to frame at sym_level
// We're currently at current_level, so we follow (current_level - sym_level) links
static void emit_static_link(int sym_level, int current_level) {
    emit("mov x9, x29");
    // Follow the static link chain to reach the scope where the procedure was declared
    for (int i = current_level; i > sym_level; i--) {
        emit("ldur x9, [x9, #-8]");
    }
}

// Helper to compute address relative to frame pointer with large offset support
// Result: x0 = x29 + offset (which is x29 - (-offset))
static void emit_addr_fp(int offset) {
    if (offset >= -4095 && offset <= 4095) {
        if (offset >= 0) {
            emit("add x0, x29, #%d", offset);
        } else {
            emit("sub x0, x29, #%d", -offset);
        }
    } else {
        // Large offset: use temporary register
        emit_mov_large("x0", offset);
        emit("add x0, x29, x0");
    }
}

// Helper to adjust stack pointer with large offset support
static void emit_sub_sp(int size) {
    if (size <= 4095) {
        emit("sub sp, sp, #%d", size);
    } else {
        emit_mov_large("x8", size);
        emit("sub sp, sp, x8");
    }
}

static void emit_add_sp(int size) {
    if (size <= 4095) {
        emit("add sp, sp, #%d", size);
    } else {
        emit_mov_large("x8", size);
        emit("add sp, sp, x8");
    }
}

static int add_string(const char *s, int len) {
    int id = num_strings++;
    strings[id].str = strdup(s);
    strings[id].len = len;
    return id;
}

// Forward declarations
static void parse_block_impl(Parser *p, int is_main);
static void parse_statement(Parser *p);
static void parse_expression(Parser *p);
static Type *parse_type(Parser *p);
static void parse_procedure_declaration(Parser *p);
static void parse_function_declaration(Parser *p);

void parser_init(Parser *p, Lexer *lexer) {
    p->lexer = lexer;
    p->error_count = 0;
    symtab_init(&p->symbols);
}

void parser_free(Parser *p) {
    (void)p;
}

// Emit prologue
static void emit_prologue(int local_size) {
    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    if (local_size > 0) {
        int aligned = (local_size + 15) & ~15;
        emit_sub_sp(aligned);
    }
}

// Emit prologue for procedure/function with static link
// Static link is passed in x9, stored at [x29, #-8]
static void emit_prologue_proc(int local_size) {
    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    // local_size already includes 8 bytes for static link at offset -8
    if (local_size > 0) {
        int aligned = (local_size + 15) & ~15;
        emit_sub_sp(aligned);
    }
    // Store static link at [x29, #-8]
    emit("stur x9, [x29, #-8]");
}

// Emit epilogue
static void emit_epilogue(int local_size) {
    if (local_size > 0) {
        int aligned = (local_size + 15) & ~15;
        emit_add_sp(aligned);
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
}

// Emit write syscall (x0=fd, x1=buf, x2=len)
static void emit_write_syscall(void) {
    emit("mov x16, #4");
    emit("movk x16, #0x200, lsl #16");
    emit("svc #0x80");
}

// Emit exit syscall (x0=code)
static void emit_exit_syscall(void) {
    emit("mov x16, #1");
    emit("movk x16, #0x200, lsl #16");
    emit("svc #0x80");
}

// Emit print integer routine
static int emit_print_int_routine(void) {
    int label = new_label();
    emit_raw("\n// Print integer routine");
    emit_label(label);

    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    emit("sub sp, sp, #32");

    emit("mov x19, x0");  // Save number
    emit("mov x20, #0");  // Digit count

    // Handle negative
    int positive = new_label();
    emit("cmp x19, #0");
    emit("b.ge L%d", positive);

    // Print minus
    emit("mov x0, #'-'");
    emit("strb w0, [sp]");
    emit("mov x0, #1");
    emit("mov x1, sp");
    emit("mov x2, #1");
    emit_write_syscall();
    emit("neg x19, x19");

    emit_label(positive);

    // Handle zero
    int not_zero = new_label();
    emit("cmp x19, #0");
    emit("b.ne L%d", not_zero);

    emit("mov x0, #'0'");
    emit("strb w0, [sp]");
    emit("mov x0, #1");
    emit("mov x1, sp");
    emit("mov x2, #1");
    emit_write_syscall();
    int done = new_label();
    emit("b L%d", done);

    emit_label(not_zero);

    // Extract digits loop
    int loop = new_label();
    int loop_done = new_label();
    emit_label(loop);

    emit("cmp x19, #0");
    emit("b.eq L%d", loop_done);

    emit("mov x21, #10");
    emit("sdiv x22, x19, x21");
    emit("msub x23, x22, x21, x19");
    emit("add x23, x23, #'0'");

    emit("add x24, sp, x20");
    emit("strb w23, [x24]");

    emit("add x20, x20, #1");
    emit("mov x19, x22");
    emit("b L%d", loop);

    emit_label(loop_done);

    // Print digits in reverse
    int print_loop = new_label();
    int print_done = new_label();
    emit_label(print_loop);

    emit("cmp x20, #0");
    emit("b.eq L%d", print_done);

    emit("sub x20, x20, #1");
    emit("add x24, sp, x20");
    emit("ldrb w0, [x24]");
    emit("strb w0, [sp, #31]");

    emit("mov x0, #1");
    emit("add x1, sp, #31");
    emit("mov x2, #1");
    emit_write_syscall();

    emit("b L%d", print_loop);

    emit_label(print_done);
    emit_label(done);

    emit("add sp, sp, #32");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");

    return label;
}

// Emit newline routine
static int emit_newline_routine(void) {
    int label = new_label();
    emit_raw("\n// Print newline routine");
    emit_label(label);

    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    emit("sub sp, sp, #16");

    emit("mov x0, #10");  // newline
    emit("strb w0, [sp]");
    emit("mov x0, #1");
    emit("mov x1, sp");
    emit("mov x2, #1");
    emit_write_syscall();

    emit("add sp, sp, #16");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");

    return label;
}

// Emit readchar routine - reads one char from stdin, returns in x0 (-1 for EOF)
static int emit_readchar_routine(void) {
    int label = new_label();
    emit_raw("\n// Read char routine");
    emit_label(label);

    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    emit("sub sp, sp, #16");

    // read(0, buf, 1)
    emit("mov x0, #0");       // fd = stdin
    emit("mov x1, sp");       // buffer on stack
    emit("mov x2, #1");       // read 1 byte
    emit("mov x16, #3");      // read syscall
    emit("movk x16, #0x200, lsl #16");
    emit("svc #0x80");

    // Check return value - if 0 or negative, return -1 for EOF
    emit("cmp x0, #1");
    int got_char = new_label();
    emit("b.ge L%d", got_char);
    emit("mov x0, #-1");      // EOF
    int done = new_label();
    emit("b L%d", done);

    emit_label(got_char);
    emit("ldrb w0, [sp]");    // Load the character

    emit_label(done);
    emit("add sp, sp, #16");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");

    return label;
}

// Emit print char routine - prints char in x0
static int emit_print_char_routine(void) {
    int label = new_label();
    emit_raw("\n// Print char routine");
    emit_label(label);

    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    emit("sub sp, sp, #16");

    emit("strb w0, [sp]");
    emit("mov x0, #1");       // fd = stdout
    emit("mov x1, sp");       // buffer
    emit("mov x2, #1");       // 1 byte
    emit("mov x16, #4");
    emit("movk x16, #0x200, lsl #16");
    emit("svc #0x80");

    emit("add sp, sp, #16");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");

    return label;
}

// Runtime routine labels
static int rt_print_int = -1;
static int rt_newline = -1;
static int rt_readchar = -1;
static int rt_print_char = -1;

// Parse factor
static void parse_factor(Parser *p) {
    // Check for built-in functions
    if (check(p, TOK_IDENT)) {
        char *name = current(p)->str_val;
        char lower[256];
        for (int i = 0; name[i] && i < 255; i++) {
            lower[i] = tolower(name[i]);
            lower[i+1] = '\0';
        }

        if (strcmp(lower, "readchar") == 0) {
            advance(p);
            free(name);
            if (match(p, TOK_LPAREN)) {
                expect(p, TOK_RPAREN);
            }
            emit("bl L%d", rt_readchar);
            return;
        }
        if (strcmp(lower, "ord") == 0) {
            advance(p);
            free(name);
            expect(p, TOK_LPAREN);
            parse_expression(p);
            expect(p, TOK_RPAREN);
            // ord() is a no-op since chars are already integers
            return;
        }
        if (strcmp(lower, "chr") == 0) {
            advance(p);
            free(name);
            expect(p, TOK_LPAREN);
            parse_expression(p);
            expect(p, TOK_RPAREN);
            // chr() is a no-op since we store chars as integers
            return;
        }
        // Not a built-in, fall through to regular identifier handling
    }

    if (check(p, TOK_INTEGER)) {
        int64_t val = current(p)->int_val;
        advance(p);
        if (val >= 0 && val < 65536) {
            emit("mov x0, #%lld", (long long)val);
        } else {
            emit("mov x0, #%lld", (long long)(val & 0xFFFF));
            if ((val >> 16) & 0xFFFF)
                emit("movk x0, #%lld, lsl #16", (long long)((val >> 16) & 0xFFFF));
            if ((val >> 32) & 0xFFFF)
                emit("movk x0, #%lld, lsl #32", (long long)((val >> 32) & 0xFFFF));
            if ((val >> 48) & 0xFFFF)
                emit("movk x0, #%lld, lsl #48", (long long)((val >> 48) & 0xFFFF));
        }
    }
    else if (check(p, TOK_STRING)) {
        // Strings are handled specially in write/writeln
        error(p, "string literals only allowed in write/writeln");
    }
    else if (check(p, TOK_TRUE)) {
        advance(p);
        emit("mov x0, #1");
    }
    else if (check(p, TOK_FALSE)) {
        advance(p);
        emit("mov x0, #0");
    }
    else if (check(p, TOK_IDENT)) {
        char *name = current(p)->str_val;
        advance(p);

        Symbol *sym = symtab_lookup(&p->symbols, name);
        if (!sym) {
            char msg[100];
            snprintf(msg, sizeof(msg), "undefined identifier '%s'", name);
            error(p, msg);
        }

        if (sym->kind == SYM_CONST) {
            emit("mov x0, #%lld", (long long)sym->const_val);
        }
        else if (sym->kind == SYM_FUNCTION) {
            // Function call - result returned in x0
            int arg_count = 0;
            if (match(p, TOK_LPAREN)) {
                if (!check(p, TOK_RPAREN)) {
                    do {
                        parse_expression(p);
                        emit("str x0, [sp, #-16]!");
                        arg_count++;
                    } while (match(p, TOK_COMMA));
                }
                expect(p, TOK_RPAREN);
            }

            // Pop arguments into registers (in reverse order)
            for (int i = arg_count - 1; i >= 0; i--) {
                emit("ldr x%d, [sp], #16", i);
            }

            // Pass static link in x9 - points to the frame where the function was declared
            emit_static_link(sym->level, p->symbols.current->level);
            emit("bl L%d", sym->label);
            // Result is in x0
        }
        else if (sym->kind == SYM_VAR || sym->kind == SYM_PARAM) {
            // Check if this might be a recursive function call
            // (result variable has same name as function, followed by parens)
            if (check(p, TOK_LPAREN)) {
                // Look for a function with this name in parent scopes
                Symbol *func_sym = NULL;
                for (Scope *s = p->symbols.current->parent; s; s = s->parent) {
                    for (Symbol *fs = s->symbols; fs; fs = fs->next) {
                        if (fs->kind == SYM_FUNCTION && strcasecmp(fs->name, name) == 0) {
                            func_sym = fs;
                            break;
                        }
                    }
                    if (func_sym) break;
                }
                if (func_sym) {
                    // It's a recursive function call
                    int arg_count = 0;
                    expect(p, TOK_LPAREN);
                    if (!check(p, TOK_RPAREN)) {
                        do {
                            parse_expression(p);
                            emit("str x0, [sp, #-16]!");
                            arg_count++;
                        } while (match(p, TOK_COMMA));
                    }
                    expect(p, TOK_RPAREN);

                    for (int i = arg_count - 1; i >= 0; i--) {
                        emit("ldr x%d, [sp], #16", i);
                    }
                    // Pass static link in x9 - points to the frame where the function was declared
                    emit_static_link(func_sym->level, p->symbols.current->level);
                    emit("bl L%d", func_sym->label);
                } else {
                    // Not a recursive call, just load the variable
                    int current_level = p->symbols.current->level;
                    if (sym->level < current_level) {
                        emit_load_outer(sym->offset, sym->level, current_level);
                    } else {
                        emit_load_fp(sym->offset);
                    }
                }
            } else if (check(p, TOK_LBRACKET)) {
                // Array element access: arr[index]
                if (!sym->type || sym->type->kind != TYPE_ARRAY) {
                    error(p, "indexing requires an array");
                }
                expect(p, TOK_LBRACKET);
                // Load base address of array
                int current_level = p->symbols.current->level;
                if (sym->level < current_level) {
                    emit_addr_outer(sym->offset, sym->level, current_level);
                } else {
                    emit_addr_fp(sym->offset);
                }
                emit("str x0, [sp, #-16]!");  // save base address
                parse_expression(p);  // index into x0
                expect(p, TOK_RBRACKET);
                // Adjust for array lower bound
                if (sym->type->array_lo != 0) {
                    emit("sub x0, x0, #%lld", sym->type->array_lo);
                }
                // Each element is 8 bytes (for now, all types are 8 bytes)
                emit("lsl x0, x0, #3");  // multiply by 8
                emit("ldr x1, [sp], #16");  // get base
                emit("add x0, x1, x0");  // address = base + offset
                emit("ldr x0, [x0]");  // load value
            } else {
                // Check if variable is in an outer scope
                int current_level = p->symbols.current->level;
                if (sym->level < current_level) {
                    emit_load_outer(sym->offset, sym->level, current_level);
                } else {
                    emit_load_fp(sym->offset);
                }
            }
        }

        free(name);
    }
    else if (match(p, TOK_LPAREN)) {
        parse_expression(p);
        expect(p, TOK_RPAREN);
    }
    else if (match(p, TOK_NOT)) {
        parse_factor(p);
        emit("cmp x0, #0");
        emit("cset x0, eq");
    }
    else {
        error(p, "expected expression");
    }
}

// Parse term
static void parse_term(Parser *p) {
    parse_factor(p);

    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_DIV) ||
           check(p, TOK_MOD) || check(p, TOK_AND)) {
        TokenType op = current(p)->type;
        advance(p);

        emit("str x0, [sp, #-16]!");

        parse_factor(p);

        emit("mov x1, x0");
        emit("ldr x0, [sp], #16");

        switch (op) {
            case TOK_STAR:
                emit("mul x0, x0, x1");
                break;
            case TOK_SLASH:
            case TOK_DIV:
                emit("sdiv x0, x0, x1");
                break;
            case TOK_MOD:
                emit("sdiv x2, x0, x1");
                emit("msub x0, x2, x1, x0");
                break;
            case TOK_AND:
                emit("and x0, x0, x1");
                break;
            default:
                break;
        }
    }
}

// Parse simple expression
static void parse_simple_expression(Parser *p) {
    bool negate = false;
    if (check(p, TOK_PLUS)) {
        advance(p);
    } else if (check(p, TOK_MINUS)) {
        advance(p);
        negate = true;
    }

    parse_term(p);

    if (negate) {
        emit("neg x0, x0");
    }

    while (check(p, TOK_PLUS) || check(p, TOK_MINUS) || check(p, TOK_OR)) {
        TokenType op = current(p)->type;
        advance(p);

        emit("str x0, [sp, #-16]!");

        parse_term(p);

        emit("mov x1, x0");
        emit("ldr x0, [sp], #16");

        switch (op) {
            case TOK_PLUS:
                emit("add x0, x0, x1");
                break;
            case TOK_MINUS:
                emit("sub x0, x0, x1");
                break;
            case TOK_OR:
                emit("orr x0, x0, x1");
                break;
            default:
                break;
        }
    }
}

// Parse expression
static void parse_expression(Parser *p) {
    parse_simple_expression(p);

    if (check(p, TOK_EQ) || check(p, TOK_NEQ) || check(p, TOK_LT) ||
        check(p, TOK_GT) || check(p, TOK_LE) || check(p, TOK_GE)) {
        TokenType op = current(p)->type;
        advance(p);

        emit("str x0, [sp, #-16]!");

        parse_simple_expression(p);

        emit("mov x1, x0");
        emit("ldr x0, [sp], #16");
        emit("cmp x0, x1");

        const char *cond;
        switch (op) {
            case TOK_EQ:  cond = "eq"; break;
            case TOK_NEQ: cond = "ne"; break;
            case TOK_LT:  cond = "lt"; break;
            case TOK_GT:  cond = "gt"; break;
            case TOK_LE:  cond = "le"; break;
            case TOK_GE:  cond = "ge"; break;
            default:      cond = "eq"; break;
        }
        emit("cset x0, %s", cond);
    }
}

// Parse assignment or procedure call
static void parse_assignment_or_call(Parser *p) {
    char *name = current(p)->str_val;
    advance(p);

    // Check for built-in procedures
    char lower_name[256];
    for (int i = 0; name[i] && i < 255; i++) {
        lower_name[i] = tolower(name[i]);
        lower_name[i+1] = '\0';
    }

    if (strcmp(lower_name, "write") == 0 || strcmp(lower_name, "writeln") == 0) {
        bool newline = (strcmp(lower_name, "writeln") == 0);
        free(name);

        if (match(p, TOK_LPAREN)) {
            do {
                if (check(p, TOK_STRING)) {
                    char *str = current(p)->str_val;
                    advance(p);
                    int len = strlen(str);
                    int id = add_string(str, len);

                    emit("adrp x1, str%d@PAGE", id);
                    emit("add x1, x1, str%d@PAGEOFF", id);
                    emit("mov x0, #1");
                    emit("mov x2, #%d", len);
                    emit_write_syscall();

                    free(str);
                } else {
                    parse_expression(p);
                    emit("bl L%d", rt_print_int);
                }
            } while (match(p, TOK_COMMA));
            expect(p, TOK_RPAREN);
        }

        if (newline) {
            emit("bl L%d", rt_newline);
        }
        return;
    }

    if (strcmp(lower_name, "writechar") == 0) {
        free(name);
        expect(p, TOK_LPAREN);
        parse_expression(p);
        expect(p, TOK_RPAREN);
        emit("bl L%d", rt_print_char);
        return;
    }

    if (strcmp(lower_name, "halt") == 0) {
        free(name);
        if (match(p, TOK_LPAREN)) {
            parse_expression(p);
            expect(p, TOK_RPAREN);
        } else {
            emit("mov x0, #0");
        }
        emit_exit_syscall();
        return;
    }

    Symbol *sym = symtab_lookup(&p->symbols, name);
    if (!sym) {
        char msg[100];
        snprintf(msg, sizeof(msg), "undefined identifier '%s'", name);
        error(p, msg);
    }

    // Check if it's a procedure call
    if (sym->kind == SYM_PROCEDURE) {
        // Parse arguments
        int arg_count = 0;
        if (match(p, TOK_LPAREN)) {
            if (!check(p, TOK_RPAREN)) {
                do {
                    parse_expression(p);
                    // Save argument on stack
                    emit("str x0, [sp, #-16]!");
                    arg_count++;
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN);
        }

        // Pop arguments into registers (in reverse order)
        for (int i = arg_count - 1; i >= 0; i--) {
            emit("ldr x%d, [sp], #16", i);
        }

        // Pass static link in x9 - points to the frame where the procedure was declared
        emit_static_link(sym->level, p->symbols.current->level);

        // Call the procedure
        emit("bl L%d", sym->label);
        free(name);
        return;
    }

    // Check for array element assignment: arr[index] := value
    if (check(p, TOK_LBRACKET)) {
        if (!sym->type || sym->type->kind != TYPE_ARRAY) {
            error(p, "indexing requires an array");
        }
        expect(p, TOK_LBRACKET);
        // Load base address of array
        int current_level = p->symbols.current->level;
        if (sym->level < current_level) {
            emit_addr_outer(sym->offset, sym->level, current_level);
        } else {
            emit_addr_fp(sym->offset);
        }
        emit("str x0, [sp, #-16]!");  // save base address
        parse_expression(p);  // index into x0
        expect(p, TOK_RBRACKET);
        // Adjust for array lower bound
        if (sym->type->array_lo != 0) {
            emit("sub x0, x0, #%lld", sym->type->array_lo);
        }
        // Each element is 8 bytes
        emit("lsl x0, x0, #3");  // multiply by 8
        emit("ldr x1, [sp], #16");  // get base
        emit("add x0, x1, x0");  // element address
        emit("str x0, [sp, #-16]!");  // save element address

        expect(p, TOK_ASSIGN);
        parse_expression(p);  // value into x0

        emit("ldr x1, [sp], #16");  // get element address
        emit("str x0, [x1]");  // store value
        free(name);
        return;
    }

    // Assignment
    expect(p, TOK_ASSIGN);
    parse_expression(p);

    if (sym->kind == SYM_VAR || sym->kind == SYM_PARAM) {
        // Check if variable is in an outer scope
        int current_level = p->symbols.current->level;
        if (sym->level < current_level) {
            emit_store_outer(sym->offset, sym->level, current_level);
        } else {
            emit_store_fp(sym->offset);
        }
    }

    free(name);
}

// Parse if statement
static void parse_if_statement(Parser *p) {
    expect(p, TOK_IF);
    parse_expression(p);
    expect(p, TOK_THEN);

    int else_label = new_label();
    int end_label = new_label();

    emit("cmp x0, #0");
    emit("b.eq L%d", else_label);

    parse_statement(p);

    if (check(p, TOK_ELSE)) {
        emit("b L%d", end_label);
    }

    emit_label(else_label);

    if (match(p, TOK_ELSE)) {
        parse_statement(p);
    }

    emit_label(end_label);
}

// Parse while statement
static void parse_while_statement(Parser *p) {
    expect(p, TOK_WHILE);

    int loop_label = new_label();
    int end_label = new_label();

    emit_label(loop_label);

    parse_expression(p);

    emit("cmp x0, #0");
    emit("b.eq L%d", end_label);

    expect(p, TOK_DO);
    parse_statement(p);

    emit("b L%d", loop_label);
    emit_label(end_label);
}

// Parse repeat statement
static void parse_repeat_statement(Parser *p) {
    expect(p, TOK_REPEAT);

    int loop_label = new_label();
    emit_label(loop_label);

    while (!check(p, TOK_UNTIL) && !check(p, TOK_EOF)) {
        parse_statement(p);
        if (!check(p, TOK_UNTIL)) {
            expect(p, TOK_SEMICOLON);
        }
    }

    expect(p, TOK_UNTIL);
    parse_expression(p);

    emit("cmp x0, #0");
    emit("b.eq L%d", loop_label);
}

// Parse for statement
static void parse_for_statement(Parser *p) {
    expect(p, TOK_FOR);

    if (!check(p, TOK_IDENT)) {
        error(p, "expected identifier");
    }
    char *var_name = current(p)->str_val;
    advance(p);

    Symbol *sym = symtab_lookup(&p->symbols, var_name);
    if (!sym || (sym->kind != SYM_VAR && sym->kind != SYM_PARAM)) {
        error(p, "for loop variable must be a variable");
    }

    expect(p, TOK_ASSIGN);
    parse_expression(p);

    int current_level = p->symbols.current->level;
    if (sym->level < current_level) {
        emit_store_outer(sym->offset, sym->level, current_level);
    } else {
        emit_store_fp(sym->offset);
    }

    bool downto = false;
    if (match(p, TOK_TO)) {
        downto = false;
    } else if (match(p, TOK_DOWNTO)) {
        downto = true;
    } else {
        error(p, "expected 'to' or 'downto'");
    }

    parse_expression(p);
    emit("str x0, [sp, #-16]!");

    expect(p, TOK_DO);

    int loop_label = new_label();
    int end_label = new_label();

    emit_label(loop_label);

    if (sym->level < current_level) {
        emit_load_outer(sym->offset, sym->level, current_level);
    } else {
        emit_load_fp(sym->offset);
    }
    emit("ldur x1, [sp]");

    if (downto) {
        emit("cmp x0, x1");
        emit("b.lt L%d", end_label);
    } else {
        emit("cmp x0, x1");
        emit("b.gt L%d", end_label);
    }

    parse_statement(p);

    if (sym->level < current_level) {
        emit_load_outer(sym->offset, sym->level, current_level);
    } else {
        emit_load_fp(sym->offset);
    }
    if (downto) {
        emit("sub x0, x0, #1");
    } else {
        emit("add x0, x0, #1");
    }
    if (sym->level < current_level) {
        emit_store_outer(sym->offset, sym->level, current_level);
    } else {
        emit_store_fp(sym->offset);
    }

    emit("b L%d", loop_label);
    emit_label(end_label);

    emit("add sp, sp, #16");

    free(var_name);
}

// Parse statement
static void parse_statement(Parser *p) {
    if (check(p, TOK_BEGIN)) {
        expect(p, TOK_BEGIN);
        while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
            parse_statement(p);
            if (!check(p, TOK_END)) {
                expect(p, TOK_SEMICOLON);
            }
        }
        expect(p, TOK_END);
    }
    else if (check(p, TOK_IF)) {
        parse_if_statement(p);
    }
    else if (check(p, TOK_WHILE)) {
        parse_while_statement(p);
    }
    else if (check(p, TOK_REPEAT)) {
        parse_repeat_statement(p);
    }
    else if (check(p, TOK_FOR)) {
        parse_for_statement(p);
    }
    else if (check(p, TOK_IDENT)) {
        parse_assignment_or_call(p);
    }
}

// Parse type
static Type *parse_type(Parser *p) {
    if (match(p, TOK_INTEGER_TYPE)) {
        return type_integer();
    }
    if (match(p, TOK_CHAR_TYPE)) {
        return type_char();
    }
    if (match(p, TOK_BOOLEAN_TYPE)) {
        return type_boolean();
    }
    if (match(p, TOK_STRING_TYPE)) {
        return type_string();
    }
    if (match(p, TOK_ARRAY)) {
        expect(p, TOK_LBRACKET);
        if (!check(p, TOK_INTEGER)) {
            error(p, "expected integer");
        }
        int64_t lo = current(p)->int_val;
        advance(p);
        expect(p, TOK_DOTDOT);
        if (!check(p, TOK_INTEGER)) {
            error(p, "expected integer");
        }
        int64_t hi = current(p)->int_val;
        advance(p);
        expect(p, TOK_RBRACKET);
        expect(p, TOK_OF);
        Type *elem = parse_type(p);
        return type_array(lo, hi, elem);
    }
    error(p, "expected type");
    return type_void();
}

// Parse var declarations
static void parse_var_declarations(Parser *p) {
    expect(p, TOK_VAR);

    while (check(p, TOK_IDENT)) {
        char *names[100];
        int name_count = 0;

        do {
            if (!check(p, TOK_IDENT)) {
                error(p, "expected identifier");
            }
            names[name_count++] = current(p)->str_val;
            advance(p);
        } while (match(p, TOK_COMMA));

        expect(p, TOK_COLON);
        Type *type = parse_type(p);
        expect(p, TOK_SEMICOLON);

        for (int i = 0; i < name_count; i++) {
            if (symtab_lookup_local(&p->symbols, names[i])) {
                char msg[100];
                snprintf(msg, sizeof(msg), "duplicate identifier '%s'", names[i]);
                error(p, msg);
            }
            symtab_add(&p->symbols, names[i], SYM_VAR, type);
            free(names[i]);
        }
    }
}

// Parse const declarations
static void parse_const_declarations(Parser *p) {
    expect(p, TOK_CONST);

    while (check(p, TOK_IDENT)) {
        char *name = current(p)->str_val;
        advance(p);
        expect(p, TOK_EQ);

        if (!check(p, TOK_INTEGER)) {
            error(p, "expected constant value");
        }
        int64_t val = current(p)->int_val;
        advance(p);
        expect(p, TOK_SEMICOLON);

        Symbol *sym = symtab_add(&p->symbols, name, SYM_CONST, type_integer());
        sym->const_val = val;
        free(name);
    }
}

// Current function being compiled (for function result variable)
static Symbol *current_function = NULL;

// Forward declare parse_block_impl
static void parse_block_impl(Parser *p, int is_main);

// Parse parameter list, returns count of parameters
static int parse_parameters(Parser *p, Symbol *proc_sym) {
    int count = 0;
    Symbol *param_list = NULL;
    Symbol *last_param = NULL;

    expect(p, TOK_LPAREN);

    if (!check(p, TOK_RPAREN)) {
        do {
            // Parse parameter names
            char *names[32];
            int name_count = 0;

            do {
                if (!check(p, TOK_IDENT)) {
                    error(p, "expected parameter name");
                }
                names[name_count++] = current(p)->str_val;
                advance(p);
            } while (match(p, TOK_COMMA));

            expect(p, TOK_COLON);
            Type *type = parse_type(p);

            // Add parameters to list (they'll be added to scope when body is parsed)
            for (int i = 0; i < name_count; i++) {
                Symbol *param = malloc(sizeof(Symbol));
                param->name = names[i];
                param->kind = SYM_PARAM;
                param->type = type;
                param->level = 0;
                param->offset = 0;  // Will be set when added to scope
                param->const_val = 0;
                param->param_count = 0;
                param->params = NULL;
                param->label = 0;
                param->defined = 0;
                param->next = NULL;

                if (last_param) {
                    last_param->next = param;
                } else {
                    param_list = param;
                }
                last_param = param;
                count++;
            }
        } while (match(p, TOK_SEMICOLON));
    }

    expect(p, TOK_RPAREN);

    proc_sym->params = param_list;
    proc_sym->param_count = count;

    return count;
}

// Parse procedure declaration
static void parse_procedure_declaration(Parser *p) {
    expect(p, TOK_PROCEDURE);

    if (!check(p, TOK_IDENT)) {
        error(p, "expected procedure name");
    }
    char *name = current(p)->str_val;
    advance(p);

    // Add procedure to symbol table
    Symbol *proc_sym = symtab_lookup_local(&p->symbols, name);
    if (proc_sym) {
        // Forward declaration exists
        if (proc_sym->defined) {
            error(p, "procedure already defined");
        }
    } else {
        proc_sym = symtab_add(&p->symbols, name, SYM_PROCEDURE, type_void());
        proc_sym->label = new_label();
    }

    // Parse parameters if present
    if (check(p, TOK_LPAREN)) {
        parse_parameters(p, proc_sym);
    }

    expect(p, TOK_SEMICOLON);

    // Check for forward declaration
    if (check(p, TOK_FORWARD)) {
        advance(p);
        expect(p, TOK_SEMICOLON);
        free(name);
        return;
    }

    // Emit procedure code
    emit_raw("");
    emit_raw("// Procedure %s", name);
    emit_label(proc_sym->label);

    // Enter new scope
    symtab_enter_scope(&p->symbols);

    // Reserve offset -8 for static link (local_offset is positive, negated for actual offset)
    p->symbols.current->local_offset = 8;

    // Add parameters to scope with correct offsets
    // Parameters are passed in x0-x7, then on stack
    // We'll copy them to local stack slots for simplicity
    int param_idx = 0;
    for (Symbol *param = proc_sym->params; param; param = param->next) {
        Symbol *local = symtab_add(&p->symbols, param->name, SYM_PARAM, param->type);
        (void)local;  // suppress warning
        param_idx++;
    }

    // Parse const and var declarations
    while (check(p, TOK_CONST) || check(p, TOK_VAR)) {
        if (check(p, TOK_CONST)) {
            parse_const_declarations(p);
        } else if (check(p, TOK_VAR)) {
            parse_var_declarations(p);
        }
    }

    // If there are nested procedures/functions, emit a jump over them
    int body_label = -1;
    if (check(p, TOK_PROCEDURE) || check(p, TOK_FUNCTION)) {
        body_label = new_label();
        emit("b L%d", body_label);
    }

    // Parse nested procedure/function declarations
    while (check(p, TOK_PROCEDURE) || check(p, TOK_FUNCTION)) {
        if (check(p, TOK_PROCEDURE)) {
            parse_procedure_declaration(p);
        } else if (check(p, TOK_FUNCTION)) {
            parse_function_declaration(p);
        }
    }

    // Emit label for body if we jumped over nested procs/funcs
    if (body_label >= 0) {
        emit_label(body_label);
    }

    int local_size = p->symbols.current->local_offset;
    emit_prologue_proc(local_size);

    // Copy parameters from registers to stack
    param_idx = 0;
    for (Symbol *param = proc_sym->params; param; param = param->next) {
        Symbol *local = symtab_lookup_local(&p->symbols, param->name);
        if (param_idx < 8) {
            // Move parameter from register xN to x0, then store
            if (param_idx != 0) {
                emit("mov x0, x%d", param_idx);
            }
            emit_store_fp(local->offset);
        }
        param_idx++;
    }

    // Parse begin..end
    expect(p, TOK_BEGIN);
    while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
        parse_statement(p);
        if (!check(p, TOK_END)) {
            expect(p, TOK_SEMICOLON);
        }
    }
    expect(p, TOK_END);

    emit_epilogue(local_size);

    // Leave scope
    symtab_leave_scope(&p->symbols);
    proc_sym->defined = 1;

    expect(p, TOK_SEMICOLON);
    free(name);
}

// Parse function declaration
static void parse_function_declaration(Parser *p) {
    expect(p, TOK_FUNCTION);

    if (!check(p, TOK_IDENT)) {
        error(p, "expected function name");
    }
    char *name = current(p)->str_val;
    advance(p);

    // Add function to symbol table
    Symbol *func_sym = symtab_lookup_local(&p->symbols, name);
    if (func_sym) {
        if (func_sym->defined) {
            error(p, "function already defined");
        }
    } else {
        func_sym = symtab_add(&p->symbols, name, SYM_FUNCTION, NULL);
        func_sym->label = new_label();
    }

    // Parse parameters if present
    if (check(p, TOK_LPAREN)) {
        parse_parameters(p, func_sym);
    }

    // Parse return type
    expect(p, TOK_COLON);
    Type *return_type = parse_type(p);
    func_sym->type = return_type;

    expect(p, TOK_SEMICOLON);

    // Check for forward declaration
    if (check(p, TOK_FORWARD)) {
        advance(p);
        expect(p, TOK_SEMICOLON);
        free(name);
        return;
    }

    // Emit function code
    emit_raw("");
    emit_raw("// Function %s", name);
    emit_label(func_sym->label);

    // Enter new scope
    symtab_enter_scope(&p->symbols);

    // Reserve offset -8 for static link (local_offset is positive, negated for actual offset)
    p->symbols.current->local_offset = 8;

    // Add function name as local variable for return value
    Symbol *result_var = symtab_add(&p->symbols, name, SYM_VAR, return_type);

    // Save current function
    Symbol *saved_func = current_function;
    current_function = func_sym;

    // Add parameters to scope
    int param_idx = 0;
    for (Symbol *param = func_sym->params; param; param = param->next) {
        symtab_add(&p->symbols, param->name, SYM_PARAM, param->type);
        param_idx++;
    }

    // Parse const and var declarations
    while (check(p, TOK_CONST) || check(p, TOK_VAR)) {
        if (check(p, TOK_CONST)) {
            parse_const_declarations(p);
        } else if (check(p, TOK_VAR)) {
            parse_var_declarations(p);
        }
    }

    // If there are nested procedures/functions, emit a jump over them
    int body_label = -1;
    if (check(p, TOK_PROCEDURE) || check(p, TOK_FUNCTION)) {
        body_label = new_label();
        emit("b L%d", body_label);
    }

    // Parse nested procedure/function declarations
    while (check(p, TOK_PROCEDURE) || check(p, TOK_FUNCTION)) {
        if (check(p, TOK_PROCEDURE)) {
            parse_procedure_declaration(p);
        } else if (check(p, TOK_FUNCTION)) {
            parse_function_declaration(p);
        }
    }

    // Emit label for body if we jumped over nested procs/funcs
    if (body_label >= 0) {
        emit_label(body_label);
    }

    int local_size = p->symbols.current->local_offset;
    emit_prologue_proc(local_size);

    // Copy parameters from registers to stack
    param_idx = 0;
    for (Symbol *param = func_sym->params; param; param = param->next) {
        Symbol *local = symtab_lookup(&p->symbols, param->name);
        if (local && param_idx < 8) {
            // Move parameter from register xN to x0, then store
            if (param_idx != 0) {
                emit("mov x0, x%d", param_idx);
            }
            emit_store_fp(local->offset);
        }
        param_idx++;
    }

    // Parse begin..end
    expect(p, TOK_BEGIN);
    while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
        parse_statement(p);
        if (!check(p, TOK_END)) {
            expect(p, TOK_SEMICOLON);
        }
    }
    expect(p, TOK_END);

    // Load return value into x0
    emit_load_fp(result_var->offset);

    emit_epilogue(local_size);

    // Restore current function
    current_function = saved_func;

    // Leave scope
    symtab_leave_scope(&p->symbols);
    func_sym->defined = 1;

    expect(p, TOK_SEMICOLON);
    free(name);
}

// Parse block (is_main controls whether to emit epilogue or just cleanup)
static void parse_block_impl(Parser *p, int is_main) {
    // First pass: parse all declarations to collect symbols
    // We need to emit a jump over procedure/function code
    int body_label = -1;
    int has_procs = 0;

    // Parse declarations - allow interleaved const, var, procedure, function
    while (check(p, TOK_CONST) || check(p, TOK_VAR) ||
           check(p, TOK_PROCEDURE) || check(p, TOK_FUNCTION)) {
        if (check(p, TOK_CONST)) {
            parse_const_declarations(p);
        } else if (check(p, TOK_VAR)) {
            parse_var_declarations(p);
        } else if (check(p, TOK_PROCEDURE) || check(p, TOK_FUNCTION)) {
            // Emit jump over procs/funcs on first encounter
            if (!has_procs) {
                body_label = new_label();
                emit("b L%d", body_label);
                has_procs = 1;
            }
            if (check(p, TOK_PROCEDURE)) {
                parse_procedure_declaration(p);
            } else {
                parse_function_declaration(p);
            }
        }
    }

    // Emit label for body if we jumped over procs/funcs
    if (body_label >= 0) {
        emit_label(body_label);
    }

    int local_size = p->symbols.current->local_offset;

    emit_prologue(local_size);

    expect(p, TOK_BEGIN);
    while (!check(p, TOK_END) && !check(p, TOK_EOF)) {
        parse_statement(p);
        if (!check(p, TOK_END)) {
            expect(p, TOK_SEMICOLON);
        }
    }
    expect(p, TOK_END);

    if (is_main) {
        // For main, just restore stack but don't return
        if (local_size > 0) {
            int aligned = (local_size + 15) & ~15;
            emit_add_sp(aligned);
        }
        // Exit will be emitted by caller
    } else {
        emit_epilogue(local_size);
    }
}

// Compile Pascal source to assembly file
int parser_compile(Parser *p, const char *output_path) {
    out = fopen(output_path, "w");
    if (!out) {
        perror("Cannot create output file");
        return -1;
    }

    // Header
    emit_raw(".global _main");
    emit_raw(".align 4");
    emit_raw("");

    // Jump to main
    int main_label = new_label();
    emit_raw("_main:");
    emit("b L%d", main_label);

    // Runtime routines
    rt_print_int = emit_print_int_routine();
    rt_newline = emit_newline_routine();
    rt_readchar = emit_readchar_routine();
    rt_print_char = emit_print_char_routine();

    // Main program
    emit_raw("\n// Main program");
    emit_label(main_label);

    // Parse program header
    expect(p, TOK_PROGRAM);
    if (!check(p, TOK_IDENT)) {
        error(p, "expected program name");
    }
    char *program_name = current(p)->str_val;
    advance(p);
    free(program_name);

    if (match(p, TOK_LPAREN)) {
        while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
            if (check(p, TOK_IDENT)) {
                free(current(p)->str_val);
                advance(p);
            }
            if (!check(p, TOK_RPAREN)) {
                expect(p, TOK_COMMA);
            }
        }
        expect(p, TOK_RPAREN);
    }
    expect(p, TOK_SEMICOLON);

    // Parse main block (is_main=1 so it doesn't emit return)
    parse_block_impl(p, 1);
    expect(p, TOK_DOT);

    // Exit
    emit("mov x0, #0");
    emit_exit_syscall();

    // Data section
    if (num_strings > 0) {
        emit_raw("");
        emit_raw(".data");
        for (int i = 0; i < num_strings; i++) {
            fprintf(out, "str%d: .ascii \"", i);
            for (int j = 0; j < strings[i].len; j++) {
                char c = strings[i].str[j];
                if (c == '\n') fprintf(out, "\\n");
                else if (c == '\t') fprintf(out, "\\t");
                else if (c == '\\') fprintf(out, "\\\\");
                else if (c == '"') fprintf(out, "\\\"");
                else fputc(c, out);
            }
            fprintf(out, "\"\n");
            free(strings[i].str);
        }
    }

    fclose(out);
    return p->error_count == 0 ? 0 : -1;
}
