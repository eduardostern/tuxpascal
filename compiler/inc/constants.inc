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
  TOK_CARET = 24;    { ^ - dereference/pointer type }
  TOK_AT = 25;       { @ - address-of }

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
  TOK_REAL_TYPE = 133;
  TOK_FLOAT_LITERAL = 134;
  TOK_NIL = 135;     { nil keyword }
  TOK_RECORD = 136;  { record keyword }
  TOK_TYPE_KW = 137;    { type keyword }
  TOK_CASE = 138;    { case keyword }
  TOK_WITH = 139;    { with keyword }
  TOK_TEXT = 140;    { text file type }
  TOK_FILE = 141;    { file keyword }

  { Symbol kinds }
  SYM_VAR = 0;
  SYM_CONST = 1;
  SYM_PROCEDURE = 2;
  SYM_FUNCTION = 3;
  SYM_PARAM = 4;
  SYM_TYPEDEF = 5;       { type definition (for records) }

  { Type kinds }
  TYPE_INTEGER = 0;
  TYPE_CHAR = 1;
  TYPE_BOOLEAN = 2;
  TYPE_STRING = 3;
  TYPE_ARRAY = 4;
  TYPE_VOID = 5;
  TYPE_REAL = 6;
  TYPE_POINTER = 7;
  TYPE_RECORD = 8;
  TYPE_FILE = 9;      { file of T - typed file }
  TYPE_TEXT = 10;     { text file }

var
  { Source input }
  ch: integer;
  line_num, col_num: integer;
  pushback_ch: integer;  { -1 means no pushback }

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

  { Record field table }
  field_name: array[0..6399] of integer;   { 200 fields * 32 chars each }
  field_type: array[0..199] of integer;    { type of each field }
  field_offset: array[0..199] of integer;  { offset within record }
  field_rec_idx: array[0..199] of integer; { which record type this field belongs to }
  field_rec_type: array[0..199] of integer; { for TYPE_RECORD fields, the nested record type index }
  field_count: integer;                    { total fields defined }

  { Pointer metadata for multi-level pointers and pointer-to-array }
  ptr_depth: array[0..499] of integer;        { pointer indirection depth (1=^T, 2=^^T) }
  ptr_ultimate_type: array[0..499] of integer; { ultimate base type after all derefs }
  ptr_ultimate_rec: array[0..499] of integer;  { if ultimate base is record, the type index }
  ptr_arr_lo: array[0..99] of integer;     { low bound for pointer-to-array }
  ptr_arr_hi: array[0..99] of integer;     { high bound for pointer-to-array }
  ptr_arr_elem: array[0..99] of integer;   { element type for pointer-to-array }
  ptr_arr_rec: array[0..99] of integer;    { if element is record, the type index }
  ptr_arr_count: integer;                  { count of pointer-to-array types }

  { Runtime labels for allocator }
  rt_alloc: integer;
  rt_free: integer;

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
  rt_print_real: integer;
  rt_read_real: integer;
  rt_read_string: integer;

  { Float literal parsing }
  tok_float_int: integer;   { integer part of float }
  tok_float_frac: integer;  { fractional part (scaled by 1000000) }
  tok_float_neg: integer;   { 1 if negative }

  { Expression type tracking }
  expr_type: integer;

  { Pointer base type tracking for arithmetic }
  ptr_base_type: integer;

  { Output file descriptor - x20 is used to store it }
  out_fd: integer;

  { Error flag }
  had_error: integer;

  { WITH statement tracking }
  with_rec_idx: integer;    { symbol index of active with record, -1 if none }
  with_rec_type: integer;   { type index for field lookup }

  { Runtime labels for heap }
  rt_heap_init: integer;

  { Runtime labels for string operations }
  rt_str_copy: integer;
  rt_str_compare: integer;
  rt_str_concat: integer;
  rt_str_cmp: integer;  { lexicographic compare: returns -1, 0, or 1 }
  rt_str_pos: integer;  { find substring: returns position or 0 }
  rt_str_delete: integer;  { delete chars from string in place }
  rt_str_insert: integer;  { insert string into another }
  rt_int_to_str: integer;  { convert integer to string }
  rt_str_to_int: integer;  { convert string to integer with error code }
  rt_str_ltrim: integer;  { trim leading whitespace }
  rt_str_rtrim: integer;  { trim trailing whitespace }
  rt_str_trim: integer;   { trim both leading and trailing whitespace }

  { Runtime labels for screen/terminal control }
  rt_clrscr: integer;     { clear screen and home cursor }
  rt_gotoxy: integer;     { move cursor to x,y position }
  rt_clreol: integer;     { clear to end of line }
  rt_textcolor: integer;  { set foreground color }
  rt_textbackground: integer;  { set background color }
  rt_normvideo: integer;  { reset attributes }
  rt_highvideo: integer;  { bold/bright }
  rt_lowvideo: integer;   { dim }
  rt_hidecursor: integer; { hide cursor }
  rt_showcursor: integer; { show cursor }
  rt_sleep: integer;      { sleep for N milliseconds using nanosleep syscall }
  rt_keypressed: integer; { check if key available (non-blocking) }
  rt_initkeyboard: integer;  { set terminal to raw mode }
  rt_donekeyboard: integer;  { restore terminal to cooked mode }

  { Runtime labels for math functions }
  rt_sin: integer;     { sin(x) - Taylor series }
  rt_cos: integer;     { cos(x) - Taylor series }
  rt_tan: integer;     { tan(x) = sin/cos }
  rt_exp: integer;     { exp(x) - Taylor series }
  rt_ln: integer;      { ln(x) - Newton iteration }
  rt_random: integer;  { random - returns random integer }
  rt_arctan: integer;  { arctan(x) - Taylor series }
  rt_arcsin: integer;  { arcsin(x) - derived from arctan }
  rt_arccos: integer;  { arccos(x) - pi/2 - arcsin }

  { Saved terminal settings for restore }
  saved_termios: array[0..79] of integer;  { 80 bytes for termios struct }

  { String temp index (0-3) for copy/concat results }
  string_temp_idx: integer;

  { File I/O metadata - element type and size for typed files }
  file_elem_type: array[0..99] of integer;  { element type for file of T }
  file_elem_size: array[0..99] of integer;  { element size in bytes }
  file_rec_idx: array[0..99] of integer;    { if element is record, type index }
  file_count: integer;                      { count of file types defined }

  { File variable structure (at runtime, 272 bytes per file var):
    offset 0: fd (8 bytes) - file descriptor, -1 if not open
    offset 8: mode (8 bytes) - 0=closed, 1=read, 2=write, 3=append
    offset 16: filename (256 bytes) - null-terminated string }

