# TuxPascal Makefile
# Builds both the C bootstrap compiler and the Pascal self-hosting compiler

CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c99

# Directories
BOOTSTRAP_SRC = bootstrap
COMPILER_SRC = compiler
BUILD = build
OBJ = $(BUILD)/obj
BIN = $(BUILD)/bin

# Bootstrap compiler (C)
BOOTSTRAP_SRCS = $(BOOTSTRAP_SRC)/main.c $(BOOTSTRAP_SRC)/lexer.c $(BOOTSTRAP_SRC)/parser.c $(BOOTSTRAP_SRC)/symbols.c
BOOTSTRAP_OBJS = $(patsubst $(BOOTSTRAP_SRC)/%.c,$(OBJ)/%.o,$(BOOTSTRAP_SRCS))
BOOTSTRAP_BIN = $(BUILD)/bootstrap/tpc

# Pascal compiler
COMPILER_PAS = $(COMPILER_SRC)/tuxpascal_modular.pas
COMPILER_BIN = $(BIN)/tuxpascal

# Installation
PREFIX ?= /usr/local

# Default target: build everything
all: $(COMPILER_BIN)

# Bootstrap compiler
$(BOOTSTRAP_BIN): $(BOOTSTRAP_OBJS) | $(BUILD)/bootstrap
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ)/%.o: $(BOOTSTRAP_SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

# Pascal compiler (requires bootstrap)
$(COMPILER_BIN): $(BOOTSTRAP_BIN) $(COMPILER_PAS) | $(BIN)
	$(BOOTSTRAP_BIN) $(COMPILER_PAS) -o $@
	@# Create convenience wrapper
	@sed 's|COMPILER=.*|COMPILER="$(CURDIR)/$(COMPILER_BIN)"|' scripts/tuxpascal-wrapper.sh > $(BIN)/tpc
	@chmod +x $(BIN)/tpc

# Create build directories
$(OBJ):
	mkdir -p $(OBJ)

$(BUILD)/bootstrap:
	mkdir -p $(BUILD)/bootstrap

$(BIN):
	mkdir -p $(BIN)

# Convenience targets
bootstrap: $(BOOTSTRAP_BIN)

compiler: $(COMPILER_BIN)

# Self-hosting verification: compiler compiles itself
self-host: $(COMPILER_BIN)
	@echo "Generating single-file compiler..."
	@./scripts/merge-compiler.sh
	@echo "Building v3 (compiled by v2)..."
	@cat $(COMPILER_SRC)/tuxpascal.pas | $(COMPILER_BIN) > /tmp/v3.s
	@clang /tmp/v3.s -o /tmp/v3
	@echo "Building v4 (compiled by v3)..."
	@cat $(COMPILER_SRC)/tuxpascal.pas | /tmp/v3 > /tmp/v4.s
	@echo "Comparing v3 and v4 output..."
	@diff /tmp/v3.s /tmp/v4.s && echo "Self-hosting verified: v3 and v4 produce identical output"

# Helper to compile Pascal source with the Pascal compiler
# Usage: $(call compile_pas,input.pas,output)
define compile_pas
	@cat $(1) | $(COMPILER_BIN) > /tmp/tpc_$$$$.s && clang /tmp/tpc_$$$$.s -o $(2) && rm /tmp/tpc_$$$$.s
endef

# Run example programs
test: $(COMPILER_BIN) | $(BIN)
	@echo "Running tests..."
	$(call compile_pas,examples/hello.pas,$(BIN)/hello)
	@$(BIN)/hello
	$(call compile_pas,examples/factorial.pas,$(BIN)/factorial)
	@$(BIN)/factorial
	$(call compile_pas,examples/fizzbuzz.pas,$(BIN)/fizzbuzz)
	@$(BIN)/fizzbuzz
	@echo "All tests passed."

# Install to system
install: $(COMPILER_BIN)
	install -d $(PREFIX)/bin
	install -m 755 $(COMPILER_BIN) $(PREFIX)/bin/tuxpascal-core
	sed 's|COMPILER=.*|COMPILER="$(PREFIX)/bin/tuxpascal-core"|' scripts/tuxpascal-wrapper.sh > /tmp/tuxpascal
	install -m 755 /tmp/tuxpascal $(PREFIX)/bin/tuxpascal
	rm /tmp/tuxpascal

# Uninstall
uninstall:
	rm -f $(PREFIX)/bin/tuxpascal $(PREFIX)/bin/tuxpascal-core

# Clean build artifacts
clean:
	rm -rf $(BUILD)

# Clean everything including generated files
distclean: clean
	rm -f $(COMPILER_SRC)/tuxpascal.pas

# Show help
help:
	@echo "TuxPascal build targets:"
	@echo "  make           - Build the Pascal compiler (default)"
	@echo "  make bootstrap - Build only the C bootstrap compiler"
	@echo "  make compiler  - Build the Pascal compiler"
	@echo "  make test      - Run example programs"
	@echo "  make self-host - Verify self-hosting capability"
	@echo "  make install   - Install to $(PREFIX)/bin"
	@echo "  make uninstall - Remove from $(PREFIX)/bin"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make help      - Show this help"

.PHONY: all bootstrap compiler self-host test install uninstall clean distclean help
