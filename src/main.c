#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "lexer.h"
#include "parser.h"

#define MAX_INCLUDE_DEPTH 8
#define MAX_INCLUDED_FILES 64

static char *included_files[MAX_INCLUDED_FILES];
static int included_count = 0;

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

static int file_already_included(const char *path) {
    for (int i = 0; i < included_count; i++) {
        if (strcmp(included_files[i], path) == 0) {
            return 1;
        }
    }
    return 0;
}

static void mark_file_included(const char *path) {
    if (included_count < MAX_INCLUDED_FILES) {
        included_files[included_count++] = strdup(path);
    }
}

static char *resolve_include_path(const char *base_file, const char *include_name) {
    char *result = malloc(512);
    if (!result) return NULL;

    // If include_name is absolute, use it directly
    if (include_name[0] == '/') {
        strncpy(result, include_name, 511);
        result[511] = '\0';
        return result;
    }

    // Get directory of base file
    char *base_copy = strdup(base_file);
    char *dir = dirname(base_copy);

    // Construct path relative to base file
    snprintf(result, 512, "%s/%s", dir, include_name);
    free(base_copy);

    return result;
}

static char *preprocess(const char *source, const char *filename, int depth);

static char *process_include_directive(const char *source, int *pos, const char *base_file, int depth) {
    // pos points after '{$'
    // Skip whitespace
    while (source[*pos] == ' ' || source[*pos] == '\t') {
        (*pos)++;
    }

    // Check for 'I' or 'INCLUDE'
    int p = *pos;
    if ((source[p] == 'I' || source[p] == 'i') &&
        (source[p+1] == ' ' || source[p+1] == '\t')) {
        p += 2;
    } else if (((source[p] == 'I' || source[p] == 'i') &&
                (source[p+1] == 'N' || source[p+1] == 'n') &&
                (source[p+2] == 'C' || source[p+2] == 'c') &&
                (source[p+3] == 'L' || source[p+3] == 'l') &&
                (source[p+4] == 'U' || source[p+4] == 'u') &&
                (source[p+5] == 'D' || source[p+5] == 'd') &&
                (source[p+6] == 'E' || source[p+6] == 'e')) &&
               (source[p+7] == ' ' || source[p+7] == '\t')) {
        p += 8;
    } else {
        // Not an include directive, return NULL to signal error/skip
        return NULL;
    }

    // Skip whitespace before filename
    while (source[p] == ' ' || source[p] == '\t') {
        p++;
    }

    // Extract filename (until '}')
    char filename_buf[256];
    int fn_len = 0;
    while (source[p] != '}' && source[p] != '\0' && fn_len < 255) {
        filename_buf[fn_len++] = source[p++];
    }
    // Trim trailing whitespace
    while (fn_len > 0 && (filename_buf[fn_len-1] == ' ' || filename_buf[fn_len-1] == '\t')) {
        fn_len--;
    }
    filename_buf[fn_len] = '\0';

    if (source[p] != '}') {
        fprintf(stderr, "Error: unterminated include directive\n");
        return NULL;
    }
    p++; // Skip '}'

    *pos = p;

    // Resolve the path
    char *include_path = resolve_include_path(base_file, filename_buf);
    if (!include_path) {
        fprintf(stderr, "Error: could not resolve include path '%s'\n", filename_buf);
        return NULL;
    }

    // Check for circular include
    if (file_already_included(include_path)) {
        fprintf(stderr, "Error: circular include detected: %s\n", include_path);
        free(include_path);
        return NULL;
    }

    // Read the include file
    char *include_source = read_file(include_path);
    if (!include_source) {
        fprintf(stderr, "Error: could not read include file '%s'\n", include_path);
        free(include_path);
        return NULL;
    }

    // Recursively preprocess the included content
    char *processed = preprocess(include_source, include_path, depth + 1);
    free(include_source);
    free(include_path);

    return processed;
}

// Helper to grow output buffer
static int ensure_capacity(char **output, size_t *capacity, size_t needed) {
    while (needed >= *capacity) {
        *capacity *= 2;
        *output = realloc(*output, *capacity);
        if (!*output) return 0;
    }
    return 1;
}

