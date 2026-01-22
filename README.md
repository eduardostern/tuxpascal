# TuxPascal

A minimal Pascal compiler targeting ARM64 macOS. Compiles a subset of Turbo Pascal 1.0-style Pascal to native executables.

## Features

- **Self-hosting**: The compiler is written in Pascal and can compile itself
- **Native ARM64**: Generates native Apple Silicon executables
- **Single-pass**: Fast compilation via recursive descent parsing with inline code generation
- **Zero dependencies**: Only requires `clang` for assembly/linking (included with Xcode)

## Installation

```bash
# Clone and build
git clone https://github.com/yourusername/tuxpascal.git
cd tuxpascal
make

# Install system-wide (optional)
sudo make install
```

Or use the install script:
```bash
./scripts/install.sh
```

## Quick Start

```bash
# Build the compiler
make

# Compile a Pascal program
./build/bin/tpc examples/hello.pas -o hello
./hello
```

## Usage

```bash
tuxpascal <input.pas> [-o <output>] [-S]
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
./build/bin/tpc examples/hanoi.pas -o hanoi && ./hanoi
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
./build/bin/tpc examples/tetris.pas -o tetris && ./tetris
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
├── bootstrap/           # C bootstrap compiler (frozen)
│   ├── main.c
│   ├── lexer.c/h
│   ├── parser.c/h
│   └── symbols.c/h
├── compiler/            # Pascal self-hosting compiler (active development)
│   ├── tuxpascal.pas         # Single-file version (generated)
│   ├── tuxpascal_modular.pas # Entry point with includes
│   └── inc/                  # Modular source files
├── examples/            # Example Pascal programs
├── build/               # Build output (gitignored)
│   ├── bootstrap/       # Bootstrap compiler binary
│   └── bin/             # Pascal compiler binary
├── scripts/             # Build and install scripts
├── docs/                # Additional documentation
└── Makefile
```

## Build Targets

```bash
make              # Build the Pascal compiler (default)
make bootstrap    # Build only the C bootstrap compiler
make test         # Run example programs
make self-host    # Verify self-hosting capability
make install      # Install to /usr/local/bin
make uninstall    # Remove from /usr/local/bin
make clean        # Remove build artifacts
make help         # Show all targets
```

## Architecture

The compiler is a single-pass recursive descent compiler:

```
Source (.pas) → Lexer → Parser → Assembly (.s) → clang → Executable
```

Two implementations exist:
- **Bootstrap (`bootstrap/`)** - C compiler used to build the Pascal compiler. Frozen.
- **Compiler (`compiler/`)** - Self-hosting compiler in Pascal. All active development happens here.

## Self-Hosting & Compiler Generations

TuxPascal is fully self-hosting. The compiler can compile itself, and successive generations produce identical output:

```
Bootstrap (C) → compiles → v2 (Pascal)
v2            → compiles → v3
v3            → compiles → v4
v4            → compiles → v5 (identical to v4)
```

### Quick Verification

```bash
make self-host
```

### Manual Self-Hosting Build

Build the complete compiler chain manually:

```bash
# Generate single-file compiler from modular source
./scripts/merge-compiler.sh

# Build v3 (compiled by v2)
cat compiler/tuxpascal.pas | ./build/bin/tuxpascal > /tmp/v3.s
clang /tmp/v3.s -o build/bin/v3

# Build v4 (compiled by v3)
cat compiler/tuxpascal.pas | ./build/bin/v3 > /tmp/v4.s
clang /tmp/v4.s -o build/bin/v4

# Verify: v3 and v4 produce identical output
diff /tmp/v3.s /tmp/v4.s  # No output = success
```

### Compiling Programs with v4

Use the v4 compiler (compiled by v3, which was compiled by v2) to build programs:

```bash
# Compile Tetris with v4
cat examples/tetris.pas | ./build/bin/v4 > /tmp/tetris.s
clang /tmp/tetris.s -o build/bin/tetris

# Compile Towers of Hanoi with v4
cat examples/hanoi.pas | ./build/bin/v4 > /tmp/hanoi.s
clang /tmp/hanoi.s -o build/bin/hanoi
```

### Build Output

After a full build with self-hosting verification:

```
build/bin/
├── tpc        # Wrapper script (user-friendly CLI)
├── tuxpascal  # v2 compiler (compiled by bootstrap)
├── v3         # v3 compiler (compiled by v2)
├── v4         # v4 compiler (compiled by v3)
├── tetris     # Compiled by v4
├── hanoi      # Compiled by v4
├── hello      # Test programs
├── factorial
└── fizzbuzz
```

### Binary Size Differences

Interestingly, v2 and v3 have different sizes despite producing identical output:

| Compiler | Size | Compiled by |
|----------|------|-------------|
| v2 | 297,736 bytes | C bootstrap |
| v3 | 281,024 bytes | v2 (Pascal) |
| v4 | 281,024 bytes | v3 (Pascal) |

The ~16KB difference comes from how each compiler handles string literals:

| Segment | v2 (bootstrap) | v3 (Pascal) |
|---------|----------------|-------------|
| __TEXT (code) | 278,528 | 278,528 |
| __DATA | 16,384 | 0 |

- **Bootstrap (C)** creates a `.data` section with string constants, requiring a 16KB page-aligned segment
- **Pascal compiler** writes strings character-by-character inline, requiring no data segment

```asm
# Bootstrap approach - uses data section
.data
str0: .ascii "Error "

# Pascal approach - inline character writes
mov x0, #69   ; 'E'
; ...syscall...
mov x0, #114  ; 'r'
; ...syscall...
```

The actual machine code is the same size. Once self-hosted, binary size stabilizes (v3 = v4).

## Requirements

- macOS on Apple Silicon (ARM64)
- Xcode Command Line Tools (`xcode-select --install`)

## Author

**Eduardo Stern** - eduardostern@icloud.com

## License

MIT
