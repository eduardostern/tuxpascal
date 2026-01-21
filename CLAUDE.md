# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TuxPascal is a minimal Pascal compiler targeting ARM64 macOS. It compiles a subset of Turbo Pascal 1.0-style Pascal to native executables via assembly output.

There are two compiler implementations:
- **v1 (`src/`)** - Bootstrap compiler written in C. Frozen - only modify if needed to compile v2.
- **v2 (`v2/compiler.pas`)** - Self-hosting compiler written in Pascal. **All new features go here.**

The v2 compiler can compile itself (v2 → v3 → v4 produces identical output).

## Build Commands

```bash
make        # Build v1 bootstrap compiler (produces ./tpc)
make clean  # Remove build artifacts
make test   # Build and run all example programs
```

## Running the Compilers

**v1 (bootstrap):**
```bash
./tpc <input.pas> [-o <output>] [-S]
```

**v2 (self-hosting):**
```bash
cat input.pas | ./v2/tpcv2 > output.s
clang output.s -o output
```

Options for v1:
- `-o <file>` - Output file name (default: a.out)
- `-S` - Output assembly only (don't assemble/link)

## Rebuilding v2

To rebuild the v2 compiler after making changes:
```bash
./tpc v2/compiler.pas -o v2/tpcv2
```

To verify self-hosting (v2 compiles itself):
```bash
cat v2/compiler.pas | ./v2/tpcv2 > /tmp/v3.s && clang /tmp/v3.s -o /tmp/v3
cat v2/compiler.pas | /tmp/v3 > /tmp/v4.s
diff /tmp/v3.s /tmp/v4.s  # Should be identical
```

## Architecture

Both compilers are single-pass recursive descent compilers that output ARM64 assembly:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

### v1 Source Files (C bootstrap - frozen)

- `src/main.c` - Entry point, file I/O, invokes clang for assembly/linking
- `src/lexer.c/h` - Tokenizer for Pascal source (case-insensitive keywords)
- `src/parser.c/h` - Recursive descent parser with inline code generation
- `src/symbols.c/h` - Symbol table with scope management

### v2 Source Files (Pascal - active development)

- `v2/compiler.pas` - Complete self-hosting compiler in a single file
- `v2/compiler_split.pas` - Modular version using include files
- `v2/inc/` - Include files for the modular compiler:
  - `constants.inc` - Token types, symbol kinds, type kinds, global variables
  - `utility.inc` - Helper functions (Error, IsDigit, IsAlpha, ToLower, etc.)
  - `lexer.inc` - Tokenizer (NextChar, NextToken, SkipWhitespace)
  - `symbols.inc` - Symbol table management
  - `emitters.inc` - Assembly output procedures (Emit*, WriteChar sequences)
  - `runtime.inc` - Runtime code generators (print routines, read routines)
  - `parser.inc` - Expression and statement parsing
  - `declarations.inc` - Procedure/function/block parsing
  - `main.inc` - Main initialization and entry point

### Code Generation

The parser emits ARM64 assembly directly to a file as it parses. Key patterns:
- Stack-based expression evaluation (push/pop via `str`/`ldr` with pre/post-indexing)
- Frame pointer (x29) based local variable access
- macOS syscalls for I/O (read=0x2000003, write=0x2000004, exit=0x2000001)
- Runtime routines for integer printing and newline output

## Supported Pascal Features

**Types:** `integer`, `char`, `boolean`, `string`, `array`

**Statements:** `:=`, `if`/`then`/`else`, `while`/`do`, `repeat`/`until`, `for`/`to`/`downto`, `begin`/`end`

**Declarations:** `program`, `const`, `var`, `procedure`, `function`, `forward`

**Operators:** `+`, `-`, `*`, `div`, `mod`, `=`, `<>`, `<`, `>`, `<=`, `>=`, `and`, `or`, `not`

**Built-ins:** `write`, `writeln`, `read`, `readln`, `readchar`, `writechar` (v2 only: `read`, `readln`, `readchar`, `writechar`)

**Procedures/Functions:** Parameters, local variables, nested scopes with static link chain, forward declarations

**Include Directives:** `{$I filename}` or `{$INCLUDE filename}` - processed by v1 preprocessor before compilation

## Not Yet Implemented

- Real numbers
- Pointers and records
- String operations beyond literals
- Case statement
- Unary minus in expressions

Note: `read`/`readln` are implemented in v2 only, not in the v1 bootstrap compiler.

## Include Directive

The v1 compiler supports Pascal-style include directives for splitting source files:

```pascal
{$I filename.inc}
{$INCLUDE path/to/file.inc}
```

Features:
- Paths are relative to the including file's directory
- Circular includes are detected and prevented
- Maximum include depth of 8 levels
- Works with both `{$I}` and `{$INCLUDE}` syntax

To compile the modular v2 compiler:
```bash
./tpc v2/compiler_split.pas -o v2/tpcv2
```
