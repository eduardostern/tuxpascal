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
  sym_is_var_param[sym_count] := 0;
  sym_var_param_flags[sym_count] := 0;
  sym_count := sym_count + 1;
  SymAdd := sym_count - 1
end;

procedure PopScope(level: integer);
begin
  while (sym_count > 0) and (sym_level[sym_count - 1] >= level) do
    sym_count := sym_count - 1
end;

{ Check if parameter position i is marked as var in flags bitmap }
function IsVarParam(flags, i: integer): integer;
var
  bit_val, result: integer;
begin
  if i = 0 then bit_val := 1
  else if i = 1 then bit_val := 2
  else if i = 2 then bit_val := 4
  else if i = 3 then bit_val := 8
  else if i = 4 then bit_val := 16
  else if i = 5 then bit_val := 32
  else if i = 6 then bit_val := 64
  else bit_val := 128;
  result := (flags div bit_val) mod 2;
  IsVarParam := result
end;

{ Find a field in a record type, returns field index or -1 }
function FindField(type_idx: integer): integer;
var
  i, j, base, match: integer;
  first_fld: integer;
begin
  FindField := -1;
  first_fld := sym_const_val[type_idx];
  i := first_fld;
  while (i < field_count) and (field_rec_idx[i] = type_idx) do
  begin
    { Compare field name with current token }
    base := i * 32;
    match := 1;
    j := 0;
    while (j < tok_len) and (match = 1) do
    begin
      if ToLower(field_name[base + j]) <> ToLower(tok_str[j]) then
        match := 0;
      j := j + 1
    end;
    if (match = 1) and (field_name[base + tok_len] = 0) then
    begin
      FindField := i;
      i := field_count  { exit loop }
    end
    else
      i := i + 1
  end
end;

