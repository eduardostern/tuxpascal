#!/bin/bash
# TuxPascal installer
# Usage: ./install.sh [prefix]
# Or: curl -fsSL https://raw.githubusercontent.com/.../install.sh | bash

set -e

PREFIX="${1:-/usr/local}"
REPO_URL="https://github.com/yourusername/tuxpascal"  # Update this

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() { echo -e "${GREEN}==>${NC} $1"; }
warn() { echo -e "${YELLOW}Warning:${NC} $1"; }
error() { echo -e "${RED}Error:${NC} $1"; exit 1; }

# Check prerequisites
check_prereqs() {
    info "Checking prerequisites..."

    if ! command -v clang &> /dev/null; then
        error "clang is required but not installed. Install Xcode Command Line Tools: xcode-select --install"
    fi

    if ! command -v make &> /dev/null; then
        error "make is required but not installed. Install Xcode Command Line Tools: xcode-select --install"
    fi

    # Check architecture
    ARCH=$(uname -m)
    if [ "$ARCH" != "arm64" ]; then
        error "TuxPascal currently only supports ARM64 macOS (Apple Silicon). Detected: $ARCH"
    fi

    OS=$(uname -s)
    if [ "$OS" != "Darwin" ]; then
        error "TuxPascal currently only supports macOS. Detected: $OS"
    fi
}

# Build from source
build_from_source() {
    info "Building TuxPascal from source..."

    TMPDIR=$(mktemp -d)
    trap "rm -rf $TMPDIR" EXIT

    cd "$TMPDIR"

    if command -v git &> /dev/null; then
        info "Cloning repository..."
        git clone --depth 1 "$REPO_URL" tuxpascal
    else
        info "Downloading source archive..."
        curl -fsSL "$REPO_URL/archive/main.tar.gz" | tar xz
        mv tuxpascal-main tuxpascal
    fi

    cd tuxpascal

    info "Building..."
    make

    info "Installing to $PREFIX/bin..."
    sudo make install PREFIX="$PREFIX"
}

# Local installation (when run from repo)
install_local() {
    if [ -f "Makefile" ] && [ -d "bootstrap" ]; then
        info "Installing from local source..."
        make

        if [ -w "$PREFIX/bin" ]; then
            make install PREFIX="$PREFIX"
        else
            info "Requires sudo for installation to $PREFIX/bin"
            sudo make install PREFIX="$PREFIX"
        fi
    else
        error "Not in TuxPascal directory and remote installation not configured"
    fi
}

main() {
    echo ""
    echo "  TuxPascal Installer"
    echo "  A Pascal compiler for ARM64 macOS"
    echo ""

    check_prereqs

    # Check if we're in the repo directory
    if [ -f "Makefile" ] && [ -d "bootstrap" ]; then
        install_local
    else
        build_from_source
    fi

    echo ""
    info "TuxPascal installed successfully!"
    echo ""
    echo "  Usage: tuxpascal <input.pas> [-o <output>]"
    echo ""
    echo "  Example:"
    echo "    echo 'program Hello; begin writeln(\"Hello!\"); end.' > hello.pas"
    echo "    tuxpascal hello.pas -o hello"
    echo "    ./hello"
    echo ""
}

main "$@"
