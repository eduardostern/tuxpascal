# TuxPascal Developer Documentation

## Overview

TuxPascal is a self-hosting Pascal compiler targeting ARM64 macOS. This document describes the compiler internals for developers who want to understand, modify, or extend the compiler.

## Architecture

TuxPascal is a **single-pass recursive descent compiler** that directly emits ARM64 assembly. There is no intermediate representation (IR) or abstract syntax tree (AST).

```
Source Code → Lexer → Parser/CodeGen → ARM64 Assembly → clang → Executable
                              ↓
                         Symbol Table
```

### Design Philosophy

1. **Simplicity**: Single-pass compilation like classic Turbo Pascal
2. **Self-hosting**: The compiler compiles itself
3. **Minimal dependencies**: Only needs clang for assembling/linking
4. **Readable output**: Assembly is emitted as human-readable strings

## Directory Structure

```
tuxpascal/
├── bootstrap/              # C bootstrap compiler
│   ├── main.c              # Entry point, invokes clang
│   ├── lexer.c/h           # Tokenizer
│   ├── parser.c/h          # Parser + code generation
│   └── symbols.c/h         # Symbol table
│
├── compiler/               # Pascal self-hosting compiler
│   ├── tuxpascal.pas       # Generated single-file (for self-hosting)
│   ├── tuxpascal_modular.pas  # Entry point with includes
│   └── inc/                # Modular source files
│       ├── constants.inc   # Constants and global variables
│       ├── utility.inc     # Helper functions
│       ├── lexer.inc       # Tokenizer
│       ├── symbols.inc     # Symbol table management
│       ├── emitters.inc    # ARM64 code emission
│       ├── runtime.inc     # Runtime library code generation
│       ├── parser.inc      # Expression and statement parsing
│       ├── declarations.inc # Type/var/const/proc declarations
│       └── main.inc        # Main program entry
│
├── build/                  # Build output
│   ├── bin/tpc             # Compiler wrapper script
│   ├── bin/tuxpascal       # Raw compiler binary
│   └── bootstrap/          # Bootstrap compiler
│
├── docs/                   # Documentation
├── examples/               # Example programs
└── scripts/                # Build scripts
```

## Compilation Pipeline

### 1. Lexical Analysis (lexer.inc)

The lexer reads input character-by-character and produces tokens.

**Key Global Variables:**
```pascal
ch: Integer;              { Current character (or -1 for EOF) }
tok_type: Integer;        { Current token type (TOK_*) }
tok_str: Array[0..255];   { Token string (for identifiers/strings) }
tok_len: Integer;         { Token string length }
tok_int: Integer;         { Integer value (for TOK_INTEGER) }
line_num: Integer;        { Current line number }
```

**Token Types:**
```pascal
TOK_INTEGER = 1;    TOK_IDENT = 2;      TOK_STRING = 3;
TOK_PLUS = 4;       TOK_MINUS = 5;      TOK_STAR = 6;
TOK_SLASH = 7;      TOK_LPAREN = 8;     TOK_RPAREN = 9;
TOK_SEMI = 10;      TOK_COLON = 11;     TOK_COMMA = 12;
TOK_DOT = 13;       TOK_ASSIGN = 14;    TOK_EQ = 15;
{ ... and more for keywords }
```

**Key Procedures:**
- `NextChar`: Read next character from input
- `NextToken`: Scan and return next token
- `SkipWhitespace`: Skip spaces, tabs, newlines, and comments

### 2. Parsing (parser.inc, declarations.inc)

The parser uses recursive descent with the grammar embedded in procedure structure.

**Expression Parsing Hierarchy:**
```
ParseExpression     { handles OR }
  └── ParseAndExpr  { handles AND }
       └── ParseRelExpr  { handles =, <>, <, >, <=, >= }
            └── ParseAddExpr  { handles +, - }
                 └── ParseMulExpr  { handles *, /, DIV, MOD }
                      └── ParseUnary  { handles NOT, -, @ }
                           └── ParseFactor  { handles literals, vars, calls }
```

**Statement Parsing:**
```pascal
Procedure ParseStatement;
{ Handles:
  - Assignment (identifier := expr)
  - Procedure calls
  - If-Then-Else
  - While-Do
  - Repeat-Until
  - For-To/DownTo-Do
  - Case-Of
  - With-Do
  - Begin-End blocks
}
```

### 3. Symbol Table (symbols.inc)

Symbols are stored in parallel arrays (no records for bootstrap compatibility):

