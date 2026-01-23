Const
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
  TOK_CARET = 24;    { ^ - dereference/pointer Type }
  TOK_AT = 25;       { @ - address-Of }

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
  TOK_NIL = 135;     { Nil keyword }
  TOK_RECORD = 136;  { Record keyword }
  TOK_TYPE_KW = 137;    { Type keyword }
  TOK_CASE = 138;    { Case keyword }
  TOK_WITH = 139;    { With keyword }
  TOK_TEXT = 140;    { Text file Type }
  TOK_FILE = 141;    { file keyword }
  TOK_SET = 142;     { Set keyword }
  TOK_IN = 143;      { In keyword (Set membership) }
  TOK_UNIT = 144;    { Unit keyword }
  TOK_INTERFACE = 145;      { Interface keyword }
  TOK_IMPLEMENTATION = 146; { Implementation keyword }
  TOK_USES = 147;    { Uses keyword }

  { Symbol kinds }
  SYM_VAR = 0;
  SYM_CONST = 1;
  SYM_PROCEDURE = 2;
  SYM_FUNCTION = 3;
  SYM_PARAM = 4;
  SYM_TYPEDEF = 5;       { Type definition (For records) }

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
  TYPE_FILE = 9;      { file Of T - typed file }
  TYPE_TEXT = 10;     { Text file }
  TYPE_ENUM = 11;     { enumerated Type }
  TYPE_SUBRANGE = 12; { subrange Type }
  TYPE_SET = 13;      { Set Type }

