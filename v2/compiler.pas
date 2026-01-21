{ TuxPascal v2 - Self-hosting Pascal Compiler }
{ Written in Pascal, compiled by v1 }

program TuxPascalV2;

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

  { Error flag }
  had_error: integer;

{ ----- Utility ----- }

procedure Error(msg: integer);
var
  i: integer;
begin
  write('Error ');
  write(msg);
  write(' at line ');
  write(line_num);
  write(' tok_type=');
  write(tok_type);
  write(' tok_len=');
  write(tok_len);
  write(' scope=');
  write(scope_level);
  write(' offset=');
  write(local_offset);
  write(' sym_count=');
  write(sym_count);
  write(' ch=');
  write(ch);
  write(' tok=');
  for i := 0 to tok_len - 1 do
    writechar(tok_str[i]);
  writeln(0);
  halt(1)
end;

function IsDigit(c: integer): integer;
begin
  if (c >= 48) and (c <= 57) then
    IsDigit := 1
  else
    IsDigit := 0
end;

function IsAlpha(c: integer): integer;
begin
  if ((c >= 65) and (c <= 90)) or ((c >= 97) and (c <= 122)) or (c = 95) then
    IsAlpha := 1
  else
    IsAlpha := 0
end;

function ToLower(c: integer): integer;
begin
  if (c >= 65) and (c <= 90) then
    ToLower := c + 32
  else
    ToLower := c
end;

function StrEqual(idx: integer): integer;
var
  i: integer;
  match: integer;
  base: integer;
  c1, c2: integer;
begin
  { Compare tok_str with sym_name[idx] }
  { sym_name is flattened: base = idx * 32 }
  base := idx * 32;
  match := 1;
  i := 0;
  while (i < tok_len) and (match = 1) do
  begin
    c1 := tok_str[i];
    c2 := sym_name[base + i];
    if ToLower(c1) <> ToLower(c2) then
      match := 0;
    i := i + 1
  end;
  if match = 1 then
    if sym_name[base + tok_len] <> 0 then
      match := 0;
  StrEqual := match
end;

{ Check if current token matches a string (case insensitive) }
{ s1-s8 are ASCII codes of the expected string, 0 marks end }
function TokIs8(s1, s2, s3, s4, s5, s6, s7, s8: integer): integer;
var
  i, match, slen: integer;
  s: array[0..7] of integer;
begin
  s[0] := s1; s[1] := s2; s[2] := s3; s[3] := s4;
  s[4] := s5; s[5] := s6; s[6] := s7; s[7] := s8;
  { Find length of expected string }
  slen := 0;
  while (slen < 8) and (s[slen] <> 0) do
    slen := slen + 1;
  { Check length match }
  if tok_len <> slen then
    match := 0
  else
  begin
    match := 1;
    i := 0;
    while (i < slen) and (match = 1) do
    begin
      if ToLower(tok_str[i]) <> ToLower(s[i]) then
        match := 0;
      i := i + 1
    end
  end;
  TokIs8 := match
end;

{ ----- Lexer ----- }

procedure NextChar;
begin
  ch := readchar;
  if ch = 10 then
  begin
    line_num := line_num + 1;
    col_num := 0
  end
  else
    col_num := col_num + 1
end;

procedure SkipWhitespace;
begin
  while (ch = 32) or (ch = 9) or (ch = 10) or (ch = 13) do
    NextChar;
  { Skip comments }
  if ch = 123 then  { '{' }
  begin
    while (ch <> 125) and (ch <> -1) do
      NextChar;
    if ch = 125 then
      NextChar;
    SkipWhitespace
  end
end;

procedure NextToken;
var
  i: integer;
