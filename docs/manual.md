# TuxPascal User Manual

## Introduction

TuxPascal is a Pascal compiler for ARM64 macOS that produces native executables. It supports a subset of Turbo Pascal with modern extensions including units, records, pointers, sets, and file I/O.

## Getting Started

### Installation

```bash
git clone https://github.com/eduardostern/tuxpascal.git
cd tuxpascal
make
sudo make install  # Optional: install to /usr/local/bin
```

### Your First Program

Create a file `hello.pas`:

```pascal
Program Hello;
Begin
  WriteLn('Hello, World!')
End.
```

Compile and run:

```bash
tpc hello.pas
./hello
```

## Command Line Usage

```
tpc <input.pas> [options]

Options:
  -o <file>    Output file name (default: input name without .pas)
  -S           Output assembly only (don't assemble/link)
  -c           Compile to object file only
  -I<path>     Add include/unit search path
```

### Examples

```bash
# Compile to executable
tpc myprogram.pas

# Compile with custom output name
tpc myprogram.pas -o myapp

# Generate assembly for inspection
tpc -S myprogram.pas

# Compile a unit
tpc -c myunit.pas
```

## Language Reference

### Program Structure

Every Pascal program has this structure:

```pascal
Program ProgramName;

{ Optional: Uses clause for units }
Uses Unit1, Unit2;

{ Optional: Constant declarations }
Const
  MaxSize = 100;

{ Optional: Type declarations }
Type
  TMyType = Integer;

{ Optional: Variable declarations }
Var
  x, y: Integer;

{ Optional: Procedure/Function declarations }
Procedure DoSomething;
Begin
  { ... }
End;

{ Main program block }
Begin
  { statements }
End.
```

### Data Types

#### Basic Types

| Type | Description | Size |
|------|-------------|------|
| `Integer` | Signed integer | 64 bits |
| `Char` | Single character | 8 bits |
| `Boolean` | True or False | 8 bits |
| `String` | Character string | 256 bytes (length byte + 255 chars) |
| `Real` | Floating point | 64 bits (IEEE 754 double) |

#### Arrays

```pascal
Var
  { Static array }
  numbers: Array[1..100] Of Integer;

  { Multi-dimensional array }
  matrix: Array[1..10, 1..10] Of Real;

  { Character array }
  buffer: Array[0..255] Of Char;
```

#### Records

```pascal
Type
  TPoint = Record
    x, y: Integer
  End;

  { Record with nested record }
  TRectangle = Record
    topLeft, bottomRight: TPoint
  End;

  { Variant record }
  TShape = Record
    Case kind: Integer Of
      1: (radius: Integer);
      2: (width, height: Integer)
  End;
```

#### Pointers

```pascal
Type
  PInteger = ^Integer;
  PPoint = ^TPoint;

Var
  p: ^Integer;
  pp: ^^Integer;  { Pointer to pointer }
```

#### Sets

```pascal
Type
  TCharSet = Set Of Char;
  TDigits = Set Of 0..9;

Var
  vowels: TCharSet;

Begin
  vowels := ['a', 'e', 'i', 'o', 'u'];
  If 'a' In vowels Then
    WriteLn('Is a vowel')
End.
```

#### Enumerated Types

```pascal
Type
  TColor = (Red, Green, Blue, Yellow);
  TDay = (Mon, Tue, Wed, Thu, Fri, Sat, Sun);

Var
  color: TColor;

Begin
  color := Red;
  color := Succ(color);  { Now Green }
End.
```

#### Subrange Types

```pascal
Type
  TMonth = 1..12;
  TUpperCase = 'A'..'Z';
  TWorkDay = Mon..Fri;
```

### Constants

```pascal
Const
  MaxItems = 100;
  Pi = 3.14159265359;
  Greeting = 'Hello';
  Debug = True;
```

### Variables

```pascal
Var
  count: Integer;
  name: String;
  x, y, z: Real;
  found: Boolean;
```

### Operators

#### Arithmetic Operators

| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Real division |
| `Div` | Integer division |
| `Mod` | Modulo (remainder) |

#### Comparison Operators

| Operator | Description |
|----------|-------------|
| `=` | Equal |
| `<>` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

#### Logical Operators

| Operator | Description |
|----------|-------------|
| `And` | Logical AND (short-circuit) |
| `Or` | Logical OR (short-circuit) |
| `Not` | Logical NOT |

#### Set Operators

| Operator | Description |
|----------|-------------|
| `+` | Union |
| `-` | Difference |
| `*` | Intersection |
| `In` | Membership test |

#### Pointer Operators

| Operator | Description |
|----------|-------------|
| `^` | Dereference |
| `@` | Address of |

### Control Structures

#### If-Then-Else

```pascal
If condition Then
  statement
Else
  statement;

{ With Begin-End blocks }
If x > 0 Then
Begin
  WriteLn('Positive');
  count := count + 1
End
Else
Begin
  WriteLn('Non-positive');
  count := 0
End;
```

