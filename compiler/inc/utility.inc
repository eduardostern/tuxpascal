{ ----- Utility ----- }

{ Emit helpers - output multiple chars at once }
Procedure E2(a, b: Integer);
Begin WriteChar(a); WriteChar(b) End;

Procedure E3(a, b, c: Integer);
Begin WriteChar(a); WriteChar(b); WriteChar(c) End;

Procedure E4(a, b, c, d: Integer);
Begin WriteChar(a); WriteChar(b); WriteChar(c); WriteChar(d) End;

Procedure E5(a, b, c, d, e: Integer);
Begin WriteChar(a); WriteChar(b); WriteChar(c); WriteChar(d); WriteChar(e) End;

Procedure E6(a, b, c, d, e, f: Integer);
Begin WriteChar(a); WriteChar(b); WriteChar(c); WriteChar(d); WriteChar(e); WriteChar(f) End;

Procedure PrintToken;
Var
  i: Integer;
Begin
  For i := 0 To tok_len - 1 Do
    WriteChar(tok_str[i])
End;

Procedure Error(code: Integer);
Begin
  Write('Error: ');

  { Print descriptive error message }
  If code = 1 Then
    Write('Unexpected character')
  Else If code = 2 Then
    Write('Unexpected token')
  Else If code = 3 Then
  Begin
    Write('Undefined identifier ');
    WriteChar(39);
    PrintToken;
    WriteChar(39)
  End
  Else If code = 4 Then
    Write('Cannot call - not a procedure or function')
  Else If code = 5 Then
    Write('Statement expected')
  Else If code = 6 Then
    Write('Identifier expected')
  Else If code = 7 Then
    Write('Invalid procedure/function call')
  Else If code = 8 Then
    Write('Unit name expected')
  Else If code = 9 Then
    Write('Syntax error')
  Else If code = 10 Then
    Write('Invalid constant expression')
  Else If code = 11 Then
    Write('Identifier expected in declaration')
  Else If code = 12 Then
    Write('String expression expected')
  Else If code = 13 Then
    Write('MOD operator not supported for real numbers')
  Else If code = 14 Then
    Write('Type identifier expected')
  Else If code = 15 Then
  Begin
    Write('Undefined field ');
    WriteChar(39);
    PrintToken;
    WriteChar(39)
  End
  Else If code = 16 Then
    Write('Set type too large (max 64 elements)')
  Else If code = 17 Then
    Write('Include file nesting too deep (max 8 levels)')
  Else If code = 18 Then
    Write('Unit not found or invalid TPU file')
  Else If code = 19 Then
    Write('Type mismatch')
  Else If code = 20 Then
    Write('Duplicate identifier')
  Else
  Begin
    Write('Unknown error (code ');
    Write(code);
    Write(')')
  End;

  { Print location }
  Write(' at line ');
  WriteLn(line_num);

  Halt(1)
End;

Function IsDigit(c: Integer): Integer;
Begin
  If (c >= 48) And (c <= 57) Then
    IsDigit := 1
  Else
    IsDigit := 0
End;

Function IsAlpha(c: Integer): Integer;
Begin
  If ((c >= 65) And (c <= 90)) Or ((c >= 97) And (c <= 122)) Or (c = 95) Then
    IsAlpha := 1
  Else
    IsAlpha := 0
End;

Function ToLower(c: Integer): Integer;
Begin
  If (c >= 65) And (c <= 90) Then
    ToLower := c + 32
  Else
    ToLower := c
End;

Function StrEqual(idx: Integer): Integer;
Var
  i: Integer;
  match: Integer;
  base: Integer;
  c1, c2: Integer;
Begin
  { Compare tok_str With sym_name[idx] }
  { sym_name is flattened: base = idx * 32 }
  base := idx * 32;
  match := 1;
  i := 0;
  While (i < tok_len) And (match = 1) Do
  Begin
    c1 := tok_str[i];
    c2 := sym_name[base + i];
    If ToLower(c1) <> ToLower(c2) Then
      match := 0;
    i := i + 1
  End;
  If match = 1 Then
    If sym_name[base + tok_len] <> 0 Then
      match := 0;
  StrEqual := match
End;

{ Check If current token matches a String (Case insensitive) }
{ s1-s8 are ASCII codes Of the expected String, 0 marks End }
Function TokIs8(s1, s2, s3, s4, s5, s6, s7, s8: Integer): Integer;
Var
  i, match, slen: Integer;
  s: Array[0..7] Of Integer;
Begin
  s[0] := s1; s[1] := s2; s[2] := s3; s[3] := s4;
  s[4] := s5; s[5] := s6; s[6] := s7; s[7] := s8;
  { Find Length Of expected String }
  slen := 0;
  While (slen < 8) And (s[slen] <> 0) Do
    slen := slen + 1;
  { Check Length match }
  If tok_len <> slen Then
    match := 0
  Else
  Begin
    match := 1;
    i := 0;
    While (i < slen) And (match = 1) Do
    Begin
      If ToLower(tok_str[i]) <> ToLower(s[i]) Then
        match := 0;
      i := i + 1
    End
  End;
  TokIs8 := match
End;