```pascal
Const
  MAX_SYMBOLS = 500;
  MAX_NAME_LEN = 32;

Var
  sym_name: Array[0..15999] Of Integer;  { Flattened: 500 * 32 chars }
  sym_type: Array[0..499] Of Integer;    { TYPE_INTEGER, TYPE_REAL, etc. }
  sym_kind: Array[0..499] Of Integer;    { SYM_VAR, SYM_CONST, SYM_PROC, etc. }
  sym_offset: Array[0..499] Of Integer;  { Stack offset for locals }
  sym_level: Array[0..499] Of Integer;   { Scope nesting level }
  sym_label: Array[0..499] Of Integer;   { Label number for procs/funcs }
  sym_count: Integer;                    { Total symbols }
  scope_level: Integer;                  { Current nesting depth }
```

**Symbol Kinds:**
```pascal
SYM_VAR = 1;      { Variable }
SYM_CONST = 2;    { Constant }
SYM_TYPE = 3;     { Type definition }
SYM_PROC = 4;     { Procedure }
SYM_FUNC = 5;     { Function }
SYM_PARAM = 6;    { Parameter }
```

**Type Codes:**
```pascal
TYPE_INTEGER = 1;
TYPE_CHAR = 2;
TYPE_BOOLEAN = 3;
TYPE_STRING = 4;
TYPE_REAL = 5;
TYPE_ARRAY = 6;
TYPE_RECORD = 7;
TYPE_POINTER = 8;
TYPE_SET = 9;
TYPE_ENUM = 10;
TYPE_SUBRANGE = 11;
TYPE_TEXT = 12;
```

### 4. Code Generation (emitters.inc)

Code is emitted directly as ARM64 assembly strings.

**Calling Convention:**
- `x0-x7`: Arguments and return value
- `x9`: Static link for nested procedures
- `x19`: stdin file descriptor
- `x20`: stdout file descriptor
- `x21`: Heap pointer
- `x25`: argc
- `x26`: argv
- `x29`: Frame pointer
- `x30`: Link register (return address)
- `sp`: Stack pointer

**Stack Frame Layout:**
```
High addresses
┌─────────────────┐
│   Saved x30     │  [x29, #8]
├─────────────────┤
│   Saved x29     │  [x29, #0]  ← x29 (frame pointer)
├─────────────────┤
│   Static link   │  [x29, #-8]
├─────────────────┤
│   Local var 1   │  [x29, #-16]
├─────────────────┤
│   Local var 2   │  [x29, #-24]
├─────────────────┤
│      ...        │
└─────────────────┘  ← sp (stack pointer)
Low addresses
```

**Expression Evaluation:**

Expressions use a stack-based evaluation model:
1. Push operands onto stack
2. Pop operands, compute, push result

```pascal
{ Evaluating: a + b * c }
{ 1. Load a into x0 }
EmitLdurX0(offset_a);
{ 2. Push x0 }
EmitPushX0;
{ 3. Load b into x0 }
EmitLdurX0(offset_b);
{ 4. Push x0 }
EmitPushX0;
{ 5. Load c into x0 }
EmitLdurX0(offset_c);
{ 6. Pop b into x1 }
EmitPopX1;
{ 7. Multiply: x0 = x1 * x0 }
EmitMul;
{ 8. Pop a into x1 }
EmitPopX1;
{ 9. Add: x0 = x1 + x0 }
EmitAdd;
{ Result in x0 }
```

**Key Emitter Procedures:**
```pascal
{ Stack operations }
Procedure EmitPushX0;    { str x0, [sp, #-16]! }
Procedure EmitPopX0;     { ldr x0, [sp], #16 }
Procedure EmitPopX1;     { ldr x1, [sp], #16 }

{ Arithmetic }
Procedure EmitAdd;       { add x0, x1, x0 }
Procedure EmitSub;       { sub x0, x1, x0 }
Procedure EmitMul;       { mul x0, x1, x0 }
Procedure EmitSDiv;      { sdiv x0, x1, x0 }

{ Memory }
Procedure EmitLdurX0(offset: Integer);   { ldur x0, [x29, #offset] }
Procedure EmitSturX0(offset: Integer);   { stur x0, [x29, #offset] }

{ Control flow }
Procedure EmitLabel(n: Integer);         { Ln: }
Procedure EmitBranchLabel(n: Integer);   { b Ln }
Procedure EmitBranchLabelZ(n: Integer);  { cbz x0, Ln }
Procedure EmitBL(n: Integer);            { bl Ln }
```

### 5. Runtime Library (runtime.inc)

The runtime provides built-in procedures emitted inline:

- **I/O**: Print integers, strings, characters; read input
- **String operations**: Length, copy, concatenate
- **Math**: Trigonometry, sqrt, random
- **Memory**: Heap allocation (simple bump allocator)
- **Screen**: ANSI escape sequences for cursor/color control

Each runtime routine is assigned a label (`rt_print_int`, `rt_readln`, etc.) and emitted at program start.

## Adding New Features

### Adding a New Built-in Function

1. **Reserve a runtime label** in `constants.inc`:
   ```pascal
   rt_my_func: Integer;
   ```

