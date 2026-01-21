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

## Example

```pascal
program Hello;
begin
  writeln('Hello, World!');
end.
```

```bash
$ ./tpcv2 hello.pas
Compiled hello.pas -> hello
$ ./hello
Hello, World!
```

## Supported Pascal Features

| Category | Features |
|----------|----------|
| **Types** | `integer`, `char`, `boolean`, `string`, `array` |
| **Statements** | `:=`, `if`/`then`/`else`, `while`/`do`, `repeat`/`until`, `for`/`to`/`downto`, `begin`/`end` |
| **Declarations** | `program`, `const`, `var`, `procedure`, `function`, `forward` |
| **Operators** | `+`, `-`, `*`, `div`, `mod`, `=`, `<>`, `<`, `>`, `<=`, `>=`, `and`, `or`, `not` |
| **I/O** | `write`, `writeln`, `read`, `readln` |
| **Parameters** | By value, by reference (`var`), nested scopes with static links |

## Project Structure

```
tuxpascal/
├── tpcv2              # Main compiler wrapper script
├── v2/
│   ├── compiler.pas   # Self-hosting compiler source (Pascal)
│   └── tpcv2          # Compiled v2 compiler binary
├── src/               # Bootstrap compiler (C) - frozen
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

## License

MIT