static char *preprocess(const char *source, const char *filename, int depth) {
    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Error: include depth exceeded (max %d)\n", MAX_INCLUDE_DEPTH);
        return NULL;
    }

    mark_file_included(filename);

    // Estimate output size (may grow with includes)
    size_t capacity = strlen(source) * 2 + 1;
    char *output = malloc(capacity);
    if (!output) return NULL;

    size_t out_pos = 0;
    int pos = 0;

    while (source[pos] != '\0') {
        // Check for '{' which could be comment or directive
        if (source[pos] == '{') {
            if (source[pos + 1] == '$') {
                // Potential directive - check if it's an include
                int save_pos = pos;
                pos += 2; // Skip '{$'

                char *included = process_include_directive(source, &pos, filename, depth);
                if (included) {
                    size_t inc_len = strlen(included);
                    if (!ensure_capacity(&output, &capacity, out_pos + inc_len + 1)) {
                        free(included);
                        return NULL;
                    }
                    memcpy(output + out_pos, included, inc_len);
                    out_pos += inc_len;
                    free(included);
                } else {
                    // Not a valid include directive - treat as unknown directive
                    // Copy the whole {$...} as-is
                    pos = save_pos;
                    if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
                    output[out_pos++] = source[pos++]; // '{'
                    while (source[pos] != '}' && source[pos] != '\0') {
                        if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
                        output[out_pos++] = source[pos++];
                    }
                    if (source[pos] == '}') {
                        output[out_pos++] = source[pos++];
                    }
                }
            } else {
                // Regular comment - must copy it but also scan for nested {$ within
                // Actually, Pascal doesn't nest {} comments, so just copy until }
                // BUT we need to handle {$I ...} if it appears at the start of a comment
                // A comment like { foo {$I bar} baz } - the {$I is not at top level
                // For simplicity: if { is followed by non-$, treat as regular comment
                if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
                output[out_pos++] = source[pos++]; // '{'
                // Now copy until closing }
                while (source[pos] != '}' && source[pos] != '\0') {
                    if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
                    output[out_pos++] = source[pos++];
                }
                if (source[pos] == '}') {
                    output[out_pos++] = source[pos++];
                }
            }
        }
        // Check for (* *) comment
        else if (source[pos] == '(' && source[pos + 1] == '*') {
            if (!ensure_capacity(&output, &capacity, out_pos + 3)) return NULL;
            output[out_pos++] = source[pos++]; // '('
            output[out_pos++] = source[pos++]; // '*'
            // Copy until *)
            while (!(source[pos] == '*' && source[pos + 1] == ')') && source[pos] != '\0') {
                if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
                output[out_pos++] = source[pos++];
            }
            if (source[pos] == '*') {
                output[out_pos++] = source[pos++]; // '*'
                output[out_pos++] = source[pos++]; // ')'
            }
        }
        // Check for string literals - don't process directives inside strings
        else if (source[pos] == '\'') {
            if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
            output[out_pos++] = source[pos++]; // opening quote
            while (source[pos] != '\0') {
                if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
                output[out_pos++] = source[pos];
                if (source[pos] == '\'') {
                    pos++;
                    // Check for escaped quote ''
                    if (source[pos] == '\'') {
                        output[out_pos++] = source[pos++];
                    } else {
                        break; // End of string
                    }
                } else {
                    pos++;
                }
            }
        }
        else {
            // Regular character
            if (!ensure_capacity(&output, &capacity, out_pos + 2)) return NULL;
            output[out_pos++] = source[pos++];
        }
    }

    output[out_pos] = '\0';
    return output;
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

    // Preprocess to expand include directives
    char *processed = preprocess(source, input_file, 0);
    free(source);
    if (!processed) {
        fprintf(stderr, "Preprocessing failed\n");
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, processed, input_file);

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
        free(processed);
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
            free(processed);
            return 1;
        }

        printf("Compiled %s -> %s\n", input_file, output_file);
    }

    parser_free(&parser);
    free(processed);

    // Free included file tracking
    for (int i = 0; i < included_count; i++) {
        free(included_files[i]);
    }

    return 0;
}
