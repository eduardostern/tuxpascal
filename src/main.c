#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"

static char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    return buf;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <input.pas> [-o <output>] [-S]\n", prog);
    fprintf(stderr, "\nTuxPascal - A minimal Pascal compiler for ARM64 macOS\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -o <file>  Output file name (default: a.out)\n");
    fprintf(stderr, "  -S         Output assembly only (don't assemble/link)\n");
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = "a.out";
    int asm_only = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-S") == 0) {
            asm_only = 1;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!input_file) {
        usage(argv[0]);
        return 1;
    }

    char *source = read_file(input_file);
    if (!source) {
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, source, input_file);

    Parser parser;
    parser_init(&parser, &lexer);

    // Determine assembly output file
    char asm_file[256];
    if (asm_only) {
        // If -S flag, use output_file as the .s file
        snprintf(asm_file, sizeof(asm_file), "%s", output_file);
    } else {
        // Otherwise create a temp file
        snprintf(asm_file, sizeof(asm_file), "/tmp/tpc_%d.s", getpid());
    }

    // Compile to assembly
    if (parser_compile(&parser, asm_file) != 0) {
        fprintf(stderr, "Compilation failed\n");
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (asm_only) {
        printf("Compiled %s -> %s\n", input_file, asm_file);
    } else {
        // Assemble and link using clang
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "clang -o '%s' '%s' 2>&1", output_file, asm_file);

        int ret = system(cmd);
        unlink(asm_file);  // Remove temp file

        if (ret != 0) {
            fprintf(stderr, "Assembly/linking failed\n");
            parser_free(&parser);
            free(source);
            return 1;
        }

        printf("Compiled %s -> %s\n", input_file, output_file);
    }

    parser_free(&parser);
    free(source);

    return 0;
}