begin
  SkipWhitespace;

  if ch = -1 then
  begin
    tok_type := TOK_EOF;
    tok_len := 0
  end
  else if IsDigit(ch) = 1 then
  begin
    tok_type := TOK_INTEGER;
    tok_int := 0;
    while IsDigit(ch) = 1 do
    begin
      tok_int := tok_int * 10 + (ch - 48);
      NextChar
    end
  end
  else if IsAlpha(ch) = 1 then
  begin
    tok_type := TOK_IDENT;
    tok_len := 0;
    while (IsAlpha(ch) = 1) or (IsDigit(ch) = 1) do
    begin
      if tok_len < 255 then
      begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      end;
      NextChar
    end;
    tok_str[tok_len] := 0;

    { Check for keywords }
    { This is simplified - would need proper keyword table }
    if tok_len = 7 then
      if (ToLower(tok_str[0]) = 112) and (ToLower(tok_str[1]) = 114) then { pr }
        if (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 103) then { og }
          if (ToLower(tok_str[4]) = 114) and (ToLower(tok_str[5]) = 97) then { ra }
            if ToLower(tok_str[6]) = 109 then { m }
              tok_type := TOK_PROGRAM;
    if tok_len = 5 then
      if (ToLower(tok_str[0]) = 98) and (ToLower(tok_str[1]) = 101) then { be }
        if (ToLower(tok_str[2]) = 103) and (ToLower(tok_str[3]) = 105) then { gi }
          if ToLower(tok_str[4]) = 110 then { n }
            tok_type := TOK_BEGIN;
    if tok_len = 3 then
    begin
      if (ToLower(tok_str[0]) = 101) and (ToLower(tok_str[1]) = 110) then { en }
        if ToLower(tok_str[2]) = 100 then { d }
          tok_type := TOK_END;
      if (ToLower(tok_str[0]) = 118) and (ToLower(tok_str[1]) = 97) then { va }
        if ToLower(tok_str[2]) = 114 then { r }
          tok_type := TOK_VAR;
      if (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 105) then { di }
        if ToLower(tok_str[2]) = 118 then { v }
          tok_type := TOK_DIV;
      if (ToLower(tok_str[0]) = 109) and (ToLower(tok_str[1]) = 111) then { mo }
        if ToLower(tok_str[2]) = 100 then { d }
          tok_type := TOK_MOD;
      if (ToLower(tok_str[0]) = 97) and (ToLower(tok_str[1]) = 110) then { an }
        if ToLower(tok_str[2]) = 100 then { d }
          tok_type := TOK_AND;
      if (ToLower(tok_str[0]) = 110) and (ToLower(tok_str[1]) = 111) then { no }
        if ToLower(tok_str[2]) = 116 then { t }
          tok_type := TOK_NOT;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 111) then { fo }
        if ToLower(tok_str[2]) = 114 then { r }
          tok_type := TOK_FOR
    end;
    if tok_len = 2 then
    begin
      if (ToLower(tok_str[0]) = 105) and (ToLower(tok_str[1]) = 102) then { if }
        tok_type := TOK_IF;
      if (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 111) then { do }
        tok_type := TOK_DO;
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 111) then { to }
        tok_type := TOK_TO;
      if (ToLower(tok_str[0]) = 111) and (ToLower(tok_str[1]) = 114) then { or }
        tok_type := TOK_OR;
      if (ToLower(tok_str[0]) = 111) and (ToLower(tok_str[1]) = 102) then { of }
        tok_type := TOK_OF
    end;
    if tok_len = 4 then
    begin
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 104) then { th }
        if (ToLower(tok_str[2]) = 101) and (ToLower(tok_str[3]) = 110) then { en }
          tok_type := TOK_THEN;
      if (ToLower(tok_str[0]) = 101) and (ToLower(tok_str[1]) = 108) then { el }
        if (ToLower(tok_str[2]) = 115) and (ToLower(tok_str[3]) = 101) then { se }
          tok_type := TOK_ELSE;
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 114) then { tr }
        if (ToLower(tok_str[2]) = 117) and (ToLower(tok_str[3]) = 101) then { ue }
          tok_type := TOK_TRUE
    end;
    if tok_len = 5 then
    begin
      if (ToLower(tok_str[0]) = 99) and (ToLower(tok_str[1]) = 111) then { co }
        if (ToLower(tok_str[2]) = 110) and (ToLower(tok_str[3]) = 115) then { ns }
          if ToLower(tok_str[4]) = 116 then { t }
            tok_type := TOK_CONST;
      if (ToLower(tok_str[0]) = 119) and (ToLower(tok_str[1]) = 104) then { wh }
        if (ToLower(tok_str[2]) = 105) and (ToLower(tok_str[3]) = 108) then { il }
          if ToLower(tok_str[4]) = 101 then { e }
            tok_type := TOK_WHILE;
      if (ToLower(tok_str[0]) = 117) and (ToLower(tok_str[1]) = 110) then { un }
        if (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 105) then { ti }
          if ToLower(tok_str[4]) = 108 then { l }
            tok_type := TOK_UNTIL;
      if (ToLower(tok_str[0]) = 97) and (ToLower(tok_str[1]) = 114) then { ar }
        if (ToLower(tok_str[2]) = 114) and (ToLower(tok_str[3]) = 97) then { ra }
          if ToLower(tok_str[4]) = 121 then { y }
            tok_type := TOK_ARRAY;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 97) then { fa }
        if (ToLower(tok_str[2]) = 108) and (ToLower(tok_str[3]) = 115) then { ls }
          if ToLower(tok_str[4]) = 101 then { e }
            tok_type := TOK_FALSE
    end;
    if tok_len = 6 then
    begin
      if (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) then { re }
        if (ToLower(tok_str[2]) = 112) and (ToLower(tok_str[3]) = 101) then { pe }
          if (ToLower(tok_str[4]) = 97) and (ToLower(tok_str[5]) = 116) then { at }
            tok_type := TOK_REPEAT;
      if (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 111) then { do }
        if (ToLower(tok_str[2]) = 119) and (ToLower(tok_str[3]) = 110) then { wn }
          if (ToLower(tok_str[4]) = 116) and (ToLower(tok_str[5]) = 111) then { to }
            tok_type := TOK_DOWNTO
    end;
    if tok_len = 7 then
    begin
      if (ToLower(tok_str[0]) = 105) and (ToLower(tok_str[1]) = 110) then { in }
        if (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 101) then { te }
          if (ToLower(tok_str[4]) = 103) and (ToLower(tok_str[5]) = 101) then { ge }
            if ToLower(tok_str[6]) = 114 then { r }
              tok_type := TOK_INTEGER_TYPE;
      if (ToLower(tok_str[0]) = 98) and (ToLower(tok_str[1]) = 111) then { bo }
        if (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 108) then { ol }
          if (ToLower(tok_str[4]) = 101) and (ToLower(tok_str[5]) = 97) then { ea }
            if ToLower(tok_str[6]) = 110 then { n }
              tok_type := TOK_BOOLEAN_TYPE;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 111) then { fo }
        if (ToLower(tok_str[2]) = 114) and (ToLower(tok_str[3]) = 119) then { rw }
          if (ToLower(tok_str[4]) = 97) and (ToLower(tok_str[5]) = 114) then { ar }
            if ToLower(tok_str[6]) = 100 then { d }
              tok_type := TOK_FORWARD
    end;
    if tok_len = 4 then
      if (ToLower(tok_str[0]) = 99) and (ToLower(tok_str[1]) = 104) then { ch }
        if (ToLower(tok_str[2]) = 97) and (ToLower(tok_str[3]) = 114) then { ar }
          tok_type := TOK_CHAR_TYPE;
    if tok_len = 9 then
      if (ToLower(tok_str[0]) = 112) and (ToLower(tok_str[1]) = 114) then { pr }
        if (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 99) then { oc }
          if (ToLower(tok_str[4]) = 101) and (ToLower(tok_str[5]) = 100) then { ed }
            if (ToLower(tok_str[6]) = 117) and (ToLower(tok_str[7]) = 114) then { ur }
              if ToLower(tok_str[8]) = 101 then { e }
                tok_type := TOK_PROCEDURE;
    if tok_len = 8 then
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 117) then { fu }
        if (ToLower(tok_str[2]) = 110) and (ToLower(tok_str[3]) = 99) then { nc }
          if (ToLower(tok_str[4]) = 116) and (ToLower(tok_str[5]) = 105) then { ti }
            if (ToLower(tok_str[6]) = 111) and (ToLower(tok_str[7]) = 110) then { on }
              tok_type := TOK_FUNCTION
  end
  else if ch = 39 then  { single quote - string }
  begin
    tok_type := TOK_STRING;
    tok_len := 0;
    NextChar;
    while (ch <> 39) and (ch <> -1) do
    begin
      if tok_len < 255 then
      begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      end;
      NextChar
    end;
    tok_str[tok_len] := 0;
    if ch = 39 then
      NextChar
  end
  else if ch = 43 then  { + }
  begin
    tok_type := TOK_PLUS;
    NextChar
  end
  else if ch = 45 then  { - }
  begin
    tok_type := TOK_MINUS;
    NextChar
  end
  else if ch = 42 then  { * }
  begin
    tok_type := TOK_STAR;
    NextChar
  end
  else if ch = 47 then  { / }
  begin
    tok_type := TOK_SLASH;
    NextChar
  end
  else if ch = 61 then  { = }
  begin
    tok_type := TOK_EQ;
    NextChar
  end
  else if ch = 60 then  { < }
  begin
    NextChar;
    if ch = 62 then  { <> }
    begin
      tok_type := TOK_NEQ;
      NextChar
    end
    else if ch = 61 then  { <= }
    begin
      tok_type := TOK_LE;
      NextChar
    end
    else
      tok_type := TOK_LT
  end
  else if ch = 62 then  { > }
  begin
    NextChar;
    if ch = 61 then  { >= }
    begin
      tok_type := TOK_GE;
      NextChar
    end
    else
      tok_type := TOK_GT
  end
  else if ch = 40 then  { ( }
  begin
    tok_type := TOK_LPAREN;
    NextChar
  end
  else if ch = 41 then  { ) }
  begin
    tok_type := TOK_RPAREN;
    NextChar
  end
  else if ch = 91 then  { [ }
  begin
    tok_type := TOK_LBRACKET;
    NextChar
  end
  else if ch = 93 then  { ] }
  begin
    tok_type := TOK_RBRACKET;
    NextChar
  end
  else if ch = 58 then  { : }
  begin
    NextChar;
    if ch = 61 then  { := }
    begin
      tok_type := TOK_ASSIGN;
      NextChar
    end
    else
      tok_type := TOK_COLON
  end
  else if ch = 59 then  { ; }
  begin
    tok_type := TOK_SEMICOLON;
    NextChar
  end
  else if ch = 44 then  { , }
  begin
    tok_type := TOK_COMMA;
    NextChar
  end
  else if ch = 46 then  { . }
  begin
    NextChar;
    if ch = 46 then  { .. }
    begin
      tok_type := TOK_DOTDOT;
      NextChar
    end
    else
      tok_type := TOK_DOT
  end
  else
  begin
    Error(1);  { unexpected character }
    NextChar
  end
end;

{ ----- Symbol Table ----- }

procedure CopyTokenToSym(idx: integer);
var
  i: integer;
  base: integer;
begin
  { sym_name is flattened: base = idx * 32 }
  base := idx * 32;
  i := 0;
  while i < tok_len do
  begin
    sym_name[base + i] := tok_str[i];
    i := i + 1
  end;
  sym_name[base + tok_len] := 0
end;

function SymLookup: integer;
var
  i: integer;
  found: integer;
begin
  { Search backwards to find most recent definition }
  i := sym_count - 1;
  found := -1;
  while (i >= 0) and (found = -1) do
  begin
    if StrEqual(i) = 1 then
      found := i;
    i := i - 1
  end;
  SymLookup := found
end;

function SymAdd(kind, typ, level, offset: integer): integer;
begin
  CopyTokenToSym(sym_count);
  sym_kind[sym_count] := kind;
  sym_type[sym_count] := typ;
  sym_level[sym_count] := level;
  sym_offset[sym_count] := offset;
  sym_label[sym_count] := 0;
  sym_const_val[sym_count] := 0;
  sym_count := sym_count + 1;
  SymAdd := sym_count - 1
end;

procedure PopScope(level: integer);
begin
  while (sym_count > 0) and (sym_level[sym_count - 1] >= level) do
    sym_count := sym_count - 1
