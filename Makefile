CC = clang
CFLAGS = -Wall -Wextra -O2 -std=c99
SRCDIR = src
OBJDIR = obj

SRCS = $(SRCDIR)/main.c $(SRCDIR)/lexer.c $(SRCDIR)/parser.c $(SRCDIR)/symbols.c
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

tpc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) tpc *.s

test: tpc
	./tpc examples/hello.pas -o hello
	./hello
	./tpc examples/factorial.pas -o factorial
	./factorial
	./tpc examples/fizzbuzz.pas -o fizzbuzz
	./fizzbuzz

.PHONY: clean test
