# TuxPascal

A minimal Pascal compiler targeting ARM64 macOS. Compiles a subset of Turbo Pascal 1.0-style Pascal to native executables.

## Features

- **Self-hosting**: The compiler is written in Pascal and can compile itself
- **Native ARM64**: Generates native Apple Silicon executables
- **Single-pass**: Fast compilation via recursive descent parsing with inline code generation
- **Zero dependencies**: Only requires `clang` for assembly/linking (included with Xcode)

## Quick Start

```bash
# Build the bootstrap compiler
make

# Compile a Pascal program
./tpcv2 examples/hello.pas -o hello
./hello
```

## Usage

```bash
./tpcv2 <input.pas> [-o <output>] [-S]
```

Options:
- `-o <file>` - Output file name (default: input name without .pas extension)
- `-S` - Output assembly only (don't assemble/link)

## Examples

### Hello World

```pascal
program Hello;
begin
  writeln('Hello, World!');
end.
```

### Towers of Hanoi with Animated Graphics

TuxPascal includes ANSI terminal support for creating retro-style games and animations:

```pascal
program TowersOfHanoi;
const
  NumDisks = 5;
  AnimDelay = 12;
var
  tower1, tower2, tower3: array[0..9] of integer;
begin
  ClrScr;
  HideCursor;
  TextColor(3);
  GotoXY(25, 1);
  write('*** TOWERS OF HANOI ***');
  NormVideo;
  { ... animate disks with smooth movement ... }
  Sleep(AnimDelay);  { millisecond timer }
  ShowCursor;
end.
```

Run the full example:
```bash
cat examples/hanoi.pas | ./v2/tpcv2 > /tmp/hanoi.s && clang /tmp/hanoi.s -o /tmp/hanoi
/tmp/hanoi
```

### Tetris

A complete Tetris game with real-time keyboard input:

```pascal
program Tetris;
begin
  ClrScr;
  HideCursor;
  InitKeyboard;  { Enable raw keyboard mode }

  while gameOver = 0 do
  begin
    if KeyPressed then
    begin
      ch := readchar;
      { Handle arrow keys and WASD }
    end;
    Sleep(25)
  end;

  DoneKeyboard;  { Restore terminal }
end.
```

Run it:
```bash
cat examples/tetris.pas | ./v2/tpcv2 > /tmp/tetris.s && clang /tmp/tetris.s -o /tmp/tetris
/tmp/tetris
```

Controls: Arrow keys or WASD, Space to drop, Q to quit.

## Supported Pascal Features

| Category | Features |
|----------|----------|
| **Types** | `integer`, `char`, `boolean`, `string`, `array`, `real`, `^type` (pointers), `record` |
| **Statements** | `:=`, `if`/`then`/`else`, `while`/`do`, `repeat`/`until`, `for`/`to`/`downto`, `begin`/`end` |
| **Declarations** | `program`, `const`, `type`, `var`, `procedure`, `function`, `forward` |
| **Operators** | `+`, `-`, `*`, `/`, `div`, `mod`, `=`, `<>`, `<`, `>`, `<=`, `>=`, `and`, `or`, `not`, `@`, `^` |
| **I/O** | `write`, `writeln`, `read`, `readln`, `readchar`, `writechar` |
| **String Ops** | `length`, `copy`, `concat`, `+`, `pos`, `delete`, `insert`, `str`, `val`, `trim`, `ltrim`, `rtrim` |
| **Utilities** | `abs`, `odd`, `sqr`, `succ`, `pred`, `inc`, `dec`, `upcase`, `lowercase`, `ord`, `chr`, `halt` |
| **Screen/Terminal** | `ClrScr`, `GotoXY`, `ClrEol`, `TextColor`, `TextBackground`, `NormVideo`, `HighVideo`, `LowVideo`, `HideCursor`, `ShowCursor`, `Sleep` |
| **Keyboard** | `KeyPressed`, `InitKeyboard`, `DoneKeyboard` - non-blocking input for games |
| **Parameters** | By value, by reference (`var`), nested scopes with static links |
| **Directives** | `{$I filename}`, `{$INCLUDE filename}` - include files |

## Project Structure

```
tuxpascal/
├── tpc                # Bootstrap compiler (C)
├── v2/
│   ├── compiler.pas       # Self-hosting compiler (generated from inc files)
│   ├── compiler_split.pas # Self-hosting compiler entry point (uses includes)
│   ├── inc/               # Include files for modular compiler
│   │   ├── constants.inc  # Token types, globals
│   │   ├── utility.inc    # Helper functions
│   │   ├── lexer.inc      # Tokenizer
│   │   ├── symbols.inc    # Symbol table
│   │   ├── emitters.inc   # Assembly output
│   │   ├── runtime.inc    # Runtime code generators
│   │   ├── parser.inc     # Expression/statement parsing
│   │   ├── declarations.inc # Proc/func/block parsing
│   │   └── main.inc       # Main entry point
│   └── tpcv2              # Compiled v2 compiler binary
├── src/               # Bootstrap compiler source (C)
├── examples/          # Example Pascal programs
└── Makefile
```

## Architecture

The compiler is a single-pass recursive descent compiler:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

Two implementations exist:
- **v1 (`src/`)** - Bootstrap compiler in C. Frozen; only modified if needed to compile v2.
- **v2 (`v2/compiler.pas`)** - Self-hosting compiler in Pascal. All active development happens here.

## Building from Source

```bash
# Build the C bootstrap compiler
make

# Rebuild v2 compiler using v1
./tpc v2/compiler.pas -o v2/tpcv2

# Verify self-hosting (v2 compiles itself to v3, v3 compiles itself to v4)
cat v2/compiler.pas | ./v2/tpcv2 > /tmp/v3.s && clang /tmp/v3.s -o /tmp/v3
cat v2/compiler.pas | /tmp/v3 > /tmp/v4.s
diff /tmp/v3.s /tmp/v4.s  # Should be identical
```

## Requirements

- macOS on Apple Silicon (ARM64)
- Xcode Command Line Tools (`xcode-select --install`)

## Author

**Eduardo Stern** - eduardostern@icloud.com

## License

MIT
