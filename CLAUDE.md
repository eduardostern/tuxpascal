# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TuxPascal is a minimal Pascal compiler written in C, targeting ARM64 macOS. It compiles a subset of Turbo Pascal 1.0-style Pascal to native executables via assembly output.

## Build Commands

```bash
make        # Build the compiler (produces ./tpc)
make clean  # Remove build artifacts
make test   # Build and run all example programs
```

## Running the Compiler

```bash
./tpc <input.pas> [-o <output>] [-S]
```

Options:
- `-o <file>` - Output file name (default: a.out)
- `-S` - Output assembly only (don't assemble/link)

Example:
```bash
./tpc examples/hello.pas -o hello
./hello
```

## Architecture

The compiler is a single-pass recursive descent compiler that outputs ARM64 assembly, then uses clang to assemble/link:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

### Source Files

- `src/main.c` - Entry point, file I/O, invokes clang for assembly/linking
- `src/lexer.c/h` - Tokenizer for Pascal source (case-insensitive keywords)
- `src/parser.c/h` - Recursive descent parser with inline code generation
- `src/symbols.c/h` - Symbol table with scope management

### Code Generation

The parser emits ARM64 assembly directly to a file as it parses. Key patterns:
- Stack-based expression evaluation (push/pop via `str`/`ldr` with pre/post-indexing)
- Frame pointer (x29) based local variable access
- macOS syscalls for I/O (write=0x2000004, exit=0x2000001)
- Runtime routines for integer printing and newline output

## Supported Pascal Features

**Types:** `integer`, `char`, `boolean`, `string`, `array`

**Statements:** `:=`, `if`/`then`/`else`, `while`/`do`, `repeat`/`until`, `for`/`to`/`downto`, `begin`/`end`

**Declarations:** `program`, `const`, `var`

**Operators:** `+`, `-`, `*`, `div`, `mod`, `=`, `<>`, `<`, `>`, `<=`, `>=`, `and`, `or`, `not`

**Built-ins:** `write`, `writeln` (strings and integers)

## Not Yet Implemented

- Procedures and functions
- `read`/`readln`
- Nested scopes
- Real numbers
- Pointers and records
- String operations beyond literals
