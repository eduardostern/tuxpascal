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

Procedure Error(msg: Integer);
Var
  i: Integer;
Begin
  Write('Error ');
  Write(msg);
  Write(' at line ');
  Write(line_num);
  Write(' tok_type=');
  Write(tok_type);
  Write(' tok_len=');
  Write(tok_len);
  Write(' scope=');
  Write(scope_level);
  Write(' offset=');
  Write(local_offset);
  Write(' sym_count=');
  Write(sym_count);
  Write(' ch=');
  Write(ch);
  Write(' tok=');
  For i := 0 To tok_len - 1 Do
    WriteChar(tok_str[i]);
  WriteLn(0);
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

