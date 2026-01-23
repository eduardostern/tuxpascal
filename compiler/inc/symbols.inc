{ ----- Symbol Table ----- }

Procedure CopyTokenToSym(idx: Integer);
Var
  i: Integer;
  base: Integer;
Begin
  { sym_name is flattened: base = idx * 32 }
  base := idx * 32;
  i := 0;
  While i < tok_len Do
  Begin
    sym_name[base + i] := tok_str[i];
    i := i + 1
  End;
  sym_name[base + tok_len] := 0
End;

Function SymLookup: Integer;
Var
  i: Integer;
  found: Integer;
Begin
  { Search backwards To find most recent definition }
  i := sym_count - 1;
  found := -1;
  While (i >= 0) And (found = -1) Do
  Begin
    If StrEqual(i) = 1 Then
      found := i;
    i := i - 1
  End;
  SymLookup := found
End;

Function SymAdd(kind, typ, level, offset: Integer): Integer;
Begin
  CopyTokenToSym(sym_count);
  sym_kind[sym_count] := kind;
  sym_type[sym_count] := typ;
  sym_level[sym_count] := level;
  sym_offset[sym_count] := offset;
  sym_label[sym_count] := 0;
  sym_const_val[sym_count] := 0;
  sym_is_var_param[sym_count] := 0;
  sym_var_param_flags[sym_count] := 0;
  sym_unit_idx[sym_count] := -1;  { -1 = local symbol, >= 0 = imported from Unit }
  sym_count := sym_count + 1;
  SymAdd := sym_count - 1
End;

Procedure PopScope(level: Integer);
Begin
  While (sym_count > 0) And (sym_level[sym_count - 1] >= level) Do
    sym_count := sym_count - 1
End;

{ Check If parameter position i is marked as Var In flags bitmap }
Function IsVarParam(flags, i: Integer): Integer;
Var
  bit_val, result: Integer;
Begin
  If i = 0 Then bit_val := 1
  Else If i = 1 Then bit_val := 2
  Else If i = 2 Then bit_val := 4
  Else If i = 3 Then bit_val := 8
  Else If i = 4 Then bit_val := 16
  Else If i = 5 Then bit_val := 32
  Else If i = 6 Then bit_val := 64
  Else bit_val := 128;
  result := (flags Div bit_val) Mod 2;
  IsVarParam := result
End;

{ Find a field In a Record Type, returns field index Or -1 }
Function FindField(type_idx: Integer): Integer;
Var
  i, j, base, match: Integer;
  first_fld: Integer;
Begin
  FindField := -1;
  first_fld := sym_const_val[type_idx];
  i := first_fld;
  While (i < field_count) And (field_rec_idx[i] = type_idx) Do
  Begin
    { Compare field name With current token }
    base := i * 32;
    match := 1;
    j := 0;
    While (j < tok_len) And (match = 1) Do
    Begin
      If ToLower(field_name[base + j]) <> ToLower(tok_str[j]) Then
        match := 0;
      j := j + 1
    End;
    If (match = 1) And (field_name[base + tok_len] = 0) Then
    Begin
      FindField := i;
      i := field_count  { Exit loop }
    End
    Else
      i := i + 1
  End
End;

