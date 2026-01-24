#!/bin/bash
# TuxPascal wrapper - provides user-friendly CLI around the compiler
# Usage: tpc <input.pas> [-o <output>] [-S] [-c] [-I<path>] [-ltuxgraph] [-ltuxnet]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMPILER="$SCRIPT_DIR/../build/bin/tuxpascal"
LIB_DIR="$SCRIPT_DIR/../lib"

# If installed system-wide, look for the raw compiler
if [ ! -f "$COMPILER" ]; then
    COMPILER="$(dirname "$0")/tuxpascal-core"
    LIB_DIR="$(dirname "$0")/../lib"
fi

# If lib dir doesn't exist, try relative to script
if [ ! -d "$LIB_DIR" ]; then
    LIB_DIR="$SCRIPT_DIR/../../lib"
fi

usage() {
    echo "Usage: tpc <input.pas> [-o <output>] [-S] [-c] [-ltuxgraph] [-ltuxnet]"
    echo ""
    echo "TuxPascal - A Pascal compiler for ARM64 macOS"
    echo ""
    echo "Options:"
    echo "  -o <file>    Output file name (default: input name without .pas)"
    echo "  -S           Output assembly only (don't assemble/link)"
    echo "  -c           Compile only, produce object file (.o)"
    echo "  -I<path>     Add directory to unit search path"
    echo "  -ltuxgraph   Link with TuxGraph library (graphics and sound)"
    echo "  -ltuxnet     Link with TuxNet library (networking)"
    echo ""
    echo "Examples:"
    echo "  tpc hello.pas                      # Compile to executable"
    echo "  tpc game.pas -ltuxgraph -o game    # Compile with graphics library"
    echo "  tpc -S program.pas                 # Output assembly only"
    exit 1
}

INPUT=""
OUTPUT=""
ASM_ONLY=0
OBJ_ONLY=0
LINK_TUXGRAPH=0
LINK_TUXNET=0
INCLUDE_PATHS=()

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
        -c)
            OBJ_ONLY=1
            shift
            ;;
        -ltuxgraph)
            LINK_TUXGRAPH=1
            shift
            ;;
        -ltuxnet)
            LINK_TUXNET=1
            shift
            ;;
        -I*)
            INCLUDE_PATHS+=("$1")
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
elif [ $OBJ_ONLY -eq 1 ]; then
    OBJ_OUTPUT="${OUTPUT%.o}.o"
    if ! clang -c "$TMPASM" -o "$OBJ_OUTPUT"; then
        echo "Assembly failed"
        exit 1
    fi
    echo "Compiled $INPUT -> $OBJ_OUTPUT"
else
    # Build link command
    LINK_CMD="clang $TMPASM -o $OUTPUT"

    # Add TuxGraph library and frameworks
    if [ $LINK_TUXGRAPH -eq 1 ]; then
        if [ -f "$LIB_DIR/tuxgraph.o" ]; then
            LINK_CMD="$LINK_CMD $LIB_DIR/tuxgraph.o"
        else
            echo "Error: tuxgraph.o not found. Run 'make' in lib/ directory first."
            exit 1
        fi
        LINK_CMD="$LINK_CMD -framework Cocoa -framework CoreGraphics -framework AudioToolbox"
    fi

    # Add TuxNet library
    if [ $LINK_TUXNET -eq 1 ]; then
        if [ -f "$LIB_DIR/tuxnet.o" ]; then
            LINK_CMD="$LINK_CMD $LIB_DIR/tuxnet.o"
        else
            echo "Error: tuxnet.o not found. Run 'make' in lib/ directory first."
            exit 1
        fi
    fi

    if ! eval $LINK_CMD; then
        echo "Linking failed"
        exit 1
    fi
    echo "Compiled $INPUT -> $OUTPUT"
fi
