# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TuxPascal is a minimal Pascal compiler targeting ARM64 macOS. It compiles a subset of Turbo Pascal 1.0-style Pascal to native executables via assembly output.

There are two compiler implementations:
- **Bootstrap (`bootstrap/`)** - C compiler used to build the Pascal compiler. Frozen - only modify if needed to compile the main compiler.
- **Compiler (`compiler/`)** - Self-hosting compiler written in Pascal. **All new features go here.**

The Pascal compiler can compile itself (self-hosting verification: v2 → v3 → v4 produces identical output).

## Directory Structure

```
tuxpascal/
├── bootstrap/           # C bootstrap compiler (frozen)
│   ├── main.c           # Entry point, file I/O, invokes clang
│   ├── lexer.c/h        # Tokenizer for Pascal source
│   ├── parser.c/h       # Recursive descent parser with code gen
│   └── symbols.c/h      # Symbol table with scope management
│
├── compiler/            # Pascal self-hosting compiler (active development)
│   ├── tuxpascal.pas    # Generated single-file (for self-hosting)
│   ├── tuxpascal_modular.pas  # Entry point with includes
│   └── inc/             # Modular source files
│       ├── constants.inc
│       ├── utility.inc
│       ├── lexer.inc
│       ├── symbols.inc
│       ├── emitters.inc
│       ├── runtime.inc
│       ├── parser.inc
│       ├── declarations.inc
│       └── main.inc
│
├── examples/            # Example Pascal programs
├── build/               # Build output (gitignored)
├── scripts/             # Build and install scripts
├── docs/                # Additional documentation
└── Makefile
```

## Build Commands

```bash
make              # Build the Pascal compiler (default)
make bootstrap    # Build only the C bootstrap compiler
make test         # Build and run example programs
make self-host    # Verify self-hosting capability
make install      # Install to /usr/local/bin
make clean        # Remove build artifacts
make help         # Show all targets
```

## Running the Compilers

**Pascal compiler (after building):**
```bash
./build/bin/tpc <input.pas> [-o <output>] [-S] [-c] [-I<path>]
```

The `tpc` wrapper provides a clang-like CLI. The raw compiler (`tuxpascal`) is a stdin/stdout filter.

Options:
- `-o <file>` - Output file name
- `-S` - Output assembly only (.s)
- `-c` - Compile only, don't link (.o)
- `-I<path>` - Add directory to unit search path

**Bootstrap compiler (for development):**
```bash
./build/bootstrap/tpc <input.pas> [-o <output>] [-S]
```

## Rebuilding After Changes

After modifying any include files in `compiler/inc/`:

```bash
# Rebuild the compiler
make

# Or manually rebuild using bootstrap
./build/bootstrap/tpc compiler/tuxpascal_modular.pas -o build/bin/tuxpascal
```

To regenerate the single-file `tuxpascal.pas` for self-hosting verification:
```bash
./scripts/merge-compiler.sh
```

To verify self-hosting:
```bash
make self-host
```

## Architecture

Both compilers are single-pass recursive descent compilers that output ARM64 assembly:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

### Code Generation

The parser emits ARM64 assembly directly as it parses. Key patterns:
- Stack-based expression evaluation (push/pop via `str`/`ldr` with pre/post-indexing)
- Frame pointer (x29) based local variable access
- macOS syscalls for I/O (read=0x2000003, write=0x2000004, exit=0x2000001)
- Runtime routines for integer printing and newline output

## Supported Pascal Features

**Types:** `integer`, `char`, `boolean`, `string`, `array`, `real`, `^type` (pointers), `^^type` (pointers to pointers), `^array[lo..hi] of T` (pointers to arrays), `record` (including nested and variant), `text` (files), `set of` (64-bit bitmask), enumerated types `(val1, val2, ...)`, subrange types `lo..hi`

**Statements:** `:=`, `if`/`then`/`else`, `while`/`do`, `repeat`/`until`, `for`/`to`/`downto`, `case`/`of`, `with`/`do`, `begin`/`end`, `break`, `continue`, `exit`

**Declarations:** `program`, `const`, `type`, `var`, `procedure`, `function`, `forward`

**Operators:** `+`, `-`, `*`, `/`, `div`, `mod`, `=`, `<>`, `<`, `>`, `<=`, `>=`, `and` (short-circuit), `or` (short-circuit), `not`, `@` (address-of), `^` (dereference), pointer arithmetic, `in` (set membership), set operators (`+` union, `-` difference, `*` intersection)

**Built-ins:** `write`, `writeln`, `read`, `readln`, `readchar`, `writechar`, `new`, `dispose`, `nil`, `halt`, `sizeof`, `paramcount`, `paramstr`

**File I/O:** `assign`, `reset`, `rewrite`, `close`, `eof`, `seek`, `filepos`, `filesize`

**String Functions:** `length`, `copy`, `concat`, `+` (concatenation), `pos`, `delete`, `insert`, `str`, `val`, `trim`, `ltrim`, `rtrim`

**Math Functions:** `sin`, `cos`, `tan`, `arctan`, `arcsin`, `arccos`, `sqrt`, `sqr`, `exp`, `ln`, `log10`, `log2`, `power`, `abs`, `round`, `trunc`, `frac`, `int`, `pi`, `random`, `randomize`

**Utility Functions:** `odd`, `succ`, `pred`, `inc`, `dec`, `upcase`, `lowercase`, `ord`, `chr`

**Procedures/Functions:** Parameters, local variables, nested scopes with static link chain, forward declarations

**Include Directives:** `{$I filename}` or `{$INCLUDE filename}` - nested includes up to 8 levels

**Units:** `unit`, `interface`, `implementation`, `uses` - full unit compilation and linking

## Unit Compilation

Units are compiled separately and linked with programs that use them:

```bash
# Compile a unit (generates .o and .tpu)
tpc -c myunit.pas

# Compile a program that uses the unit (auto-links)
tpc myprogram.pas -o myprogram
```

When compiling a unit, the compiler generates:
- Assembly/object code with unit-prefixed labels (`_UnitName_ProcName`)
- A `.tpu` file containing interface metadata (lowercase filename)

### TPU File Format

```
TUXPASCAL_UNIT_V1
UNIT UnitName
INTERFACE
CONST name TYPE value
VAR name TYPE offset
PROC name label var_param_flags
FUNC name TYPE label var_param_flags
TYPE name kind const_val label
ENDINTERFACE
```

### Unit Workflow

1. Unit code emits `.globl _UnitName_ProcName` labels for exports
2. Unit initialization emits `_UnitName_init` entry point
3. Programs calling `uses MyUnit` load `myunit.tpu` for symbols
4. Program emits `bl _UnitName_ProcName` for unit calls
5. Program emits `bl _UnitName_init` at startup for each unit
6. The tpc wrapper auto-links unit `.o` files

### Build System Features

The `tpc` wrapper provides:
- **Auto-dependency tracking**: Recursively compiles unit dependencies
- **Timestamp checking**: Only recompiles units when source is newer than object
- **Include paths**: `-I<path>` flag adds directories to unit search path
- **Automatic linking**: Links all required `.o` files when building executables

Note: `read`/`readln`/`new`/`dispose`/file I/O are implemented in the Pascal compiler only, not in the bootstrap.
