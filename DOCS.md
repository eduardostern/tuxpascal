# TuxPascal Documentation

**Author:** Eduardo Stern (eduardostern@icloud.com)
**Co-Author:** Claude Opus 4.5 (Anthropic)

A minimal Pascal compiler targeting ARM64 macOS, compiling a subset of Turbo Pascal 1.0-style Pascal to native executables.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Language Reference](#language-reference)
   - [Program Structure](#program-structure)
   - [Data Types](#data-types)
   - [Variables and Constants](#variables-and-constants)
   - [Operators](#operators)
   - [Control Structures](#control-structures)
   - [Procedures and Functions](#procedures-and-functions)
   - [Arrays](#arrays)
   - [Records](#records)
   - [Pointers](#pointers)
   - [Strings](#strings)
3. [Built-in Procedures and Functions](#built-in-procedures-and-functions)
   - [Input/Output](#inputoutput)
   - [String Functions](#string-functions)
   - [Utility Functions](#utility-functions)
   - [Screen/Terminal Control](#screenterminal-control)
4. [Compiler Directives](#compiler-directives)
5. [Examples](#examples)
6. [Compiler Architecture](#compiler-architecture)
7. [Building from Source](#building-from-source)

---

## Getting Started

### Requirements

- macOS on Apple Silicon (ARM64)
- Xcode Command Line Tools (`xcode-select --install`)

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/tuxpascal.git
cd tuxpascal

# Build the bootstrap compiler
make

# The v2 self-hosting compiler is ready at v2/tpcv2
```

### Your First Program

Create a file `hello.pas`:

```pascal
program Hello;
begin
  writeln('Hello, World!')
end.
```

Compile and run:

```bash
cat hello.pas | ./v2/tpcv2 > hello.s && clang hello.s -o hello
./hello
```

---

## Language Reference

### Program Structure

Every TuxPascal program follows this structure:

```pascal
program ProgramName;

const
  { constant declarations }

type
  { type declarations }

var
  { variable declarations }

{ procedure and function declarations }

begin
  { main program statements }
end.
```

### Data Types

| Type | Description | Size |
|------|-------------|------|
| `integer` | Signed 64-bit integer | 8 bytes |
| `char` | Single ASCII character | 8 bytes |
| `boolean` | `true` or `false` | 8 bytes |
| `real` | 64-bit floating point | 8 bytes |
| `string` | Pascal string (length byte + 255 chars) | 256 bytes |
| `array` | Fixed-size array | element_size × count |
| `record` | Structured data type | sum of field sizes |
| `^type` | Pointer to type | 8 bytes |

### Variables and Constants

**Constants:**
```pascal
const
  MaxSize = 100;
  Pi = 3.14159;
  Greeting = 'Hello';
```

**Variables:**
```pascal
var
  x, y, z: integer;
  name: string;
  values: array[1..10] of integer;
  flag: boolean;
```

### Operators

**Arithmetic:**
| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Real division |
| `div` | Integer division |
| `mod` | Modulo |

**Comparison:**
| Operator | Description |
|----------|-------------|
| `=` | Equal |
| `<>` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |

**Logical:**
| Operator | Description |
|----------|-------------|
| `and` | Logical AND |
| `or` | Logical OR |
| `not` | Logical NOT |

**Pointer:**
| Operator | Description |
|----------|-------------|
| `@` | Address-of |
| `^` | Dereference |

### Control Structures

**If-Then-Else:**
```pascal
if x > 0 then
  writeln('Positive')
else if x < 0 then
  writeln('Negative')
else
  writeln('Zero')
```

**While Loop:**
```pascal
while x > 0 do
begin
  writeln(x);
  x := x - 1
end
```

**Repeat-Until:**
```pascal
repeat
  writeln(x);
  x := x + 1
until x > 10
```

**For Loop:**
```pascal
for i := 1 to 10 do
  writeln(i);

for i := 10 downto 1 do
  writeln(i)
```

**Case Statement:**
```pascal
case choice of
  1: writeln('One');
  2: writeln('Two');
  3, 4, 5: writeln('Three to Five')
end
```

### Procedures and Functions

**Procedure:**
```pascal
procedure Greet(name: string);
begin
  write('Hello, ');
  writeln(name)
end;
```

**Function:**
```pascal
function Square(x: integer): integer;
begin
  Square := x * x
end;
```

**Var Parameters (Pass by Reference):**
```pascal
procedure Swap(var a, b: integer);
var temp: integer;
begin
  temp := a;
  a := b;
  b := temp
end;
```

**Forward Declarations:**
```pascal
procedure Later(x: integer); forward;

procedure Earlier;
begin
  Later(42)
end;

procedure Later(x: integer);
begin
  writeln(x)
end;
```

### Arrays

```pascal
var
  numbers: array[1..100] of integer;
  matrix: array[0..9] of integer;

begin
  numbers[1] := 42;
  for i := 0 to 9 do
    matrix[i] := i * i
end.
```

### Records

```pascal
type
  Point = record
    x: integer;
    y: integer
  end;

var
  p: Point;

begin
  p.x := 10;
  p.y := 20;
  writeln(p.x + p.y)
end.
```

**With Statement:**
```pascal
with p do
begin
  x := 10;
  y := 20
end
```

### Pointers

```pascal
var
  p: ^integer;
  x: integer;

begin
  x := 42;
  p := @x;      { p points to x }
  writeln(p^);  { prints 42 }
  p^ := 100;    { x is now 100 }

  new(p);       { allocate memory }
  p^ := 999;
  dispose(p)    { free memory (no-op in current implementation) }
end.
```

### Strings

TuxPascal strings are Pascal-style: a length byte followed by up to 255 characters.

```pascal
var
  s, t: string;

begin
  s := 'Hello';
  t := 'World';
  writeln(s + ', ' + t + '!');  { Hello, World! }
  writeln(length(s));            { 5 }
  writeln(copy(s, 1, 3));        { Hel }
end.
```

---

## Built-in Procedures and Functions

### Input/Output

| Procedure | Description |
|-----------|-------------|
| `write(...)` | Output values without newline |
| `writeln(...)` | Output values with newline |
| `read(var x)` | Read integer from input |
| `readln(var x)` | Read integer and skip to next line |
| `readln(var s: string)` | Read line into string |
| `readchar` | Read single character (returns integer) |
| `writechar(c)` | Write single character |

### String Functions

| Function | Description |
|----------|-------------|
| `length(s)` | Return length of string |
| `copy(s, start, count)` | Extract substring |
| `concat(s1, s2)` | Concatenate strings |
| `pos(substr, s)` | Find substring position (0 if not found) |
| `delete(var s, start, count)` | Delete characters from string |
| `insert(src, var dest, pos)` | Insert string into another |
| `str(n, var s)` | Convert integer to string |
| `val(s, var n, var code)` | Convert string to integer |
| `trim(s)` | Remove leading and trailing whitespace |
| `ltrim(s)` | Remove leading whitespace |
| `rtrim(s)` | Remove trailing whitespace |

### Utility Functions

| Function | Description |
|----------|-------------|
| `abs(x)` | Absolute value |
| `sqr(x)` | Square (x * x) |
| `odd(x)` | True if x is odd |
| `succ(x)` | Successor (x + 1) |
| `pred(x)` | Predecessor (x - 1) |
| `inc(var x)` | Increment x |
| `dec(var x)` | Decrement x |
| `ord(c)` | Character to ASCII code |
| `chr(n)` | ASCII code to character |
| `upcase(c)` | Convert to uppercase |
| `lowercase(c)` | Convert to lowercase |
| `halt` | Exit program immediately |

### Screen/Terminal Control

TuxPascal provides ANSI escape sequence-based terminal control for creating text-based user interfaces and games.

| Procedure | Description |
|-----------|-------------|
| `ClrScr` | Clear screen and move cursor to home |
| `GotoXY(x, y)` | Move cursor to column x, row y |
| `ClrEol` | Clear from cursor to end of line |
| `TextColor(c)` | Set foreground color (0-7) |
| `TextBackground(c)` | Set background color (0-7) |
| `NormVideo` | Reset text attributes to normal |
| `HighVideo` | Set bold/bright text |
| `LowVideo` | Set dim text |
| `HideCursor` | Hide the cursor |
| `ShowCursor` | Show the cursor |
| `Sleep(ms)` | Pause for ms milliseconds |

**Keyboard Input (for games):**

| Procedure/Function | Description |
|--------------------|-------------|
| `InitKeyboard` | Set terminal to raw mode (immediate key response) |
| `DoneKeyboard` | Restore terminal to normal mode |
| `KeyPressed` | Returns true if a key is available (non-blocking) |

**Color Values:**
| Value | Color |
|-------|-------|
| 0 | Black |
| 1 | Red |
| 2 | Green |
| 3 | Yellow/Cyan |
| 4 | Blue |
| 5 | Magenta |
| 6 | Cyan |
| 7 | White |

**Example - Animated Graphics:**
```pascal
program ColorDemo;
var i: integer;
begin
  ClrScr;
  HideCursor;
  for i := 1 to 10 do
  begin
    GotoXY(i, i);
    TextBackground(i mod 8);
    write('  ');
    Sleep(100)
  end;
  NormVideo;
  ShowCursor;
  GotoXY(1, 12)
end.
```

---

## Compiler Directives

### Include Directive

Split source code across multiple files:

```pascal
{$I filename.inc}
{$INCLUDE path/to/file.inc}
```

- Paths are relative to the including file's directory
- Maximum include depth of 8 levels
- Circular includes are detected and prevented

---

## Examples

### Factorial

```pascal
program Factorial;
var n: integer;

function Fact(n: integer): integer;
begin
  if n <= 1 then
    Fact := 1
  else
    Fact := n * Fact(n - 1)
end;

begin
  write('Enter a number: ');
  readln(n);
  write(n);
  write('! = ');
  writeln(Fact(n))
end.
```

### FizzBuzz

```pascal
program FizzBuzz;
var i: integer;
begin
  for i := 1 to 100 do
  begin
    if (i mod 15) = 0 then
      writeln('FizzBuzz')
    else if (i mod 3) = 0 then
      writeln('Fizz')
    else if (i mod 5) = 0 then
      writeln('Buzz')
    else
      writeln(i)
  end
end.
```

### Towers of Hanoi with Animation

See `examples/hanoi.pas` for a complete implementation featuring:
- Smooth sprite-like disk animation
- Colored disks using terminal colors
- Timer-based animation delays

```bash
cat examples/hanoi.pas | ./v2/tpcv2 > /tmp/hanoi.s && clang /tmp/hanoi.s -o /tmp/hanoi
/tmp/hanoi
```

### Tetris

A complete Tetris game (~33KB executable) with real-time keyboard input:
- All 7 tetromino pieces with proper rotation
- Arrow keys and WASD controls
- Line clearing and scoring
- Non-blocking keyboard input

```bash
cat examples/tetris.pas | ./v2/tpcv2 > /tmp/tetris.s && clang /tmp/tetris.s -o /tmp/tetris
/tmp/tetris
```

---

## Compiler Architecture

TuxPascal is a single-pass recursive descent compiler:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

### Components

| Component | Description |
|-----------|-------------|
| **Lexer** | Tokenizes Pascal source (case-insensitive keywords) |
| **Parser** | Recursive descent parser with inline code generation |
| **Symbol Table** | Manages identifiers with scope support |
| **Code Generator** | Emits ARM64 assembly directly |
| **Runtime** | Built-in routines for I/O, strings, memory, terminal |

### Register Usage

| Register | Purpose |
|----------|---------|
| x0-x7 | Function arguments / return value |
| x8-x18 | Temporary registers |
| x19 | stdin file descriptor |
| x20 | stdout file descriptor |
| x21-x28 | Callee-saved |
| x29 | Frame pointer |
| x30 | Link register |
| sp | Stack pointer |
| d0-d7 | Floating-point arguments / return |

### Memory Layout

- Stack-based expression evaluation
- Frame pointer (x29) based local variable access
- Heap allocation via bump allocator (mmap)
- Strings: 256 bytes (1 length byte + 255 chars)

---

## Building from Source

### Two Compiler Implementations

1. **v1 (`src/`)** - Bootstrap compiler written in C. Frozen.
2. **v2 (`v2/`)** - Self-hosting compiler written in Pascal. Active development.

### Build Commands

```bash
# Build C bootstrap compiler
make

# Rebuild v2 using v1
./tpc v2/compiler_split.pas -o v2/tpcv2

# Verify self-hosting
cat v2/compiler.pas | ./v2/tpcv2 > /tmp/v3.s && clang /tmp/v3.s -o /tmp/v3
cat v2/compiler.pas | /tmp/v3 > /tmp/v4.s
diff /tmp/v3.s /tmp/v4.s  # Should be identical
```

### Regenerating compiler.pas

After modifying include files, regenerate the single-file version:

```bash
(
echo '{ TuxPascal v2 - Self-hosting Pascal Compiler }'
echo 'program TuxPascalV2;'
echo ''
cat v2/inc/constants.inc
cat v2/inc/utility.inc
cat v2/inc/lexer.inc
cat v2/inc/symbols.inc
cat v2/inc/emitters.inc
cat v2/inc/runtime.inc
cat v2/inc/parser.inc
cat v2/inc/declarations.inc
cat v2/inc/main.inc
) > v2/compiler.pas
```

---

## License

MIT License

Copyright (c) 2024-2025 Eduardo Stern

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
