const
  MAX_TOKENS = 1000;
  MAX_SYMBOLS = 100;
  MAX_STRINGS = 64;
  MAX_NAME = 32;

  { Token types }
  TOK_EOF = 0;
  TOK_IDENT = 1;
  TOK_INTEGER = 2;
  TOK_STRING = 3;
  TOK_PLUS = 4;
  TOK_MINUS = 5;
  TOK_STAR = 6;
  TOK_SLASH = 7;
  TOK_EQ = 8;
  TOK_NEQ = 9;
  TOK_LT = 10;
  TOK_GT = 11;
  TOK_LE = 12;
  TOK_GE = 13;
  TOK_LPAREN = 14;
  TOK_RPAREN = 15;
  TOK_LBRACKET = 16;
  TOK_RBRACKET = 17;
  TOK_ASSIGN = 18;
  TOK_COLON = 19;
  TOK_SEMICOLON = 20;
  TOK_COMMA = 21;
  TOK_DOT = 22;
  TOK_DOTDOT = 23;

  { Keywords - starting at 100 }
  TOK_PROGRAM = 100;
  TOK_BEGIN = 101;
  TOK_END = 102;
  TOK_VAR = 103;
  TOK_CONST = 104;
  TOK_PROCEDURE = 105;
  TOK_FUNCTION = 106;
  TOK_IF = 107;
  TOK_THEN = 108;
  TOK_ELSE = 109;
  TOK_WHILE = 110;
  TOK_DO = 111;
  TOK_REPEAT = 112;
  TOK_UNTIL = 113;
  TOK_FOR = 114;
  TOK_TO = 115;
  TOK_DOWNTO = 116;
  TOK_ARRAY = 117;
  TOK_OF = 118;
  TOK_DIV = 119;
  TOK_MOD = 120;
  TOK_AND = 121;
  TOK_OR = 122;
  TOK_NOT = 123;
  TOK_TRUE = 124;
  TOK_FALSE = 125;
  TOK_INTEGER_TYPE = 126;
  TOK_CHAR_TYPE = 127;
  TOK_BOOLEAN_TYPE = 128;
  TOK_STRING_TYPE = 129;
  TOK_FORWARD = 130;
  TOK_READ = 131;
  TOK_READLN = 132;

  { Symbol kinds }
  SYM_VAR = 0;
  SYM_CONST = 1;
  SYM_PROCEDURE = 2;
  SYM_FUNCTION = 3;
  SYM_PARAM = 4;

  { Type kinds }
  TYPE_INTEGER = 0;
  TYPE_CHAR = 1;
  TYPE_BOOLEAN = 2;
  TYPE_STRING = 3;
  TYPE_ARRAY = 4;
  TYPE_VOID = 5;

var
  { Source input }
  ch: integer;
  line_num, col_num: integer;

  { Current token }
  tok_type: integer;
  tok_int: integer;
  tok_str: array[0..255] of integer;  { string as array of chars }
  tok_len: integer;

  { Symbol table - flattened 2D array: sym_name[idx * 32 + char_pos] }
  sym_name: array[0..15999] of integer;  { 500 symbols * 32 chars each }
  sym_kind: array[0..499] of integer;
  sym_type: array[0..499] of integer;
  sym_level: array[0..499] of integer;
  sym_offset: array[0..499] of integer;
  sym_const_val: array[0..499] of integer;
  sym_label: array[0..499] of integer;
  sym_is_var_param: array[0..499] of integer;  { 1 if var parameter (pass by ref) }
  sym_var_param_flags: array[0..499] of integer;  { bitmap: bit i = 1 if param i is var (for proc/func) }
  sym_count: integer;

  { Scope tracking }
  scope_level: integer;
  local_offset: integer;

  { Code generation }
  label_count: integer;

  { String table - not used yet, simplified }
  string_count: integer;

  { Runtime labels }
  rt_print_int: integer;
  rt_newline: integer;
  rt_readchar: integer;
  rt_print_char: integer;
  rt_read_int: integer;
  rt_skip_line: integer;
  rt_print_string: integer;

  { Output file descriptor - x20 is used to store it }
  out_fd: integer;

  { Error flag }
  had_error: integer;