#### Case Statement

```pascal
Case value Of
  1: WriteLn('One');
  2: WriteLn('Two');
  3, 4, 5: WriteLn('Three to Five');
  6..10: WriteLn('Six to Ten')
Else
  WriteLn('Other')
End;
```

#### While Loop

```pascal
While condition Do
  statement;

While i <= 10 Do
Begin
  WriteLn(i);
  i := i + 1
End;
```

#### Repeat-Until Loop

```pascal
Repeat
  statement;
  statement
Until condition;

Repeat
  Write('Enter a number: ');
  ReadLn(n)
Until n > 0;
```

#### For Loop

```pascal
For i := 1 To 10 Do
  WriteLn(i);

For i := 10 DownTo 1 Do
  WriteLn(i);
```

#### Loop Control

```pascal
{ Exit current loop }
Break;

{ Skip to next iteration }
Continue;

{ Exit procedure/function }
Exit;
```

### Procedures and Functions

#### Procedures

```pascal
Procedure Greet(name: String);
Begin
  WriteLn('Hello, ', name, '!')
End;

{ With local variables }
Procedure Calculate(a, b: Integer);
Var
  result: Integer;
Begin
  result := a + b;
  WriteLn('Sum: ', result)
End;

{ Var parameters (pass by reference) }
Procedure Swap(Var a, b: Integer);
Var
  temp: Integer;
Begin
  temp := a;
  a := b;
  b := temp
End;
```

#### Functions

```pascal
Function Square(x: Integer): Integer;
Begin
  Square := x * x
End;

Function Max(a, b: Integer): Integer;
Begin
  If a > b Then
    Max := a
  Else
    Max := b
End;

{ Recursive function }
Function Factorial(n: Integer): Integer;
Begin
  If n <= 1 Then
    Factorial := 1
  Else
    Factorial := n * Factorial(n - 1)
End;
```

#### Forward Declarations

```pascal
Procedure B(x: Integer); Forward;

Procedure A(x: Integer);
Begin
  If x > 0 Then B(x - 1)
End;

Procedure B(x: Integer);
Begin
  If x > 0 Then A(x - 1)
End;
```

### Units

#### Creating a Unit

```pascal
Unit MyUtils;

Interface
  Function Double(x: Integer): Integer;
  Procedure PrintLine;

Implementation

Function Double(x: Integer): Integer;
Begin
  Double := x * 2
End;

Procedure PrintLine;
Begin
  WriteLn('----------------')
End;

Begin
  { Unit initialization code }
  WriteLn('MyUtils initialized')
End.
```

#### Using a Unit

```pascal
Program Main;
Uses MyUtils;

Begin
  WriteLn(Double(21));
  PrintLine
End.
```

Compile the unit first:
```bash
tpc -c myutils.pas
tpc main.pas
```

### Built-in Procedures and Functions

#### Input/Output

| Procedure/Function | Description |
|--------------------|-------------|
| `Write(...)` | Output without newline |
| `WriteLn(...)` | Output with newline |
| `Read(var)` | Read value |
| `ReadLn(var)` | Read value with newline |
| `ReadChar` | Read single character |
| `WriteChar(c)` | Write single character |

```pascal
Write('Enter name: ');
ReadLn(name);
WriteLn('Hello, ', name, '!');

{ Multiple values }
WriteLn('x=', x, ' y=', y);

{ Read single key }
ch := ReadChar;
```

#### String Functions

| Function | Description |
|----------|-------------|
| `Length(s)` | String length |
| `Copy(s, start, len)` | Extract substring |
| `Concat(s1, s2)` | Concatenate strings |
| `Pos(substr, s)` | Find substring position |
| `Trim(s)` | Remove leading/trailing spaces |
| `UpCase(c)` | Convert to uppercase |
| `LowerCase(c)` | Convert to lowercase |

```pascal
s := 'Hello World';
WriteLn(Length(s));        { 11 }
WriteLn(Copy(s, 1, 5));    { Hello }
WriteLn(Pos('World', s));  { 7 }

{ String concatenation }
s := 'Hello' + ' ' + 'World';
s := Concat('Hello', ' World');
```

#### Math Functions

| Function | Description |
|----------|-------------|
| `Abs(x)` | Absolute value |
| `Sqr(x)` | Square |
| `Sqrt(x)` | Square root |
| `Sin(x)` | Sine (radians) |
| `Cos(x)` | Cosine (radians) |
| `Tan(x)` | Tangent |
| `ArcTan(x)` | Arctangent |
| `Exp(x)` | e^x |
| `Ln(x)` | Natural logarithm |
| `Round(x)` | Round to nearest integer |
| `Trunc(x)` | Truncate to integer |
| `Random` | Random number 0..1 |
| `Random(n)` | Random integer 0..n-1 |

