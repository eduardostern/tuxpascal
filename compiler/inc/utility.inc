{ ----- Utility ----- }

{ Emit helpers - output multiple chars at once }
procedure E2(a, b: integer);
begin writechar(a); writechar(b) end;

procedure E3(a, b, c: integer);
begin writechar(a); writechar(b); writechar(c) end;

procedure E4(a, b, c, d: integer);
begin writechar(a); writechar(b); writechar(c); writechar(d) end;

procedure E5(a, b, c, d, e: integer);
begin writechar(a); writechar(b); writechar(c); writechar(d); writechar(e) end;

procedure E6(a, b, c, d, e, f: integer);
begin writechar(a); writechar(b); writechar(c); writechar(d); writechar(e); writechar(f) end;

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

