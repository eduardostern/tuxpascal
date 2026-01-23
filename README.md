```
  ████████╗██╗   ██╗██╗  ██╗██████╗  █████╗ ███████╗ ██████╗ █████╗ ██╗
  ╚══██╔══╝██║   ██║╚██╗██╔╝██╔══██╗██╔══██╗██╔════╝██╔════╝██╔══██╗██║
     ██║   ██║   ██║ ╚███╔╝ ██████╔╝███████║███████╗██║     ███████║██║
     ██║   ██║   ██║ ██╔██╗ ██╔═══╝ ██╔══██║╚════██║██║     ██╔══██║██║
     ██║   ╚██████╔╝██╔╝ ██╗██║     ██║  ██║███████║╚██████╗██║  ██║███████╗
     ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝
```

<div align="center">

**The Pascal compiler Niklaus Wirth would run on his M4 Mac.**

*For the programmers who learned to code when monitors were green,*
*when "memory" meant 64KB, and when GOTO was considered harmful.*

[![Self-Hosting](https://img.shields.io/badge/Self--Hosting-Verified-brightgreen)]()
[![Platform](https://img.shields.io/badge/Platform-ARM64%20macOS-blue)]()
[![License](https://img.shields.io/badge/License-MIT-green)]()
[![Borland Spirit](https://img.shields.io/badge/Borland%20Spirit-100%25-red)]()

</div>

---

## Welcome Back, Pascal Programmer

Remember when compilers came on floppy disks? When Turbo Pascal's blue IDE was the most beautiful thing you'd ever seen? When Anders Hejlsberg showed the world that a compiler could be *fast*?

**TuxPascal brings that magic to Apple Silicon.**

No frameworks. No package managers. No 500MB of node_modules. Just you, your code, and a compiler that turns Pascal into native ARM64 executables faster than you can say "BEGIN...END."

```pascal
Program HelloNewWorld;
Begin
  WriteLn('The 80s called. They want their elegance back.')
End.
```

```bash
$ tpc hello.pas && ./hello
The 80s called. They want their elegance back.
```

---

## Why TuxPascal?

### Because Some of Us Remember

We remember when programming was *fun*. When you could understand the entire toolchain. When a "compile-run-debug" cycle took seconds, not minutes. When code was readable by humans, not just by linters.

### Because Pascal Was Right

Niklaus Wirth designed Pascal in 1970 with a radical idea: **programs should be correct**. Strong typing. Structured programming. No pointer arithmetic unless you really meant it. Fifty years later, modern languages are still catching up.

### Because Modern Doesn't Mean Bloated

TuxPascal is:
- **~280KB** native executable (smaller than most favicons)
- **Zero dependencies** (just needs clang for linking)
- **Single-pass compilation** (like Turbo Pascal, it's *fast*)
- **Self-hosting** (the compiler compiles itself - the ultimate flex)

---

## Features That Would Make Borland Proud

### Turbo Pascal 1.0 Compatibility
Write code like it's 1983. We support the classics:

```pascal
Program Classics;
Var
  i: Integer;
  name: String;
Begin
  Write('What is your name? ');
  ReadLn(name);
  For i := 1 To 3 Do
    WriteLn('Hello, ', name, '!');
  WriteLn('Press any key to continue...');
  ReadChar
End.
```

### Modern Extensions
But we didn't stop in 1983:

| Feature | Description |
|---------|-------------|
| **Units** | Modular compilation with `unit`/`uses` |
| **Records** | Including nested and variant records |
| **Pointers** | `^Type`, `^^Type`, pointer arithmetic |
| **Sets** | `set of` with full set operations |
| **Enums** | Enumerated and subrange types |
| **File I/O** | Text and typed files |
| **Reals** | IEEE 754 floating point |

### CRT Unit Nostalgia
Remember writing games in your high school computer lab?

```pascal
Program RetroGame;
Begin
  ClrScr;
  HideCursor;
  TextColor(14);  { Yellow, obviously }
  GotoXY(30, 12);
  Write('*** GAME OVER ***');
  TextColor(7);
  GotoXY(25, 14);
  Write('Insert coin to continue');
  ShowCursor
End.
```

All your favorites are here: `ClrScr`, `GotoXY`, `TextColor`, `TextBackground`, `KeyPressed`, `ReadKey`...

---

## Installation

```bash
# Clone the repository
git clone https://github.com/eduardostern/tuxpascal.git
cd tuxpascal

# Build (requires Xcode Command Line Tools)
make

# Optional: Install system-wide
sudo make install
```

That's it. No brew. No pip. No npm. No cargo. Just `make`.

---

## Usage

TuxPascal uses a clang-style command line interface:

```bash
# Compile to executable
tpc program.pas

# Compile to assembly (for the curious)
tpc -S program.pas

# Compile to object file (for linking)
tpc -c program.pas

# Specify output name
tpc program.pas -o myprogram
```

### Working with Units

```bash
# Compile a unit (creates .o and .tpu files)
tpc -c myunit.pas

# Compile a program that uses the unit (auto-links)
tpc myprogram.pas

# Add include path for units in another directory
tpc -I/path/to/units myprogram.pas
```

The build system automatically:
- Compiles unit dependencies recursively
- Tracks timestamps to avoid unnecessary recompilation
- Links all required `.o` files

---

## The Classics, Reimagined

### Towers of Hanoi
*With smooth ANSI animation, because we have standards*

```bash
tpc examples/hanoi.pas && ./examples/hanoi
```

```pascal
Program TowersOfHanoi;
Const
  NumDisks = 5;
  AnimDelay = 50;
Var
  towers: Array[1..3, 1..10] Of Integer;
Begin
  ClrScr;
  HideCursor;
  TextColor(11);
  GotoXY(28, 1);
  WriteLn('TOWERS OF HANOI');
  { Watch the disks dance across your terminal }
End.
```

### Tetris
*Fully playable, because what's a Pascal compiler without Tetris?*

```bash
tpc examples/tetris.pas && ./examples/tetris
```

```pascal
Program Tetris;
Begin
  ClrScr;
  HideCursor;
  InitKeyboard;  { Raw mode for real-time input }

  While Not GameOver Do
  Begin
    If KeyPressed Then
      HandleInput(ReadChar);
    UpdateGame;
    DrawBoard;
    Sleep(50)
  End;

  DoneKeyboard;
  ShowCursor
End.
```

**Controls:** Arrow keys or WASD, Space to drop, Q to quit.

---

## The Self-Hosting Story

TuxPascal is written in Pascal and compiles itself. Here's how the magic works:

```
Bootstrap (C) ──compiles──▶ v2 (Pascal)
     v2      ──compiles──▶ v3
     v3      ──compiles──▶ v4
     v4      ──compiles──▶ v5 (identical to v4) ✓
```

When v4 and v5 produce identical output, we know the compiler is correct. This is called **compiler bootstrapping**, and it's been the gold standard since the 1960s.

```bash
# Verify it yourself
make self-host
# => Self-hosting verified: v3 and v4 produce identical output
```

### The Size Paradox

| Compiler | Size | Compiled By |
|----------|------|-------------|
| v2 | 297,736 bytes | Bootstrap (C) |
| v3 | 281,024 bytes | v2 (Pascal) |
| v4 | 281,024 bytes | v3 (Pascal) |

The Pascal compiler produces smaller binaries than the C bootstrap! Why? The C compiler puts strings in a `.data` section (16KB page). The Pascal compiler writes strings character-by-character. Same output, smaller binary.

---

## Language Reference

### Types

```pascal
Var
  i: Integer;           { 64-bit signed }
  c: Char;              { 8-bit character }
  b: Boolean;           { True/False }
  s: String;            { 255-char max }
  r: Real;              { IEEE 754 double }
  a: Array[1..10] Of Integer;
  p: ^Integer;          { Pointer }
  pp: ^^Integer;        { Pointer to pointer }
  f: Text;              { Text file }
```

**Turbo Pascal type aliases:** `Byte`, `Word`, `LongInt`, `Int64`, `ShortInt`, `LongWord`, `Cardinal` → Integer; `Single`, `Double`, `Extended` → Real

### Records

```pascal
Type
  TPoint = Record
    x, y: Integer
  End;

  TShape = Record
    origin: TPoint;     { Nested record }
    Case kind: Integer Of
      1: (radius: Integer);           { Circle }
      2: (width, height: Integer)     { Rectangle }
  End;
```

### Units

```pascal
Unit MathUtils;

Interface
  Function Factorial(n: Integer): Integer;
  Function Fibonacci(n: Integer): Integer;

Implementation

Function Factorial(n: Integer): Integer;
Begin
  If n <= 1 Then
    Factorial := 1
  Else
    Factorial := n * Factorial(n - 1)
End;

Function Fibonacci(n: Integer): Integer;
Begin
  If n <= 2 Then
    Fibonacci := 1
  Else
    Fibonacci := Fibonacci(n-1) + Fibonacci(n-2)
End;

Begin
  { Unit initialization }
  WriteLn('MathUtils loaded')
End.
```

### Built-in Functions

| Category | Functions |
|----------|-----------|
| **I/O** | `Write`, `WriteLn`, `Read`, `ReadLn`, `ReadChar`, `WriteChar` |
| **Strings** | `Length`, `Copy`, `Concat`, `Pos`, `Delete`, `Insert`, `Str`, `Val`, `Trim` |
| **Math** | `Sin`, `Cos`, `Tan`, `ArcTan`, `Sqrt`, `Sqr`, `Exp`, `Ln`, `Abs`, `Round`, `Trunc` |
| **Utility** | `Ord`, `Chr`, `Succ`, `Pred`, `Inc`, `Dec`, `Odd`, `UpCase`, `LowerCase` |
| **Memory** | `New`, `Dispose`, `GetMem`, `FreeMem`, `FillChar`, `Move`, `SizeOf` |
| **Byte/Word** | `Hi`, `Lo`, `Swap`, `Assigned` |
| **System** | `Halt`, `ParamCount`, `ParamStr`, `Random`, `Randomize` |
| **Files** | `Assign`, `Reset`, `Rewrite`, `Close`, `Eof`, `Seek`, `FilePos`, `FileSize` |
| **Screen** | `ClrScr`, `GotoXY`, `ClrEol`, `TextColor`, `TextBackground`, `HideCursor`, `ShowCursor` |
| **Keyboard** | `KeyPressed`, `InitKeyboard`, `DoneKeyboard`, `Sleep` |

---

## Project Structure

```
tuxpascal/
├── bootstrap/           # C bootstrap compiler (frozen)
│   └── *.c, *.h         # Only touched to support new Pascal features
│
├── compiler/            # Pascal self-hosting compiler
│   ├── tuxpascal.pas    # Single-file version (generated)
│   └── inc/             # Modular source files
│       ├── lexer.inc    # Tokenizer
│       ├── parser.inc   # Recursive descent parser
│       ├── emitters.inc # ARM64 code generation
│       └── runtime.inc  # Built-in functions
│
├── examples/            # Classic Pascal programs
│   ├── hello.pas
│   ├── tetris.pas
│   ├── hanoi.pas
│   └── ...
│
└── build/               # Compiler binaries
    └── bin/tpc          # The compiler you'll use
```

---

## For the Curious: Architecture

TuxPascal is a **single-pass recursive descent compiler**. Like Turbo Pascal, it reads your source code once and emits machine code as it goes. No AST. No intermediate representation. Just pure, direct translation.

```
Source (.pas) → Lexer → Parser → ARM64 Assembly → clang → Executable
```

The parser is the code generator. When it sees `a := b + c`, it immediately emits:
```asm
    ldur x0, [x29, #-24]    ; load b
    str x0, [sp, #-16]!     ; push
    ldur x0, [x29, #-32]    ; load c
    ldr x1, [sp], #16       ; pop
    add x0, x1, x0          ; add
    stur x0, [x29, #-16]    ; store to a
```

No optimization passes. No register allocation. Just honest, predictable code generation. Anders would approve.

---

## Requirements

- **macOS on Apple Silicon** (ARM64)
- **Xcode Command Line Tools** (`xcode-select --install`)

That's literally it.

---

## Contributing

Found a bug? Want to add a feature? Remember that warm feeling when you submitted your first patch to a Borland newsgroup?

1. Fork the repository
2. Make your changes to `compiler/inc/*.inc`
3. Run `make self-host` to verify
4. Submit a pull request

---

## Built with Claude Code

This compiler was developed with significant assistance from [Claude Code](https://claude.ai/code), Anthropic's AI coding assistant. What started as a bootstrap compiler in C evolved into a fully self-hosting Pascal compiler through iterative pair programming sessions.

Claude helped with:
- Designing the single-pass recursive descent architecture
- Implementing ARM64 assembly code generation
- Adding features like records, pointers, sets, enums, and file I/O
- Building the unit system with TPU files and cross-unit linking
- Creating the build system with automatic dependency tracking
- Debugging the self-hosting verification process

The collaboration demonstrated that AI can be a powerful partner for systems programming - understanding low-level details like calling conventions, stack management, and assembly syntax while maintaining the high-level vision of recreating the Turbo Pascal experience.

*"Pair programming with an AI that knows both Pascal AND ARM64? Anders would have shipped Delphi a year early."*

---

## Dedication

*To Niklaus Wirth, who taught us that simplicity is the ultimate sophistication.*

*To Anders Hejlsberg, who proved that compilers could be both fast and friendly.*

*To Philippe Kahn, who made Borland the Microsoft-slayer of the 1980s.*

*And to every programmer who ever typed `BEGIN` and felt the world make sense.*

---

## License

**MIT License** - Do whatever you want, just keep the copyright notice.

Use it, sell it, modify it, make it closed source, we don't care. The only rule: include the license file so everyone knows Eduardo Stern wrote the original (and can't sue him for his own code).

*"Treat this compiler like a book"* - The Borland Way

---

<div align="center">

**TuxPascal** — *Because real programmers never forgot Pascal.*

```
╔════════════════════════════════════════════════════════════════╗
║  Compile Complete. 0 Errors, 0 Warnings, 0 Hints.              ║
║  Press any key to run... (where's the "any" key?)              ║
╚════════════════════════════════════════════════════════════════╝
```

</div>