end;

{ ----- Output Helpers ----- }

procedure EmitIndent;
begin
  writechar(32);
  writechar(32);
  writechar(32);
  writechar(32)
end;

procedure EmitNL;
begin
  writeln
end;

{ Emit specific strings character by character }
procedure EmitGlobl;
begin
  { .globl _main }
  writechar(46); writechar(103); writechar(108); writechar(111);
  writechar(98); writechar(108); writechar(32);
  writechar(95); writechar(109); writechar(97); writechar(105); writechar(110);
  EmitNL
end;

procedure EmitAlign4;
begin
  { .align 4 }
  writechar(46); writechar(97); writechar(108); writechar(105);
  writechar(103); writechar(110); writechar(32); writechar(52);
  EmitNL
end;

procedure EmitMain;
begin
  { _main: }
  writechar(95); writechar(109); writechar(97); writechar(105); writechar(110);
  writechar(58);
  EmitNL
end;

procedure EmitLabel(n: integer);
begin
  writechar(76);  { L }
  write(n);
  writechar(58);  { : }
  EmitNL
end;

function NewLabel: integer;
begin
  NewLabel := label_count;
  label_count := label_count + 1
end;

procedure EmitStp;
begin
  { stp x29, x30, [sp, #-16]! }
  EmitIndent;
  writechar(115); writechar(116); writechar(112); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(120); writechar(51); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49); writechar(54);
  writechar(93); writechar(33);
  EmitNL
end;

procedure EmitMovFP;
begin
  { mov x29, sp }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL
end;

procedure EmitLdp;
begin
  { ldp x29, x30, [sp], #16 }
  EmitIndent;
  writechar(108); writechar(100); writechar(112); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(120); writechar(51); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);
  EmitNL
end;

procedure EmitRet;
begin
  { ret }
  EmitIndent;
  writechar(114); writechar(101); writechar(116);
  EmitNL
end;

procedure EmitStoreStaticLink;
begin
  { stur x9, [x29, #-8] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL
end;

{ Emit code to set up static link in x9 before a call }
{ The callee was declared at sym_level, so its static link should point to frame at sym_level }
procedure EmitStaticLink(sym_level, cur_level: integer);
var
  i: integer;
begin
  { mov x9, x29 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { Follow static link chain to reach sym_level }
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(91); writechar(120); writechar(57); writechar(44); writechar(32);  { [x9, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end
end;

procedure EmitMovX0(val: integer);
var
  lo, hi: integer;
  neg: integer;
begin
  neg := 0;
  if val < 0 then
  begin
    neg := 1;
    val := 0 - val
  end;
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    { movz x0, #lo }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(35);
    write(lo);
    EmitNL;
    { movk x0, #hi, lsl #16 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(35);
    write(hi);
    writechar(44); writechar(32);
    writechar(108); writechar(115); writechar(108); writechar(32);
    writechar(35); writechar(49); writechar(54);
    EmitNL
  end
  else
  begin
    { mov x0, #val }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(35);
    write(val);
    EmitNL
  end;
  if neg = 1 then
  begin
    { neg x0, x0 }
    EmitIndent;
    writechar(110); writechar(101); writechar(103); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(120); writechar(48);
    EmitNL
  end
end;

procedure EmitMovX16(val: integer);
var
  lo, hi: integer;
begin
  { For large values like syscall numbers (0x2000001, etc), use movz+movk }
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    { movz x16, #lo }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);
    writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);
    writechar(35);
    write(lo);
    EmitNL;
    { movk x16, #hi, lsl #16 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);
    writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);
    writechar(35);
    write(hi);
    writechar(44); writechar(32);
    writechar(108); writechar(115); writechar(108); writechar(32);
    writechar(35); writechar(49); writechar(54);
    EmitNL
  end
  else
  begin
    { mov x16, #val }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);
    writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);
    writechar(35);
    write(val);
    EmitNL
  end
end;

procedure EmitMovX8(val: integer);
var
  lo, hi: integer;
  neg: integer;
begin
  { Handle negative values: negate, emit, then negate result }
  neg := 0;
  if val < 0 then
  begin
    neg := 1;
    val := 0 - val
  end;
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    { movz x8, #lo }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(35);
    write(lo);
    EmitNL;
    { movk x8, #hi, lsl #16 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(35);
    write(hi);
    writechar(44); writechar(32);
    writechar(108); writechar(115); writechar(108); writechar(32);
    writechar(35); writechar(49); writechar(54);
    EmitNL
  end
  else
  begin
    { mov x8, #val }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(35);
    write(val);
    EmitNL
  end;
  if neg = 1 then
  begin
    { neg x8, x8 }
    EmitIndent;
    writechar(110); writechar(101); writechar(103); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(120); writechar(56);
    EmitNL
  end
end;

{ Emit sub xD, xS, #offset where offset can be large }
{ If offset > 4095, loads into x10 first }
procedure EmitSubLargeOffset(dest, src, offset: integer);
var
  lo, hi: integer;
begin
  if offset <= 4095 then
  begin
    { sub xD, xS, #offset }
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
    writechar(120);
    if dest < 10 then
      writechar(48 + dest)
    else
    begin
      writechar(49);
      writechar(48 + dest - 10)
    end;
    writechar(44); writechar(32);
    writechar(120);
    if src < 10 then
      writechar(48 + src)
    else
    begin
      if src = 29 then
      begin
        writechar(50); writechar(57)
      end
      else
      begin
        writechar(49);
        writechar(48 + src - 10)
      end
    end;
    writechar(44); writechar(32);
    writechar(35);
    write(offset);
    EmitNL
  end
  else
  begin
    { Load offset into x10 }
    lo := offset mod 65536;
    hi := offset div 65536;
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);  { movz }
    writechar(120); writechar(49); writechar(48); writechar(44); writechar(32);  { x10, }
    writechar(35);
    write(lo);
    EmitNL;
    if hi > 0 then
    begin
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);  { movk }
      writechar(120); writechar(49); writechar(48); writechar(44); writechar(32);  { x10, }
      writechar(35);
      write(hi);
      writechar(44); writechar(32);
      writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL
    end;
    { sub xD, xS, x10 }
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
    writechar(120);
    if dest < 10 then
      writechar(48 + dest)
    else
    begin
      writechar(49);
      writechar(48 + dest - 10)
    end;
    writechar(44); writechar(32);
    writechar(120);
    if src < 10 then
      writechar(48 + src)
    else
    begin
      if src = 29 then
      begin
        writechar(50); writechar(57)
      end
      else
      begin
        writechar(49);
        writechar(48 + src - 10)
      end
    end;
    writechar(44); writechar(32);
    writechar(120); writechar(49); writechar(48);  { x10 }
    EmitNL
  end
end;

procedure EmitSvc;
begin
  { svc #0x80 }
  EmitIndent;
  writechar(115); writechar(118); writechar(99); writechar(32);
  writechar(35); writechar(48); writechar(120); writechar(56); writechar(48);
  EmitNL
end;

procedure EmitPushX0;
begin
  { str x0, [sp, #-16]! }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49); writechar(54);
  writechar(93); writechar(33);
  EmitNL
end;

procedure EmitPopX0;
begin
  { ldr x0, [sp], #16 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);
  EmitNL
end;

procedure EmitPopX1;
begin
  { ldr x1, [sp], #16 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);
  EmitNL
end;

procedure EmitAdd;
begin
  { add x0, x1, x0 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitSub;
begin
  { sub x0, x1, x0 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitMul;
begin
  { mul x0, x1, x0 }
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitSDiv;
begin
  { sdiv x0, x1, x0 }
  EmitIndent;
  writechar(115); writechar(100); writechar(105); writechar(118); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitBranchLabel(lbl: integer);
begin
  { b Lxx }
  EmitIndent;
  writechar(98); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitBranchLabelZ(lbl: integer);
begin
  { cbz x0, Lxx }
  EmitIndent;
  writechar(99); writechar(98); writechar(122); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitBranchLabelNZ(lbl: integer);
begin
  { cbnz x0, Lxx }
  EmitIndent;
  writechar(99); writechar(98); writechar(110); writechar(122); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitBL(lbl: integer);
begin
  { bl Lxx }
  EmitIndent;
  writechar(98); writechar(108); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitCmpX0X1;
begin
  { cmp x0, x1 - actually cmp x1, x0 for our stack order }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitCset(cond: integer);
begin
  { cset x0, <cond> }
  { cond: 0=eq, 1=ne, 2=lt, 3=le, 4=gt, 5=ge }
  EmitIndent;
  writechar(99); writechar(115); writechar(101); writechar(116); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  if cond = 0 then
  begin writechar(101); writechar(113) end  { eq }
  else if cond = 1 then
  begin writechar(110); writechar(101) end  { ne }
  else if cond = 2 then
  begin writechar(108); writechar(116) end  { lt }
  else if cond = 3 then
  begin writechar(108); writechar(101) end  { le }
  else if cond = 4 then
  begin writechar(103); writechar(116) end  { gt }
  else
  begin writechar(103); writechar(101) end; { ge }
  EmitNL
end;

procedure EmitLdurX0(offset: integer);
begin
  if (offset >= -255) and (offset <= 255) then
  begin
    { ldur x0, [x29, #offset] }
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(91); writechar(120); writechar(50); writechar(57);
    writechar(44); writechar(32); writechar(35);
    write(offset);
    writechar(93);
    EmitNL
  end
  else
  begin
    { Large offset: mov x8, #offset; add x8, x29, x8; ldr x0, [x8] }
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitIndent;
    writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

{ Load from outer scope - follow static link chain }
{ Static link is stored at [frame, #-8] }
procedure EmitLdurX0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  { Start with current frame pointer }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { Follow static link chain }
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end;
  { Now x8 points to the target frame }
  if (offset >= -255) and (offset <= 255) then
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56);  { [x8 }
    writechar(44); writechar(32); writechar(35);  { , # }
    write(offset);
    writechar(93);  { ] }
    EmitNL
  end
  else
  begin
    { Large offset: save x8 to x9, load offset to x8, add them }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitIndent;
    writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

{ Follow static link chain, leave target frame in x8 }
{ Static link is stored at [frame, #-8] }
procedure EmitFollowChain(sym_level, cur_level: integer);
var
  i: integer;
begin
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end
end;

{ Store to outer scope - follow saved frame pointer chain }
procedure EmitSturX0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  { Save x0 temporarily }
  EmitPushX0;
  { Start with current frame pointer }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { Follow static link chain }
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end;
  { Restore x0 }
  EmitPopX0;
  { Now x8 points to the target frame }
  if (offset >= -255) and (offset <= 255) then
  begin
    EmitIndent;
    writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56);  { [x8 }
    writechar(44); writechar(32); writechar(35);  { , # }
    write(offset);
    writechar(93);  { ] }
    EmitNL
  end
  else
  begin
    { Large offset: save x8 to x9, x0 to stack, load offset to x8, add, restore x0 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitPushX0;
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitPopX0;
    EmitIndent;
    writechar(115); writechar(116); writechar(114); writechar(32);  { str }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

procedure EmitSturX0(offset: integer);
begin
  if (offset >= -255) and (offset <= 255) then
  begin
    { stur x0, [x29, #offset] }
    EmitIndent;
    writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(91); writechar(120); writechar(50); writechar(57);
    writechar(44); writechar(32); writechar(35);
    write(offset);
    writechar(93);
    EmitNL
  end
  else
  begin
    { Large offset: mov x8, #offset; add x8, x29, x8; str x0, [x8] }
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitIndent;
    writechar(115); writechar(116); writechar(114); writechar(32);  { str }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

procedure EmitSubSP(n: integer);
begin
  if n <= 4095 then
  begin
    { sub sp, sp, #n }
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(35);
    write(n);
    EmitNL
  end
  else
  begin
    { Large value: mov x8, #n; sub sp, sp, x8 }
    EmitMovX8(n);
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(120); writechar(56);  { x8 }
    EmitNL
  end
end;

procedure EmitAddSP(n: integer);
begin
  if n <= 4095 then
  begin
    { add sp, sp, #n }
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(35);
    write(n);
    EmitNL
  end
  else
  begin
    { Large value: mov x8, #n; add sp, sp, x8 }
    EmitMovX8(n);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(120); writechar(56);  { x8 }
    EmitNL
  end
end;

procedure EmitNeg;
begin
  { neg x0, x0 }
  EmitIndent;
  writechar(110); writechar(101); writechar(103); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitMsub;
begin
  { msub x0, x0, x2, x1   (x0 = x1 - x0 * x2) for mod }
  EmitIndent;
  writechar(109); writechar(115); writechar(117); writechar(98); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(49);
  EmitNL
end;

procedure EmitMovX2X0;
begin
  { mov x2, x0 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitAndX0X1;
begin
  { and x0, x1, x0 }
  EmitIndent;
  writechar(97); writechar(110); writechar(100); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitOrrX0X1;
begin
  { orr x0, x1, x0 }
  EmitIndent;
  writechar(111); writechar(114); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitEorX0(val: integer);
begin
  { eor x0, x0, #val }
  EmitIndent;
  writechar(101); writechar(111); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35);
  write(val);
  EmitNL
end;

{ ----- Print Runtime ----- }

procedure EmitPrintIntRuntime;
var
  loop_lbl, done_lbl, neg_lbl, print_lbl: integer;
begin
  { Runtime routine to print integer in x0 }
  EmitLabel(rt_print_int);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { Save value }
  EmitSturX0(-24);

  { Handle negative }
  neg_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #0 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;

  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge Lxx }
  writechar(76); write(neg_lbl);
  EmitNL;

  { Print minus sign }
  EmitMovX0(1);
  EmitSturX0(-32);
  EmitMovX0(45);  { '-' }
  EmitSturX0(-8);
  EmitMovX16(33554436); { 0x2000004 }
  EmitMovX0(1);
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #8 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitSvc;

  { Negate }
  EmitLdurX0(-24);
  EmitNeg;
  EmitSturX0(-24);

  EmitLabel(neg_lbl);

  { Convert to string (digits in reverse) }
  EmitMovX0(0);
  EmitSturX0(-40);  { digit count }

  loop_lbl := NewLabel;
  print_lbl := NewLabel;

  EmitLabel(loop_lbl);
  EmitLdurX0(-24);
  EmitBranchLabelZ(print_lbl);

  { val % 10 }
  EmitLdurX0(-24);
  EmitPushX0;
  EmitMovX0(10);
  EmitPopX1;
  EmitSDiv;
  EmitMovX2X0;
  EmitLdurX0(-24);
  EmitPushX0;
  EmitMovX0(10);
  EmitPopX1;
  EmitMsub;

  { Store digit }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x0, x0, #48 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;

  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur x1, [x29, #-40] }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);
  EmitNL;

  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x2, x29, #48 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;

  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb w0, [x2, x1] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(93);
  EmitNL;

  { digit count++ }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x1, x1, #1 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;

  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur x1, [x29, #-40] }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);
  EmitNL;

  { val /= 10 }
  EmitLdurX0(-24);
  EmitPushX0;
  EmitMovX0(10);
  EmitPopX1;
  EmitSDiv;
  EmitSturX0(-24);

  EmitBranchLabel(loop_lbl);

  EmitLabel(print_lbl);

  { Handle zero }
  EmitLdurX0(-40);
  EmitBranchLabelNZ(done_lbl);
  EmitMovX0(48);  { '0' }
  EmitSturX0(-48);
  EmitMovX0(1);
  EmitSturX0(-40);

  EmitLabel(done_lbl);

  { Print digits in reverse order }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);
  EmitLdurX0(-40);
  EmitBranchLabelZ(done_lbl);

  { digit count-- }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x0, x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitSturX0(-40);

  { Load digit }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #48 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;

  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w0, [x1, x0] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(93);
  EmitNL;

  { Print char }
  EmitSturX0(-8);
  EmitMovX16(33554436);
  EmitMovX0(1);
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #8 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitSvc;

  EmitBranchLabel(loop_lbl);

  { Exit label }
  EmitLabel(done_lbl);

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitNewlineRuntime;
begin
  { Newline routine - print chr(10) }
  EmitLabel(rt_newline);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  EmitMovX0(10);
  EmitSturX0(-9);
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0(1);
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #9 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitReadcharRuntime;
begin
  { Readchar routine - read one char, return in x0 (-1 for EOF) }
  EmitLabel(rt_readchar);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  EmitMovX0(0);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read returned >= 1 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge Lxx }
  writechar(76); write(label_count);
  EmitNL;
  EmitMovX0(-1);  { EOF }
  EmitBranchLabel(label_count + 1);
  EmitLabel(label_count);
  label_count := label_count + 1;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w0, [sp] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  EmitLabel(label_count);
  label_count := label_count + 1;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitPrintCharRuntime;
begin
  { Print char routine - print char in x0 }
  EmitLabel(rt_print_char);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb w0, [sp] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0(1);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

{ ----- Parser ----- }

procedure ParseExpression; forward;
procedure ParseStatement; forward;

procedure Expect(t: integer);
begin
  if tok_type <> t then
    Error(2);
  NextToken
end;

function Match(t: integer): integer;
begin
  if tok_type = t then
  begin
    NextToken;
    Match := 1
  end
  else
    Match := 0
end;

procedure ParseFactor;
var
  idx, arg_count, i: integer;
begin
  if tok_type = TOK_INTEGER then
  begin
    EmitMovX0(tok_int);
    NextToken
  end
  else if tok_type = TOK_TRUE then
  begin
    EmitMovX0(1);
    NextToken
  end
  else if tok_type = TOK_FALSE then
  begin
    EmitMovX0(0);
    NextToken
  end
  else if tok_type = TOK_LPAREN then
  begin
    NextToken;
    ParseExpression;
    Expect(TOK_RPAREN)
  end
  else if tok_type = TOK_NOT then
  begin
    NextToken;
    ParseFactor;
    EmitEorX0(1)
  end
  else if tok_type = TOK_IDENT then
  begin
    { Check for built-in functions: readchar, ord, chr }
    { readchar = 114,101,97,100,99,104,97,114 }
    if TokIs8(114, 101, 97, 100, 99, 104, 97, 114) = 1 then
    begin
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        Expect(TOK_RPAREN)
      end;
      EmitBL(rt_readchar)
    end
    { ord = 111,114,100 }
    else if TokIs8(111, 114, 100, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN)
      { ord() is identity for integers/chars }
    end
    { chr = 99,104,114 }
    else if TokIs8(99, 104, 114, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN)
      { chr() is identity for integers/chars }
    end
    else
    begin
      idx := SymLookup;
      if idx < 0 then
        Error(3);  { undefined identifier }
      NextToken;
      if sym_kind[idx] = SYM_CONST then
        EmitMovX0(sym_const_val[idx])
      else if (sym_kind[idx] = SYM_VAR) or (sym_kind[idx] = SYM_PARAM) then
      begin
        if (sym_type[idx] = TYPE_ARRAY) and (tok_type = TOK_LBRACKET) then
        begin
          { Array element access: arr[index] }
          NextToken;  { consume '[' }
          ParseExpression;  { index in x0 }
          Expect(TOK_RBRACKET);
          { Subtract low bound }
          EmitPushX0;
          EmitMovX0(sym_const_val[idx]);  { low bound }
          EmitPopX1;
          { x0 = x1 - x0 = index - low_bound }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(48);  { x0 }
          EmitNL;
          { Multiply by 8 (element size) using lsl #3 }
          EmitIndent;
          writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(35); writechar(51);  { #3 }
          EmitNL;
          { Get base address: frame + offset (offset is negative) }
          if sym_level[idx] < scope_level then
          begin
            EmitFollowChain(sym_level[idx], scope_level);
            { sub x1, x8, #offset - may be large }
            EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
          end
          else
          begin
            { sub x1, x29, #offset - may be large }
            EmitSubLargeOffset(1, 29, 0 - sym_offset[idx])
          end;
          { Load from x1 - x0 (base - offset) }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(48);  { x0 }
          EmitNL;
          EmitIndent;
          writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
          EmitNL
        end
        else
        begin
          if sym_level[idx] < scope_level then
            EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          else
            EmitLdurX0(sym_offset[idx])
        end
      end
      else if sym_kind[idx] = SYM_FUNCTION then
      begin
        { Function call - pass args in x0-x7 }
        arg_count := 0;
        if tok_type = TOK_LPAREN then
        begin
          NextToken;
          if tok_type <> TOK_RPAREN then
          begin
            { Evaluate all args and push to stack }
            repeat
              if tok_type = TOK_COMMA then NextToken;
              ParseExpression;
              EmitPushX0;
              arg_count := arg_count + 1
            until tok_type <> TOK_COMMA
          end;
          Expect(TOK_RPAREN)
        end;
        { Pop args from stack into registers in reverse order }
        for i := arg_count - 1 downto 0 do
        begin
          EmitIndent;
          writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
          writechar(120); writechar(48 + i); writechar(44); writechar(32);  { xi, }
          writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
          writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);  { , #16 }
          EmitNL
        end;
        { Set up static link for callee }
        EmitStaticLink(sym_level[idx], scope_level);
        EmitBL(sym_label[idx])
      end
      else
        Error(4)
    end  { end of else for non-builtin ident }
  end  { end of else if tok_type = TOK_IDENT }
  else
    Error(5)
end;

procedure ParseUnary;
begin
  if tok_type = TOK_MINUS then
  begin
    NextToken;
    ParseFactor;
    EmitNeg
  end
  else if tok_type = TOK_PLUS then
  begin
    NextToken;
    ParseFactor
  end
  else
    ParseFactor
end;

procedure EmitPushX1;
begin
  { str x1, [sp, #-16]! }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49); writechar(54);
  writechar(93); writechar(33);
  EmitNL
end;

procedure ParseTerm;
var
  op: integer;
begin
  ParseUnary;
  while (tok_type = TOK_STAR) or (tok_type = TOK_DIV) or (tok_type = TOK_MOD) or (tok_type = TOK_AND) do
  begin
    op := tok_type;
    NextToken;
    EmitPushX0;
    ParseUnary;
    EmitPopX1;
    if op = TOK_STAR then
      EmitMul
    else if op = TOK_DIV then
      EmitSDiv
    else if op = TOK_MOD then
    begin
      { x1 mod x0: x1 - (x1 / x0) * x0 }
      EmitPushX0;
      EmitPushX1;
      EmitSDiv;
      EmitMovX2X0;
      EmitPopX1;
      EmitPopX0;
      EmitMsub
    end
    else { TOK_AND }
      EmitAndX0X1
  end
end;

procedure ParseSimpleExpr;
var
  op: integer;
begin
  ParseTerm;
  while (tok_type = TOK_PLUS) or (tok_type = TOK_MINUS) or (tok_type = TOK_OR) do
  begin
    op := tok_type;
    NextToken;
    EmitPushX0;
    ParseTerm;
    EmitPopX1;
    if op = TOK_PLUS then
      EmitAdd
    else if op = TOK_MINUS then
      EmitSub
    else { TOK_OR }
      EmitOrrX0X1
  end
end;

procedure ParseExpression;
var
  op, cond: integer;
begin
  ParseSimpleExpr;
  if (tok_type = TOK_EQ) or (tok_type = TOK_NEQ) or (tok_type = TOK_LT) or
     (tok_type = TOK_LE) or (tok_type = TOK_GT) or (tok_type = TOK_GE) then
  begin
    op := tok_type;
    NextToken;
    EmitPushX0;
    ParseSimpleExpr;
    EmitPopX1;
    EmitCmpX0X1;
    if op = TOK_EQ then cond := 0
    else if op = TOK_NEQ then cond := 1
    else if op = TOK_LT then cond := 2
    else if op = TOK_LE then cond := 3
    else if op = TOK_GT then cond := 4
    else cond := 5;
    EmitCset(cond)
  end
end;

procedure ParseStatement;
var
  idx, lbl1, lbl2, lbl3, arg_count, i: integer;
begin
  if tok_type = TOK_BEGIN then
  begin
    NextToken;
    ParseStatement;
    while tok_type = TOK_SEMICOLON do
    begin
      NextToken;
      ParseStatement
    end;
    Expect(TOK_END)
  end
  else if tok_type = TOK_IF then
  begin
    NextToken;
    ParseExpression;
    Expect(TOK_THEN);
    lbl1 := NewLabel;
    lbl2 := NewLabel;
    EmitBranchLabelZ(lbl1);
    ParseStatement;
    if tok_type = TOK_ELSE then
    begin
      EmitBranchLabel(lbl2);
      EmitLabel(lbl1);
      NextToken;
      ParseStatement;
      EmitLabel(lbl2)
    end
    else
      EmitLabel(lbl1)
  end
  else if tok_type = TOK_WHILE then
  begin
    lbl1 := NewLabel;
    lbl2 := NewLabel;
    EmitLabel(lbl1);
    NextToken;
    ParseExpression;
    Expect(TOK_DO);
    EmitBranchLabelZ(lbl2);
    ParseStatement;
    EmitBranchLabel(lbl1);
    EmitLabel(lbl2)
  end
  else if tok_type = TOK_REPEAT then
  begin
    lbl1 := NewLabel;
    EmitLabel(lbl1);
    NextToken;
    ParseStatement;
    while tok_type = TOK_SEMICOLON do
    begin
      NextToken;
      ParseStatement
    end;
    Expect(TOK_UNTIL);
    ParseExpression;
    EmitBranchLabelZ(lbl1)
  end
  else if tok_type = TOK_FOR then
  begin
    NextToken;
    if tok_type <> TOK_IDENT then
      Error(6);
    idx := SymLookup;
    if idx < 0 then
      Error(3);
    NextToken;
    Expect(TOK_ASSIGN);
    ParseExpression;
    EmitSturX0(sym_offset[idx]);

    lbl1 := NewLabel;
    lbl2 := NewLabel;

    if tok_type = TOK_TO then
    begin
      NextToken;
      ParseExpression;  { end value into x0 }
      EmitPushX0;       { save end value on stack }
      Expect(TOK_DO);
      EmitLabel(lbl1);
      EmitLdurX0(sym_offset[idx]);  { load loop var }
      { ldur x1, [sp] - load end value from stack }
      EmitIndent;
      writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
      EmitNL;
      EmitCmpX0X1;
      EmitCset(2);  { lt: exit when end < i, meaning i > end }
      EmitBranchLabelNZ(lbl2);
      ParseStatement;
      { increment }
      EmitLdurX0(sym_offset[idx]);
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add x0, x0, #1 }
      writechar(120); writechar(48); writechar(44); writechar(32);
      writechar(120); writechar(48); writechar(44); writechar(32);
      writechar(35); writechar(49);
      EmitNL;
      EmitSturX0(sym_offset[idx]);
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { Pop end value from stack }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL
    end
    else
    begin
      Expect(TOK_DOWNTO);
      ParseExpression;  { end value into x0 }
      EmitPushX0;       { save end value on stack }
      Expect(TOK_DO);
      EmitLabel(lbl1);
      EmitLdurX0(sym_offset[idx]);  { load loop var }
      { ldur x1, [sp] - load end value from stack }
      EmitIndent;
      writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
      EmitNL;
      EmitCmpX0X1;
      EmitCset(4);  { gt: exit when end > i, meaning i < end }
      EmitBranchLabelNZ(lbl2);
      ParseStatement;
      { decrement }
      EmitLdurX0(sym_offset[idx]);
      EmitIndent;
      writechar(115); writechar(117); writechar(98); writechar(32);  { sub x0, x0, #1 }
      writechar(120); writechar(48); writechar(44); writechar(32);
      writechar(120); writechar(48); writechar(44); writechar(32);
      writechar(35); writechar(49);
      EmitNL;
      EmitSturX0(sym_offset[idx]);
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { Pop end value from stack }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL
    end
  end
  else if tok_type = TOK_IDENT then
  begin
    { Check for built-in procedures first }
    { write = 119,114,105,116,101 }
    { writeln = 119,114,105,116,101,108,110 }
    { readchar = 114,101,97,100,99,104,97,114 }
    { writechar = 119,114,105,116,101,99,104,97 - actually too long, use 8 }
    { halt = 104,97,108,116 }
    if TokIs8(119, 114, 105, 116, 101, 108, 110, 0) = 1 then
    begin
      { writeln }
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        if tok_type <> TOK_RPAREN then
        begin
          repeat
            if tok_type = TOK_STRING then
            begin
              { Print string character by character }
              idx := 0;
              while idx < tok_len do
              begin
                EmitMovX0(tok_str[idx]);
                EmitBL(rt_print_char);
                idx := idx + 1
              end;
              NextToken
            end
            else
            begin
              ParseExpression;
              EmitBL(rt_print_int)
            end
          until tok_type <> TOK_COMMA;
          if tok_type = TOK_COMMA then NextToken
        end;
        Expect(TOK_RPAREN)
      end;
      EmitBL(rt_newline)
    end
    else if TokIs8(119, 114, 105, 116, 101, 0, 0, 0) = 1 then
    begin
      { write }
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        if tok_type <> TOK_RPAREN then
        begin
          repeat
            if tok_type = TOK_STRING then
            begin
              { Print string character by character }
              idx := 0;
              while idx < tok_len do
              begin
                EmitMovX0(tok_str[idx]);
                EmitBL(rt_print_char);
                idx := idx + 1
              end;
              NextToken
            end
            else
            begin
              ParseExpression;
              EmitBL(rt_print_int)
            end
          until tok_type <> TOK_COMMA;
          if tok_type = TOK_COMMA then NextToken
        end;
        Expect(TOK_RPAREN)
      end
    end
    else if TokIs8(104, 97, 108, 116, 0, 0, 0, 0) = 1 then
    begin
      { halt }
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        if tok_type <> TOK_RPAREN then
          ParseExpression
        else
          EmitMovX0(0);
        Expect(TOK_RPAREN)
      end
      else
        EmitMovX0(0);
      EmitMovX16(33554433);  { 0x2000001 = exit }
      EmitSvc
    end
    else if (tok_len = 9) and (tok_str[0] = 119) and (tok_str[1] = 114) and
            (tok_str[2] = 105) and (tok_str[3] = 116) and (tok_str[4] = 101) and
            (tok_str[5] = 99) and (tok_str[6] = 104) and (tok_str[7] = 97) and
            (tok_str[8] = 114) then
    begin
      { writechar - 119,114,105,116,101,99,104,97,114 }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      EmitBL(rt_print_char)
    end
    else if (tok_len = 8) and (tok_str[0] = 114) and (tok_str[1] = 101) and
            (tok_str[2] = 97) and (tok_str[3] = 100) and (tok_str[4] = 99) and
            (tok_str[5] = 104) and (tok_str[6] = 97) and (tok_str[7] = 114) then
    begin
      { readchar - 114,101,97,100,99,104,97,114 }
      NextToken;
      EmitBL(rt_readchar)
    end
    else
    begin
      { Not a built-in, look up in symbol table }
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      NextToken;

      if sym_kind[idx] = SYM_PROCEDURE then
      begin
        { Procedure call - pass args in x0-x7 }
        arg_count := 0;
        if tok_type = TOK_LPAREN then
        begin
          NextToken;
          if tok_type <> TOK_RPAREN then
          begin
            { Evaluate all args and push to stack }
            repeat
              if tok_type = TOK_COMMA then NextToken;
              ParseExpression;
              EmitPushX0;
              arg_count := arg_count + 1
            until tok_type <> TOK_COMMA
          end;
          Expect(TOK_RPAREN)
        end;
        { Pop args from stack into registers in reverse order }
        for i := arg_count - 1 downto 0 do
        begin
          EmitIndent;
          writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
          writechar(120); writechar(48 + i); writechar(44); writechar(32);  { xi, }
          writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
          writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);  { , #16 }
          EmitNL
        end;
        { Set up static link for callee }
        EmitStaticLink(sym_level[idx], scope_level);
        EmitBL(sym_label[idx])
      end
      else if (sym_kind[idx] = SYM_VAR) or (sym_kind[idx] = SYM_PARAM) then
      begin
        if (sym_type[idx] = TYPE_ARRAY) and (tok_type = TOK_LBRACKET) then
        begin
          { Array element assignment: arr[index] := expr }
          NextToken;  { consume '[' }
          ParseExpression;  { index in x0 }
          Expect(TOK_RBRACKET);
          { Save index on stack }
          EmitPushX0;
          Expect(TOK_ASSIGN);
          ParseExpression;  { value in x0 }
          { Save value, get index back }
          EmitPushX0;
          { x0 = value, x1 = index is what we need }
          { But stack has [value, index] so pop in reverse }
          EmitIndent;
          writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
          writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
          writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
          writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);  { , #16 }
          EmitNL;
          { x2 = value, now get index }
          EmitIndent;
          writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
          writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);  { , #16 }
          EmitNL;
          { x1 = index, x2 = value }
          { Subtract low bound from index }
          EmitMovX0(sym_const_val[idx]);  { low bound }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(48);  { x0 }
          EmitNL;
          { Multiply by 8 using lsl #3 }
          EmitIndent;
          writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(35); writechar(51);  { #3 }
          EmitNL;
          { Get base address: frame + offset (offset is negative) }
          if sym_level[idx] < scope_level then
          begin
            EmitFollowChain(sym_level[idx], scope_level);
            { sub x0, x8, #offset - may be large }
            EmitSubLargeOffset(0, 8, 0 - sym_offset[idx])
          end
          else
          begin
            { sub x0, x29, #offset - may be large }
            EmitSubLargeOffset(0, 29, 0 - sym_offset[idx])
          end;
          { Store at x0 - x1 }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(120); writechar(49);  { x1 }
          EmitNL;
          EmitIndent;
          writechar(115); writechar(116); writechar(114); writechar(32);  { str }
          writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
          writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
          EmitNL
        end
        else
        begin
          { Simple assignment }
          Expect(TOK_ASSIGN);
          ParseExpression;
          if sym_level[idx] < scope_level then
            EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          else
            EmitSturX0(sym_offset[idx])
        end
      end
      else if sym_kind[idx] = SYM_FUNCTION then
      begin
        { Function result assignment - store to result variable at -16 }
        Expect(TOK_ASSIGN);
        ParseExpression;
        EmitSturX0(-16)
      end
      else
        Error(7)
    end  { end of else for non-builtin identifier }
  end  { end of else if tok_type = TOK_IDENT }
end;

{ ----- Declarations ----- }

procedure ParseVarDeclarations;
var
  idx, first_idx, arr_size, lo_bound, hi_bound: integer;
begin
  NextToken;  { consume 'var' }
  while tok_type = TOK_IDENT do
  begin
    { Remember first var in a group for fixing up array size }
    local_offset := local_offset - 8;
    first_idx := SymAdd(SYM_VAR, TYPE_INTEGER, scope_level, local_offset);
    idx := first_idx;
    NextToken;
    while tok_type = TOK_COMMA do
    begin
      NextToken;
      if tok_type <> TOK_IDENT then
        Error(8);
      local_offset := local_offset - 8;
      idx := SymAdd(SYM_VAR, TYPE_INTEGER, scope_level, local_offset);
      NextToken
    end;
    Expect(TOK_COLON);
    { Parse type }
    if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
       (tok_type = TOK_BOOLEAN_TYPE) then
      NextToken
    else if tok_type = TOK_ARRAY then
    begin
      NextToken;
      Expect(TOK_LBRACKET);
      { Parse low bound }
      if tok_type = TOK_INTEGER then
      begin
        lo_bound := tok_int;
        NextToken
      end
      else
        Error(9);
      Expect(TOK_DOTDOT);
      { Parse high bound }
      if tok_type = TOK_INTEGER then
      begin
        hi_bound := tok_int;
        NextToken
      end
      else
        Error(9);
      Expect(TOK_RBRACKET);
      Expect(TOK_OF);
      { Parse element type }
      if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
         (tok_type = TOK_BOOLEAN_TYPE) then
        NextToken
      else
        Error(9);
      { Calculate array size and adjust offset }
      { Each element is 8 bytes for integer, but we'll use simpler 1-byte for char }
      { For now, use 8 bytes per element for simplicity }
      arr_size := (hi_bound - lo_bound + 1) * 8;
      { Adjust local_offset: we already allocated 8 bytes, need (arr_size - 8) more }
      local_offset := local_offset - (arr_size - 8);
      { Store array info: use sym_const_val for low bound, sym_label for size }
      sym_type[first_idx] := TYPE_ARRAY;
      sym_const_val[first_idx] := lo_bound;
      sym_label[first_idx] := arr_size
    end
    else
      Error(9);
    Expect(TOK_SEMICOLON)
  end
end;

procedure ParseConstDeclarations;
var
  idx: integer;
begin
  NextToken;  { consume 'const' }
  while tok_type = TOK_IDENT do
  begin
    idx := SymAdd(SYM_CONST, TYPE_INTEGER, scope_level, 0);
    NextToken;
    Expect(TOK_EQ);
    if tok_type = TOK_INTEGER then
    begin
      sym_const_val[idx] := tok_int;
      NextToken
    end
    else
      Error(10);
    Expect(TOK_SEMICOLON)
  end
end;

procedure ParseProcedureDeclaration; forward;
procedure ParseFunctionDeclaration; forward;

procedure ParseBlock;
var
  saved_offset: integer;
  alloc_size: integer;
  body_label: integer;
begin
  saved_offset := local_offset;
  body_label := 0;

  while (tok_type = TOK_CONST) or (tok_type = TOK_VAR) do
  begin
    if tok_type = TOK_CONST then
      ParseConstDeclarations
    else
      ParseVarDeclarations
  end;

  { If there are procedure/function declarations, jump over them }
  if (tok_type = TOK_PROCEDURE) or (tok_type = TOK_FUNCTION) then
  begin
    body_label := NewLabel;
    EmitBranchLabel(body_label)
  end;

  while (tok_type = TOK_PROCEDURE) or (tok_type = TOK_FUNCTION) do
  begin
    if tok_type = TOK_PROCEDURE then
      ParseProcedureDeclaration
    else
      ParseFunctionDeclaration
  end;

  if body_label > 0 then
    EmitLabel(body_label);

  { Allocate stack space - round up to 16 for alignment }
  alloc_size := 0;
  if local_offset < saved_offset then
  begin
    alloc_size := saved_offset - local_offset;
    alloc_size := ((alloc_size + 15) div 16) * 16;
    EmitSubSP(alloc_size)
  end;

  Expect(TOK_BEGIN);
  ParseStatement;
  while tok_type = TOK_SEMICOLON do
  begin
    NextToken;
    ParseStatement
  end;
  Expect(TOK_END);

  { Deallocate stack space }
  if alloc_size > 0 then
  begin
    EmitAddSP(alloc_size)
  end
end;

procedure ParseProcedureDeclaration;
var
  idx, proc_label: integer;
  saved_level, saved_offset: integer;
  param_count, param_idx, i: integer;
  param_indices: array[0..7] of integer;
begin
  NextToken;  { consume 'procedure' }

  if tok_type <> TOK_IDENT then
    Error(11);

  { Check if procedure already exists (forward declaration) }
  idx := SymLookup;
  if (idx >= 0) and (sym_kind[idx] = SYM_PROCEDURE) then
  begin
    { Reuse existing forward-declared procedure }
    proc_label := sym_label[idx]
  end
  else
  begin
    { Create new procedure symbol }
    idx := SymAdd(SYM_PROCEDURE, TYPE_VOID, scope_level, 0);
    proc_label := NewLabel;
    sym_label[idx] := proc_label
  end;
  NextToken;

  { Save state and enter new scope for parameters }
  saved_level := scope_level;
  saved_offset := local_offset;
  scope_level := scope_level + 1;
  local_offset := -8;  { Reserve -8 for static link }
  param_count := 0;

  { Handle optional parameters }
  if tok_type = TOK_LPAREN then
  begin
    NextToken;
    if tok_type <> TOK_RPAREN then
    begin
      repeat
        if tok_type = TOK_COMMA then NextToken;
        if tok_type <> TOK_IDENT then Error(11);
        { Add parameter as local variable }
        local_offset := local_offset - 8;
        param_idx := SymAdd(SYM_PARAM, TYPE_INTEGER, scope_level, local_offset);
        if param_count < 8 then
          param_indices[param_count] := param_idx;
        param_count := param_count + 1;
        NextToken;
        { Skip type annotation }
        if tok_type = TOK_COLON then
        begin
          NextToken;
          if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
             (tok_type = TOK_BOOLEAN_TYPE) then
            NextToken
        end
      until tok_type <> TOK_COMMA
    end;
    Expect(TOK_RPAREN)
  end;

  Expect(TOK_SEMICOLON);

  { Check for forward declaration }
  if tok_type = TOK_FORWARD then
  begin
    NextToken;
    Expect(TOK_SEMICOLON);
    PopScope(scope_level);
    scope_level := saved_level;
    local_offset := saved_offset
  end
  else
  begin

  { Emit procedure label and prolog - save x29, x30, static link }
  EmitLabel(proc_label);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);  { Allocate space for static link }
  EmitStoreStaticLink;

  { Allocate space for parameters and copy from registers }
  if param_count > 0 then
  begin
    EmitSubSP(((param_count * 8 + 15) div 16) * 16);
    for i := 0 to param_count - 1 do
    begin
      if i < 8 then
      begin
        { Store xi to [x29, #offset] }
        EmitIndent;
        writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
        writechar(120); writechar(48 + i); writechar(44); writechar(32);  { xi, }
        writechar(91); writechar(120); writechar(50); writechar(57);  { [x29 }
        writechar(44); writechar(32); writechar(35);  { , # }
        write(sym_offset[param_indices[i]]);
        writechar(93);  { ] }
        EmitNL
      end
    end
  end;

  { Parse procedure body }
  ParseBlock;

  { Pop local symbols and restore scope }
  PopScope(scope_level);
  scope_level := saved_level;
  local_offset := saved_offset;

  { Restore sp to frame pointer (undoes static link + params + local allocations) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;

  { Restore frame and return }
  EmitLdp;
  EmitIndent;
  writechar(114); writechar(101); writechar(116);  { ret }
  EmitNL;

  Expect(TOK_SEMICOLON)
  end  { end of else for non-forward declaration }
end;

procedure ParseFunctionDeclaration;
var
  idx, func_label: integer;
  saved_level, saved_offset: integer;
  param_count, param_idx, i: integer;
  param_indices: array[0..7] of integer;
begin
  NextToken;  { consume 'function' }

  if tok_type <> TOK_IDENT then
    Error(11);

  { Check if function already exists (forward declaration) }
  idx := SymLookup;
  if (idx >= 0) and (sym_kind[idx] = SYM_FUNCTION) then
  begin
    { Reuse existing forward-declared function }
    func_label := sym_label[idx]
  end
  else
  begin
    { Create new function symbol }
    idx := SymAdd(SYM_FUNCTION, TYPE_INTEGER, scope_level, 0);
    func_label := NewLabel;
    sym_label[idx] := func_label
  end;
  NextToken;

  { Save state and enter new scope for parameters }
  saved_level := scope_level;
  saved_offset := local_offset;
  scope_level := scope_level + 1;
  local_offset := -16;  { Reserve -8 for static link, -16 for result }
  param_count := 0;

  { Handle optional parameters }
  if tok_type = TOK_LPAREN then
  begin
    NextToken;
    if tok_type <> TOK_RPAREN then
    begin
      repeat
        if tok_type = TOK_COMMA then NextToken;
        if tok_type <> TOK_IDENT then Error(11);
        { Add parameter as local variable }
        local_offset := local_offset - 8;
        param_idx := SymAdd(SYM_PARAM, TYPE_INTEGER, scope_level, local_offset);
        if param_count < 8 then
          param_indices[param_count] := param_idx;
        param_count := param_count + 1;
        NextToken;
        { Skip type annotation }
        if tok_type = TOK_COLON then
        begin
          NextToken;
          if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
             (tok_type = TOK_BOOLEAN_TYPE) then
            NextToken
        end
      until tok_type <> TOK_COMMA
    end;
    Expect(TOK_RPAREN)
  end;

  { Parse return type }
  Expect(TOK_COLON);
  if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
     (tok_type = TOK_BOOLEAN_TYPE) then
    NextToken
  else
    Error(9);

  Expect(TOK_SEMICOLON);

  { Check for forward declaration }
  if tok_type = TOK_FORWARD then
  begin
    NextToken;
    Expect(TOK_SEMICOLON);
    PopScope(scope_level);
    scope_level := saved_level;
    local_offset := saved_offset
  end
  else
  begin

  { Emit function label and prolog - save x29, x30, static link }
  EmitLabel(func_label);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);  { Allocate space for static link }
  EmitStoreStaticLink;

  { Allocate space for parameters and copy from registers }
  if param_count > 0 then
  begin
    EmitSubSP(((param_count * 8 + 15) div 16) * 16);
    for i := 0 to param_count - 1 do
    begin
      if i < 8 then
      begin
        { Store xi to [x29, #offset] }
        EmitIndent;
        writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
        writechar(120); writechar(48 + i); writechar(44); writechar(32);  { xi, }
        writechar(91); writechar(120); writechar(50); writechar(57);  { [x29 }
        writechar(44); writechar(32); writechar(35);  { , # }
        write(sym_offset[param_indices[i]]);
        writechar(93);  { ] }
        EmitNL
      end
    end
  end;

  { Parse function body }
  ParseBlock;

  { Pop local symbols and restore scope }
  PopScope(scope_level);
  scope_level := saved_level;
  local_offset := saved_offset;

  { Load result from local variable into x0 }
  EmitLdurX0(-16);

  { Restore sp to frame pointer (undoes static link + params + local allocations) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;

  { Restore frame and return }
  EmitLdp;
  EmitIndent;
  writechar(114); writechar(101); writechar(116);  { ret }
  EmitNL;

  Expect(TOK_SEMICOLON)
  end  { end of else for non-forward declaration }
end;

procedure ParseProgram;
var
  main_lbl: integer;
begin
  Expect(TOK_PROGRAM);
  if tok_type <> TOK_IDENT then
    Error(11);
  NextToken;
  Expect(TOK_SEMICOLON);

  { Emit header }
  EmitGlobl;
  EmitAlign4;
  EmitMain;

  { Jump over runtime routines }
  main_lbl := NewLabel;
  EmitBranchLabel(main_lbl);

  { Emit runtime routines }
  rt_print_int := NewLabel;
  rt_newline := NewLabel;
  rt_readchar := NewLabel;
  rt_print_char := NewLabel;

  EmitPrintIntRuntime;
  EmitNewlineRuntime;
  EmitReadcharRuntime;
  EmitPrintCharRuntime;

  { Main program entry }
  EmitLabel(main_lbl);
  EmitStp;
  EmitMovFP;

  ParseBlock;

  Expect(TOK_DOT);

  { Exit syscall }
  EmitMovX0(0);
  EmitMovX16(33554433);  { 0x2000001 }
  EmitSvc
end;

{ ----- Main ----- }

begin
  { Initialize }
  line_num := 1;
  col_num := 0;
  sym_count := 0;
  scope_level := 0;
  local_offset := 0;
  label_count := 0;
  string_count := 0;
  had_error := 0;
  rt_print_int := 0;
  rt_newline := 0;
  rt_readchar := 0;
  rt_print_char := 0;

  { Read first character and token }
  NextChar;
  NextToken;

  { Compile the program }
  ParseProgram
end.