```pascal
x := Sqrt(2.0);
y := Sin(Pi / 2);
n := Random(100);  { 0 to 99 }
```

#### Ordinal Functions

| Function | Description |
|----------|-------------|
| `Ord(x)` | Ordinal value |
| `Chr(n)` | Character from ordinal |
| `Succ(x)` | Successor |
| `Pred(x)` | Predecessor |
| `Inc(x)` | Increment |
| `Dec(x)` | Decrement |
| `Odd(n)` | True if odd |

```pascal
WriteLn(Ord('A'));   { 65 }
WriteLn(Chr(65));    { A }
Inc(count);          { count := count + 1 }
Dec(count, 5);       { count := count - 5 }
```

#### Memory Functions

| Procedure/Function | Description |
|--------------------|-------------|
| `New(p)` | Allocate memory |
| `Dispose(p)` | Free memory |
| `SizeOf(type)` | Size in bytes |

```pascal
Type
  PNode = ^TNode;
  TNode = Record
    value: Integer;
    next: PNode
  End;

Var
  p: PNode;

Begin
  New(p);
  p^.value := 42;
  p^.next := Nil;
  Dispose(p)
End.
```

#### System Functions

| Function | Description |
|----------|-------------|
| `ParamCount` | Number of command line arguments |
| `ParamStr(n)` | Get command line argument |
| `Halt` | Exit program |
| `Halt(n)` | Exit with code |

```pascal
If ParamCount < 1 Then
Begin
  WriteLn('Usage: ', ParamStr(0), ' <filename>');
  Halt(1)
End;
filename := ParamStr(1);
```

### File I/O

#### Text Files

```pascal
Var
  f: Text;
  line: String;

Begin
  { Reading }
  Assign(f, 'input.txt');
  Reset(f);
  While Not Eof(f) Do
  Begin
    ReadLn(f, line);
    WriteLn(line)
  End;
  Close(f);

  { Writing }
  Assign(f, 'output.txt');
  Rewrite(f);
  WriteLn(f, 'Line 1');
  WriteLn(f, 'Line 2');
  Close(f)
End.
```

### Screen Control (CRT-like)

| Procedure | Description |
|-----------|-------------|
| `ClrScr` | Clear screen |
| `GotoXY(x, y)` | Move cursor |
| `ClrEol` | Clear to end of line |
| `TextColor(c)` | Set text color |
| `TextBackground(c)` | Set background color |
| `HideCursor` | Hide cursor |
| `ShowCursor` | Show cursor |

#### Colors

| Value | Color |
|-------|-------|
| 0 | Black |
| 1 | Red |
| 2 | Green |
| 3 | Yellow |
| 4 | Blue |
| 5 | Magenta |
| 6 | Cyan |
| 7 | White |
| 8-15 | Bright versions |

```pascal
ClrScr;
TextColor(14);  { Bright yellow }
GotoXY(10, 5);
Write('Hello!');
TextColor(7);   { Reset to white }
```

### Keyboard Input

| Function/Procedure | Description |
|--------------------|-------------|
| `KeyPressed` | True if key available |
| `ReadKey` | Read key without echo |
| `InitKeyboard` | Enable raw mode |
| `DoneKeyboard` | Restore normal mode |
| `Sleep(ms)` | Pause execution |

```pascal
InitKeyboard;
Repeat
  If KeyPressed Then
  Begin
    ch := ReadKey;
    WriteLn('You pressed: ', ch)
  End;
  Sleep(100)
Until ch = 'q';
DoneKeyboard;
```

## Error Messages

TuxPascal provides descriptive error messages:

| Error | Description |
|-------|-------------|
| Unexpected character | Invalid character in source |
| Unexpected token | Syntax error - unexpected symbol |
| Undefined identifier 'name' | Variable/procedure not declared |
| Identifier expected | Expected a name |
| Type identifier expected | Expected a type name |
| Syntax error | General syntax error |
| String expression expected | Expected a string |
| Undefined field 'name' | Record field not found |
| Set type too large | Set exceeds 64 elements |
| Unit not found | TPU file not found |

## Tips and Best Practices

1. **Use meaningful names**: `customerCount` instead of `cc`
2. **Initialize variables**: Uninitialized variables have undefined values
3. **Free allocated memory**: Always `Dispose` what you `New`
4. **Close files**: Always `Close` opened files
5. **Use units**: Organize code into reusable units
6. **Comment your code**: Use `{ }` or `(* *)` for comments

## Limitations

- Maximum 500 symbols (variables, procedures, etc.)
- Maximum string length: 255 characters
- Maximum set size: 64 elements
- Maximum include nesting: 8 levels
- No floating-point in sets
- No object-oriented features

## Example Programs

See the `examples/` directory for complete programs including:
- `hello.pas` - Hello World
- `tetris.pas` - Tetris game
- `hanoi.pas` - Towers of Hanoi
- `calculator.pas` - Simple calculator
