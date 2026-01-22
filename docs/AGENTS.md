# AGENTS.md

This file provides guidance for AI agents working on the TuxPascal compiler codebase.

## Build Commands

```bash
make              # Build the compiler (produces ./tpc)
make clean        # Remove build artifacts (obj/, tpc, *.s files)
make test         # Build and run all example programs (hello, factorial, fizzbuzz)
```

**Compiler Usage:**
```bash
./tpc <input.pas> [-o <output>] [-S]
./tpc examples/hello.pas -o hello    # Compile to executable
./tpc examples/hello.pas -S hello.s  # Output assembly only
```

**Single Test Run:**
```bash
./tpc examples/hello.pas -o hello && ./hello
./tpc examples/factorial.pas -o factorial && ./factorial
./tpc examples/fizzbuzz.pas -o fizzbuzz && ./fizzbuzz
```

## Code Style Guidelines

### C Standard
- Use **C99** (`-std=c99`) for all source files
- Use `int64_t`, `int32_t`, `uint64_t`, etc. from `<stdint.h>` for fixed-width types
- Use `bool` from `<stdbool.h>` for boolean values

### Naming Conventions
- **Functions**: `snake_case` (e.g., `lexer_init`, `parse_expression`, `type_size`)
- **Variables**: `snake_case` (e.g., `current_level`, `error_count`)
- **Constants/Enums**: `SCREAMING_SNAKE_CASE` (e.g., `TOK_INTEGER`, `TYPE_ARRAY`)
- **Typedefs**: Single-word or underscore-separated, **no `typedef struct` tag prefix** (e.g., `typedef struct Type Type;` pattern is not used - structs use `struct Type` or typedef to same name)
- **Macros**: `SCREAMING_SNAKE_CASE` for constants, `snake_case` for macro functions

### File Structure
- Header files (`*.h`) at top of .c files:
  ```c
  #include "parser.h"   // Project headers first
  #include <stdio.h>    // Then system headers
  #include <stdlib.h>
  ```
- One public header per module (lexer.h, parser.h, symbols.h)
- Static helper functions declared at top of .c file, defined where used
- Forward declarations for static functions needed before definition

### Functions
- Keep functions focused and reasonably sized (< 200 lines preferred)
- Static helper functions for internal implementation
- Public API functions declared in header, prefixed with module name (`lexer_*`, `parser_*`, `symtab_*`)
- Use `(void)param` pattern to suppress unused parameter warnings

### Formatting
- 4-space indentation (no tabs)
- Opening brace on same line as control statement:
  ```c
  if (condition) {
      // code
  } else {
      // code
  }
  ```
- Space around binary operators: `a + b`, not `a+b`
- No space between function name and parentheses: `foo()` not `foo ()`
- Pointer asterisks adjacent to type: `Type *type` not `Type* type`

### Error Handling
- Use `fprintf(stderr, ...)` for errors with location info when possible
- Call `exit(1)` on unrecoverable errors in lexer/parser
- Return error codes (`-1` or `NULL`) from functions that can fail gracefully
- Check all allocations: `if (!buf) { fclose(f); return NULL; }`
- Check all file operations: `if (!f) { perror(filename); return NULL; }`

### Symbol Table & Types
- Use `SymbolTable` and `Scope` structs for scope management
- Use `Type` struct for type information (arrays, primitives)
- Use `SymbolKind` enum for symbol classification (VAR, CONST, PROCEDURE, FUNCTION, PARAM)
- Use `TypeKind` enum for type classification (INTEGER, CHAR, BOOLEAN, STRING, ARRAY, VOID)

### Code Generation (ARM64 Assembly)
- Emit assembly via `emit()`, `emit_label()`, `emit_raw()` helper functions
- Emit 4-space indentation for instructions: `    mov x0, #0`
- Use x0 for return values, x1-x7 for arguments, x8-x30 as temporaries
- Frame pointer in x29, link register in x30
- Static link for nested scopes stored at `[x29, #-8]`
- macOS syscalls: write=0x2000004, exit=0x2000001

### Memory Management
- Use `malloc`/`free` for dynamic allocations
- Use `strdup` for string copies (includes allocation)
- Always free strings from lexer after use: `free(name)`
- Free allocated types and symbols (currently omitted for simplicity in some places)

### Comments
- Avoid comments unless explaining complex logic (e.g., ARM64 assembly tricks)
- Use `//` for single-line comments (C99 style)
- Use `// Section headers` for grouping related functions:
  ```c
  // Parse expression
  static void parse_expression(Parser *p) { ... }
  ```

## Architecture Overview

The compiler is a **single-pass recursive descent compiler** that outputs ARM64 macOS assembly:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

### Modules
- `src/main.c`: Entry point, file I/O, invokes clang for assembly/linking
- `src/lexer.c/h`: Tokenizer (case-insensitive keywords, Pascal comments)
- `src/parser.c/h`: Recursive descent parser with inline code generation
- `src/symbols.c/h`: Symbol table with scope management and type system

### Key Patterns
- Stack-based expression evaluation (push/pop via `str`/`ldr` with pre/post-indexing)
- Frame pointer (x29) based local variable access with negative offsets
- String literals emitted to `.data` section with `adrp`/`add` addressing
- Runtime routines (print_int, newline, readchar, print_char) emitted before main

## Unsupported Features (Do Not Implement)

- Procedures and functions (already implemented in parser.c)
- `read`/`readln` builtins
- Nested scopes (partially implemented)
- Real/floating-point numbers
- Pointers and records
- String operations beyond literals (concatenation, slicing)
