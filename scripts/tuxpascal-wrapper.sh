#!/bin/bash
# TuxPascal wrapper - provides user-friendly CLI around the compiler
# Usage: tuxpascal <input.pas> [-o <output>] [-S]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMPILER="$SCRIPT_DIR/../build/bin/tuxpascal"

# If installed system-wide, look for the raw compiler
if [ ! -f "$COMPILER" ]; then
    COMPILER="$(dirname "$0")/tuxpascal-core"
fi

usage() {
    echo "Usage: tuxpascal <input.pas> [-o <output>] [-S]"
    echo ""
    echo "TuxPascal - A Pascal compiler for ARM64 macOS"
    echo ""
    echo "Options:"
    echo "  -o <file>  Output file name (default: input name without .pas)"
    echo "  -S         Output assembly only (don't assemble/link)"
    exit 1
}

INPUT=""
OUTPUT=""
ASM_ONLY=0

while [ $# -gt 0 ]; do
    case "$1" in
        -o)
            OUTPUT="$2"
            shift 2
            ;;
        -S)
            ASM_ONLY=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        -*)
            echo "Unknown option: $1"
            usage
            ;;
        *)
            INPUT="$1"
            shift
            ;;
    esac
done

if [ -z "$INPUT" ]; then
    usage
fi

if [ ! -f "$INPUT" ]; then
    echo "Error: Input file not found: $INPUT"
    exit 1
fi

# Default output name: input without .pas extension
if [ -z "$OUTPUT" ]; then
    OUTPUT="${INPUT%.pas}"
    if [ "$OUTPUT" = "$INPUT" ]; then
        OUTPUT="a.out"
    fi
fi

# Compile to assembly
TMPASM=$(mktemp /tmp/tpc_XXXXXX.s)
trap "rm -f $TMPASM" EXIT

if ! cat "$INPUT" | "$COMPILER" > "$TMPASM"; then
    echo "Compilation failed"
    exit 1
fi

if [ $ASM_ONLY -eq 1 ]; then
    mv "$TMPASM" "$OUTPUT"
    echo "Compiled $INPUT -> $OUTPUT"
else
    if ! clang "$TMPASM" -o "$OUTPUT"; then
        echo "Assembly/linking failed"
        exit 1
    fi
    echo "Compiled $INPUT -> $OUTPUT"
fi