Var
  { Source input }
  ch: Integer;
  line_num, col_num: Integer;
  pushback_ch: Integer;  { -1 means no pushback }

  { Current token }
  tok_type: Integer;
  tok_int: Integer;
  tok_str: Array[0..255] Of Integer;  { String as Array Of chars }
  tok_len: Integer;

  { Symbol table - flattened 2D Array: sym_name[idx * 32 + char_pos] }
  sym_name: Array[0..15999] Of Integer;  { 500 symbols * 32 chars each }
  sym_kind: Array[0..499] Of Integer;
  sym_type: Array[0..499] Of Integer;
  sym_level: Array[0..499] Of Integer;
  sym_offset: Array[0..499] Of Integer;
  sym_const_val: Array[0..499] Of Integer;
  sym_label: Array[0..499] Of Integer;
  sym_is_var_param: Array[0..499] Of Integer;  { 1 If Var parameter (pass by ref) }
  sym_var_param_flags: Array[0..499] Of Integer;  { bitmap: bit i = 1 If param i is Var (For proc/func) }
  sym_unit_idx: Array[0..499] Of Integer;  { Unit index For imported symbols, -1 For local }
  sym_count: Integer;

  { Record field table }
  field_name: Array[0..6399] Of Integer;   { 200 fields * 32 chars each }
  field_type: Array[0..199] Of Integer;    { Type Of each field }
  field_offset: Array[0..199] Of Integer;  { offset within Record }
  field_rec_idx: Array[0..199] Of Integer; { which Record Type this field belongs To }
  field_rec_type: Array[0..199] Of Integer; { For TYPE_RECORD fields, the nested Record Type index }
  field_count: Integer;                    { total fields defined }

  { Pointer metadata For multi-level pointers And pointer-To-Array }
  ptr_depth: Array[0..499] Of Integer;        { pointer indirection depth (1=^T, 2=^^T) }
  ptr_ultimate_type: Array[0..499] Of Integer; { ultimate base Type after all derefs }
  ptr_ultimate_rec: Array[0..499] Of Integer;  { If ultimate base is Record, the Type index }
  ptr_arr_lo: Array[0..99] Of Integer;     { low bound For pointer-To-Array }
  ptr_arr_hi: Array[0..99] Of Integer;     { high bound For pointer-To-Array }
  ptr_arr_elem: Array[0..99] Of Integer;   { element Type For pointer-To-Array }
  ptr_arr_rec: Array[0..99] Of Integer;    { If element is Record, the Type index }
  ptr_arr_count: Integer;                  { count Of pointer-To-Array types }

  { Runtime labels For allocator }
  rt_alloc: Integer;
  rt_free: Integer;
  rt_fillchar: Integer;
  rt_move: Integer;

  { Scope tracking }
  scope_level: Integer;
  local_offset: Integer;

  { Code generation }
  label_count: Integer;

  { String table - Not used yet, simplified }
  string_count: Integer;

  { Runtime labels }
  rt_print_int: Integer;
  rt_newline: Integer;
  rt_readchar: Integer;
  rt_print_char: Integer;
  rt_write_char_fd: Integer;  { Write single Char To file: x0=fd, x1=Char }
  rt_read_int: Integer;
  rt_skip_line: Integer;
  rt_print_string: Integer;
  rt_print_real: Integer;
  rt_read_real: Integer;
  rt_read_string: Integer;

  { Float literal parsing }
  tok_float_int: Integer;   { Integer part Of float }
  tok_float_frac: Integer;  { fractional part (scaled by 1000000) }
  tok_float_neg: Integer;   { 1 If negative }

  { Expression Type tracking }
  expr_type: Integer;

  { Pointer base Type tracking For arithmetic }
  ptr_base_type: Integer;

  { Output file descriptor - x20 is used To store it }
  out_fd: Integer;

  { Error flag }
  had_error: Integer;

  { With statement tracking }
  with_rec_idx: Integer;    { symbol index Of active With Record, -1 If none }
  with_rec_type: Integer;   { Type index For field lookup }

  { Loop control For Break/Continue }
  break_label: Integer;     { label To jump To For Break, 0 If Not In loop }
  continue_label: Integer;  { label To jump To For Continue, 0 If Not In loop }

  { Exit label For current Procedure/Function }
  exit_label: Integer;      { label To jump To For Exit, 0 If In main Program }

  { Runtime labels For heap }
  rt_heap_init: Integer;

  { Runtime labels For String operations }
  rt_str_copy: Integer;
  rt_str_compare: Integer;
  rt_str_concat: Integer;
  rt_str_cmp: Integer;  { lexicographic compare: returns -1, 0, Or 1 }
  rt_str_pos: Integer;  { find substring: returns position Or 0 }
  rt_str_delete: Integer;  { delete chars from String In place }
  rt_str_insert: Integer;  { insert String into another }
  rt_int_to_str: Integer;  { convert Integer To String }
  rt_str_to_int: Integer;  { convert String To Integer With error code }
  rt_str_ltrim: Integer;  { trim leading whitespace }
  rt_str_rtrim: Integer;  { trim trailing whitespace }
  rt_str_trim: Integer;   { trim both leading And trailing whitespace }

  { Runtime labels For screen/terminal control }
  rt_clrscr: Integer;     { clear screen And home cursor }
  rt_gotoxy: Integer;     { move cursor To x,y position }
  rt_clreol: Integer;     { clear To End Of line }
  rt_textcolor: Integer;  { Set foreground color }
  rt_textbackground: Integer;  { Set background color }
  rt_normvideo: Integer;  { reset attributes }
  rt_highvideo: Integer;  { bold/bright }
  rt_lowvideo: Integer;   { dim }
  rt_hidecursor: Integer; { hide cursor }
  rt_showcursor: Integer; { show cursor }
  rt_sleep: Integer;      { sleep For N milliseconds using nanosleep syscall }
  rt_keypressed: Integer; { check If key available (non-blocking) }
  rt_initkeyboard: Integer;  { Set terminal To raw mode }
  rt_donekeyboard: Integer;  { restore terminal To cooked mode }

  { Runtime labels For math functions }
  rt_sin: Integer;     { sin(x) - Taylor series }
  rt_cos: Integer;     { cos(x) - Taylor series }
  rt_tan: Integer;     { tan(x) = sin/cos }
  rt_exp: Integer;     { exp(x) - Taylor series }
  rt_ln: Integer;      { ln(x) - Newton iteration }
  rt_random: Integer;  { random - returns random Integer }
  rt_arctan: Integer;  { arctan(x) - Taylor series }
  rt_arcsin: Integer;  { arcsin(x) - derived from arctan }
  rt_arccos: Integer;  { arccos(x) - pi/2 - arcsin }

  { Runtime labels For command line }
  rt_paramstr: Integer;  { paramstr(n) - get command line argument as Pascal String }

  { Register usage:
    x25 = argc (saved at Program start)
    x26 = argv (saved at Program start)
    x27 = random seed }

  { Saved terminal settings For restore }
  saved_termios: Array[0..79] Of Integer;  { 80 bytes For termios struct }

  { String temp index (0-3) For copy/concat results }
  string_temp_idx: Integer;

  { File I/O metadata - element Type And size For typed files }
  file_elem_type: Array[0..99] Of Integer;  { element Type For file Of T }
  file_elem_size: Array[0..99] Of Integer;  { element size In bytes }
  file_rec_idx: Array[0..99] Of Integer;    { If element is Record, Type index }
  file_count: Integer;                      { count Of file types defined }

  { Enumerated Type metadata }
  enum_low: Array[0..99] Of Integer;        { first value (always 0) }
  enum_high: Array[0..99] Of Integer;       { last value }
  enum_count: Integer;                      { count Of enum types defined }

  { Subrange Type metadata }
  subr_low: Array[0..99] Of Integer;        { low bound }
  subr_high: Array[0..99] Of Integer;       { high bound }
  subr_base: Array[0..99] Of Integer;       { base Type (Integer, Char, enum) }
  subr_count: Integer;                      { count Of subrange types defined }

  { Set Type metadata - sets limited To 64 elements (fits In one register) }
  set_base: Array[0..99] Of Integer;        { base Type (Char, enum, subrange) }
  set_low: Array[0..99] Of Integer;         { low bound Of base Type }
  set_high: Array[0..99] Of Integer;        { high bound Of base Type }
  set_count: Integer;                       { count Of Set types defined }

  { File variable structure (at runtime, 272 bytes per file Var):
    offset 0: fd (8 bytes) - file descriptor, -1 If Not open
    offset 8: mode (8 bytes) - 0=closed, 1=Read, 2=Write, 3=append
    offset 16: filename (256 bytes) - null-terminated String }

  { Include file support - using individual variables since bootstrap doesn't support Array subscripts With eof }
  include_file0, include_file1, include_file2, include_file3: Text;
  include_file4, include_file5, include_file6, include_file7: Text;
  include_depth: Integer;                   { 0 = reading from stdin, >0 = In include }
  include_line: Array[0..7] Of Integer;     { Line number In each include file }
  include_col: Array[0..7] Of Integer;      { Column number In each include file }
  include_ch: Array[0..7] Of Integer;       { Current Char In each include file }
  include_pushback: Array[0..7] Of Integer; { Pushback Char For each include file }
  include_path: Array[0..2047] Of Integer;  { Paths Of included files (256 chars * 8 files) }

  { Unit/module support }
  compiling_unit: Integer;       { 0=Program, 1=Unit }
  in_interface: Integer;         { 1=In Interface section }
  current_unit_name: Array[0..31] Of Integer;  { Name Of current Unit being compiled }
  current_unit_len: Integer;

  { TPU file support }
  tpu_file: Text;                { File handle For reading/writing TPU }
  tpu_line: Array[0..1023] Of Integer;  { Buffer For reading TPU lines }
  tpu_line_len: Integer;         { Length Of current line }
  tpu_pos: Integer;              { Current parse position In tpu_line }

  { Interface symbol tracking - marks which symbols are exported }
  interface_start: Integer;      { First symbol index In Interface section }
  interface_end: Integer;        { Last symbol index In Interface section }

  { Loaded units tracking }
  loaded_units: Array[0..511] Of Integer;  { Unit names (16 units * 32 chars) }
  loaded_count: Integer;         { Number Of loaded units }
  unit_sym_start: Array[0..15] Of Integer;  { First symbol index For each Unit }
  unit_sym_end: Array[0..15] Of Integer;    { Last symbol index For each Unit }
  unit_init_label: Array[0..15] Of Integer; { Initialization label For each Unit }