2. **Initialize the label** in `main.inc`:
   ```pascal
   rt_my_func := NewLabel;
   ```

3. **Add recognition** in `parser.inc` (in ParseFactor):
   ```pascal
   Else If TokIs8(109, 121, 102, 117, 110, 99, 0, 0) = 1 Then  { myfunc }
   Begin
     NextToken;
     Expect(TOK_LPAREN);
     ParseExpression;  { argument in x0 }
     Expect(TOK_RPAREN);
     EmitBL(rt_my_func);
     expr_type := TYPE_INTEGER
   End
   ```

4. **Emit the runtime code** in `runtime.inc`:
   ```pascal
   Procedure EmitMyFuncRuntime;
   Begin
     EmitLabel(rt_my_func);
     EmitStp;
     EmitMovFP;
     { ... implementation ... }
     EmitLdp;
     EmitRet
   End;
   ```

5. **Call the emitter** in `main.inc`:
   ```pascal
   EmitMyFuncRuntime;
   ```

### Adding a New Statement

1. **Add token type** if needed in `constants.inc`:
   ```pascal
   TOK_MYSTATEMENT = 100;
   ```

2. **Add keyword recognition** in `lexer.inc`:
   ```pascal
   Else If TokIs8(...) = 1 Then tok_type := TOK_MYSTATEMENT
   ```

3. **Add parsing** in `parser.inc` (in ParseStatement):
   ```pascal
   Else If tok_type = TOK_MYSTATEMENT Then
   Begin
     NextToken;
     { parse and emit code }
   End
   ```

### Adding a New Type

1. **Add type code** in `constants.inc`:
   ```pascal
   TYPE_MYTYPE = 20;
   ```

2. **Add parsing** in `declarations.inc` (in ParseType)

3. **Handle in expressions** in `parser.inc`

4. **Handle in assignments** in `parser.inc`

## Debugging

### Compiler Debugging

The `Error` procedure outputs diagnostic information:
```pascal
Error: Undefined identifier 'xyz' at line 42
```

### Generated Code Debugging

Use `-S` to inspect generated assembly:
```bash
tpc -S program.pas
cat program.s
```

### Self-Hosting Verification

```bash
make self-host
```

This compiles the compiler multiple times:
1. Bootstrap (C) → v2 (Pascal binary)
2. v2 → v3 (assembly)
3. v3 → v4 (assembly)
4. Compare v3 and v4 (must be identical)

## Code Style Guidelines

### Pascal Source

- Use proper casing: `Begin`, `End`, `WriteLn`, `Integer`
- Indent with 2 spaces
- One statement per line
- Comments in `{ }` style

### Assembly Output

- 4-space indentation for instructions
- Labels at column 0
- Comments with `; `

## Testing Changes

1. **Build the compiler:**
   ```bash
   make
   ```

2. **Test with examples:**
   ```bash
   make test
   ```

3. **Verify self-hosting:**
   ```bash
   make self-host
   ```

4. **Test specific features:**
   ```bash
   echo "Program Test; Begin WriteLn('hello') End." | ./build/bin/tuxpascal > /tmp/test.s
   clang /tmp/test.s -o /tmp/test && /tmp/test
   ```

## Bootstrap Compiler

The C bootstrap (`bootstrap/`) is a minimal implementation sufficient to compile the Pascal compiler. It's intentionally kept simple and is rarely modified.

**Key differences from Pascal compiler:**
- No file I/O in compiled programs
- No `read`/`readln` for programs (only stdin for source)
- Limited error messages
- String table limited to 4096 entries

## Memory Layout

```
┌─────────────────────────────────────┐
│           Stack (grows down)         │
│  - Local variables                   │
│  - Saved registers                   │
│  - Procedure arguments               │
├─────────────────────────────────────┤
│           Heap (grows up)            │
│  - New() allocations                 │
│  - String buffers                    │
│  x21 points to next free byte        │
├─────────────────────────────────────┤
│           Code                       │
│  - Runtime routines                  │
│  - Program code                      │
├─────────────────────────────────────┤
│           Data                       │
│  - String literals                   │
└─────────────────────────────────────┘
```

## System Calls

TuxPascal uses macOS ARM64 system calls:

| Call | Number | Description |
|------|--------|-------------|
| exit | 0x2000001 | Exit program |
| read | 0x2000003 | Read from fd |
| write | 0x2000004 | Write to fd |
| open | 0x2000005 | Open file |
| close | 0x2000006 | Close file |
| lseek | 0x20000C7 | Seek in file |

## Contributing

1. Fork the repository
2. Make changes to `compiler/inc/*.inc`
3. Run `make self-host` to verify
4. Submit a pull request

### Code Review Checklist

- [ ] Self-hosting passes
- [ ] No regressions in examples
- [ ] Proper Pascal casing
- [ ] Comments for complex code
- [ ] Error messages are clear
