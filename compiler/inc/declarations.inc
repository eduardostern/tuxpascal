{ ----- Declarations ----- }

Procedure ParseVarDeclarations;
Var
  idx, first_idx, arr_size, lo_bound, hi_bound, j, base_idx: Integer;
Begin
  NextToken;  { consume 'Var' }
  While tok_type = TOK_IDENT Do
  Begin
    { Remember first Var In a group For fixing up Array size }
    local_offset := local_offset - 8;
    first_idx := SymAdd(SYM_VAR, TYPE_INTEGER, scope_level, local_offset);
    idx := first_idx;
    NextToken;
    While tok_type = TOK_COMMA Do
    Begin
      NextToken;
      If tok_type <> TOK_IDENT Then
        Error(8);
      local_offset := local_offset - 8;
      idx := SymAdd(SYM_VAR, TYPE_INTEGER, scope_level, local_offset);
      NextToken
    End;
    Expect(TOK_COLON);
    { Parse Type }
    If (tok_type = TOK_INTEGER_TYPE) Or (tok_type = TOK_CHAR_TYPE) Or
       (tok_type = TOK_BOOLEAN_TYPE) Then
      NextToken
    Else If tok_type = TOK_REAL_TYPE Then
    Begin
      { Set Type To Real For all vars In this group }
      For j := first_idx To idx Do
        sym_type[j] := TYPE_REAL;
      NextToken
    End
    Else If tok_type = TOK_ARRAY Then
    Begin
      NextToken;
      Expect(TOK_LBRACKET);
      { Parse low bound }
      If tok_type = TOK_INTEGER Then
      Begin
        lo_bound := tok_int;
        NextToken
      End
      Else
        Error(9);
      Expect(TOK_DOTDOT);
      { Parse high bound }
      If tok_type = TOK_INTEGER Then
      Begin
        hi_bound := tok_int;
        NextToken
      End
      Else
        Error(9);
      Expect(TOK_RBRACKET);
      Expect(TOK_OF);
      { Parse element Type }
      If (tok_type = TOK_INTEGER_TYPE) Or (tok_type = TOK_CHAR_TYPE) Or
         (tok_type = TOK_BOOLEAN_TYPE) Then
      Begin
        NextToken;
        arr_size := (hi_bound - lo_bound + 1) * 8;
        local_offset := local_offset - (arr_size - 8);
        sym_type[first_idx] := TYPE_ARRAY;
        sym_const_val[first_idx] := lo_bound;
        sym_label[first_idx] := arr_size;
        sym_var_param_flags[first_idx] := 0  { 0 = basic Type element }
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        { Array Of Record Type }
        base_idx := SymLookup;
        If (base_idx >= 0) And (sym_kind[base_idx] = SYM_TYPEDEF) And (sym_type[base_idx] = TYPE_RECORD) Then
        Begin
          arr_size := (hi_bound - lo_bound + 1) * sym_label[base_idx];
          local_offset := local_offset - (arr_size - 8);
          sym_type[first_idx] := TYPE_ARRAY;
          sym_const_val[first_idx] := lo_bound;
          sym_label[first_idx] := arr_size;
          sym_var_param_flags[first_idx] := base_idx + 1;  { Record Type index + 1 (0 means basic) }
          NextToken
        End
        Else
          Error(9)
      End
      Else
        Error(9)
    End
    Else If tok_type = TOK_STRING_TYPE Then
    Begin
      { String Type: 256 bytes (1 Length byte + 255 Char bytes) }
      NextToken;
      { Handle all variables In the list }
      For j := first_idx To idx Do
      Begin
        sym_type[j] := TYPE_STRING;
        sym_label[j] := 256;  { Store size For reference }
        { Adjust offset: already allocated 8 bytes, need 248 more }
        If j = first_idx Then
        Begin
          local_offset := local_offset - 248;
          sym_offset[j] := local_offset  { Update offset To start Of 256-byte area }
        End
        Else
        Begin
          local_offset := local_offset - 256;
          sym_offset[j] := local_offset
        End
      End
    End
    Else If tok_type = TOK_TEXT Then
    Begin
      { Text file Type: 272 bytes (fd + mode + filename) }
      NextToken;
      For j := first_idx To idx Do
      Begin
        sym_type[j] := TYPE_TEXT;
        sym_const_val[j] := TYPE_CHAR;  { element Type is Char }
        sym_label[j] := 1;  { element size is 1 byte }
        { Adjust offset: already allocated 8 bytes, need 264 more For 272 total }
        If j = first_idx Then
        Begin
          local_offset := local_offset - 264;
          sym_offset[j] := local_offset
        End
        Else
        Begin
          local_offset := local_offset - 272;
          sym_offset[j] := local_offset
        End
      End
    End
    Else If tok_type = TOK_FILE Then
    Begin
      { Typed file: file Of T - 272 bytes (fd + mode + filename) }
      NextToken;
      Expect(TOK_OF);
      { Parse element Type }
      If tok_type = TOK_INTEGER_TYPE Then
      Begin
        base_idx := TYPE_INTEGER;
        arr_size := 8;
        NextToken
      End
      Else If tok_type = TOK_CHAR_TYPE Then
      Begin
        base_idx := TYPE_CHAR;
        arr_size := 1;
        NextToken
      End
      Else If tok_type = TOK_BOOLEAN_TYPE Then
      Begin
        base_idx := TYPE_BOOLEAN;
        arr_size := 1;
        NextToken
      End
      Else If tok_type = TOK_REAL_TYPE Then
      Begin
        base_idx := TYPE_REAL;
        arr_size := 8;
        NextToken
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        { Record Type }
        lo_bound := SymLookup;
        If (lo_bound >= 0) And (sym_kind[lo_bound] = SYM_TYPEDEF) And (sym_type[lo_bound] = TYPE_RECORD) Then
        Begin
          base_idx := TYPE_RECORD;
          arr_size := sym_label[lo_bound];  { Record size }
          file_rec_idx[file_count] := lo_bound;
          NextToken
        End
        Else
          Error(14)
      End
      Else
        Error(9);
      { Store file Type info }
      file_elem_type[file_count] := base_idx;
      file_elem_size[file_count] := arr_size;
      For j := first_idx To idx Do
      Begin
        sym_type[j] := TYPE_FILE;
        sym_const_val[j] := file_count;  { index into file arrays }
        sym_label[j] := arr_size;  { element size }
        { Adjust offset For 272 bytes }
        If j = first_idx Then
        Begin
          local_offset := local_offset - 264;
          sym_offset[j] := local_offset
        End
        Else
        Begin
          local_offset := local_offset - 272;
          sym_offset[j] := local_offset
        End
      End;
      file_count := file_count + 1
    End
    Else If tok_type = TOK_SET Then
    Begin
      { Inline Set Type: Set Of Char / Set Of 0..63 }
      NextToken;  { consume 'Set' }
      Expect(TOK_OF);
      If tok_type = TOK_CHAR_TYPE Then
      Begin
        { Set Of Char - Uses 64-bit bitmask }
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_SET;
          sym_const_val[j] := set_count
        End;
        set_base[set_count] := TYPE_CHAR;
        set_low[set_count] := 0;
        set_high[set_count] := 63;  { limited To 64 elements }
        set_count := set_count + 1;
        NextToken
      End
      Else If tok_type = TOK_INTEGER Then
      Begin
        { Set Of 0..N - inline subrange }
        lo_bound := tok_int;
        NextToken;
        Expect(TOK_DOTDOT);
        If tok_type <> TOK_INTEGER Then Error(9);
        hi_bound := tok_int;
        NextToken;
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_SET;
          sym_const_val[j] := set_count
        End;
        set_base[set_count] := TYPE_INTEGER;
        set_low[set_count] := lo_bound;
        set_high[set_count] := hi_bound;
        set_count := set_count + 1
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        { Set Of EnumType }
        base_idx := SymLookup;
        If (base_idx >= 0) And (sym_kind[base_idx] = SYM_TYPEDEF) And
           (sym_type[base_idx] = TYPE_ENUM) Then
        Begin
          For j := first_idx To idx Do
          Begin
            sym_type[j] := TYPE_SET;
            sym_const_val[j] := set_count
          End;
          set_base[set_count] := TYPE_ENUM;
          set_low[set_count] := 0;
          set_high[set_count] := sym_label[base_idx] - 1;  { enum count - 1 }
          set_count := set_count + 1;
          NextToken
        End
        Else
          Error(9)
      End
      Else
        Error(9)
    End
    Else If tok_type = TOK_CARET Then
    Begin
      { Pointer Type: ^BaseType Or ^^BaseType Or ^Array[lo..hi] Of T }
      NextToken;
      lo_bound := 1;  { reuse lo_bound For depth count }
      While tok_type = TOK_CARET Do
      Begin
        lo_bound := lo_bound + 1;
        NextToken
      End;
      { Check For pointer To Array }
      If tok_type = TOK_ARRAY Then
      Begin
        { ^Array[lo..hi] Of T }
        NextToken;  { consume 'Array' }
        Expect(TOK_LBRACKET);
        If tok_type <> TOK_INTEGER Then
          Error(9);
        hi_bound := tok_int;  { reuse hi_bound temporarily For low }
        NextToken;
        Expect(TOK_DOTDOT);
        If tok_type <> TOK_INTEGER Then
          Error(9);
        arr_size := tok_int;  { high bound }
        NextToken;
        Expect(TOK_RBRACKET);
        Expect(TOK_OF);
        { Parse element Type }
        If tok_type = TOK_INTEGER_TYPE Then
          base_idx := TYPE_INTEGER
        Else If tok_type = TOK_CHAR_TYPE Then
          base_idx := TYPE_CHAR
        Else If tok_type = TOK_BOOLEAN_TYPE Then
          base_idx := TYPE_BOOLEAN
        Else If tok_type = TOK_REAL_TYPE Then
          base_idx := TYPE_REAL
        Else
          base_idx := TYPE_INTEGER;  { default }
        { Store In ptr_arr arrays }
        ptr_arr_lo[ptr_arr_count] := hi_bound;
        ptr_arr_hi[ptr_arr_count] := arr_size;
        ptr_arr_elem[ptr_arr_count] := base_idx;
        ptr_arr_rec[ptr_arr_count] := 0;
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_POINTER;
          sym_const_val[j] := TYPE_ARRAY;  { immediate base is Array }
          sym_label[j] := ptr_arr_count;   { index into ptr_arr arrays }
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_ARRAY;
          ptr_ultimate_rec[j] := 0
        End;
        ptr_arr_count := ptr_arr_count + 1;
        NextToken
      End
      { Now parse ultimate base Type }
      Else If tok_type = TOK_INTEGER_TYPE Then
      Begin
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_POINTER;
          If lo_bound = 1 Then
            sym_const_val[j] := TYPE_INTEGER
          Else
            sym_const_val[j] := TYPE_POINTER;  { immediate base is pointer }
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_INTEGER;
          ptr_ultimate_rec[j] := 0
        End;
        NextToken
      End
      Else If tok_type = TOK_CHAR_TYPE Then
      Begin
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_POINTER;
          If lo_bound = 1 Then
            sym_const_val[j] := TYPE_CHAR
          Else
            sym_const_val[j] := TYPE_POINTER;
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_CHAR;
          ptr_ultimate_rec[j] := 0
        End;
        NextToken
      End
      Else If tok_type = TOK_BOOLEAN_TYPE Then
      Begin
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_POINTER;
          If lo_bound = 1 Then
            sym_const_val[j] := TYPE_BOOLEAN
          Else
            sym_const_val[j] := TYPE_POINTER;
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_BOOLEAN;
          ptr_ultimate_rec[j] := 0
        End;
        NextToken
      End
      Else If tok_type = TOK_REAL_TYPE Then
      Begin
        For j := first_idx To idx Do
        Begin
          sym_type[j] := TYPE_POINTER;
          If lo_bound = 1 Then
            sym_const_val[j] := TYPE_REAL
          Else
            sym_const_val[j] := TYPE_POINTER;
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_REAL;
          ptr_ultimate_rec[j] := 0
        End;
        NextToken
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        { Pointer To Record Type: ^RecordType Or ^^RecordType etc }
        arr_size := SymLookup;  { reuse arr_size as type_idx }
        If (arr_size >= 0) And (sym_kind[arr_size] = SYM_TYPEDEF) And (sym_type[arr_size] = TYPE_RECORD) Then
        Begin
          For j := first_idx To idx Do
          Begin
            sym_type[j] := TYPE_POINTER;
            If lo_bound = 1 Then
              sym_const_val[j] := TYPE_RECORD
            Else
              sym_const_val[j] := TYPE_POINTER;
            sym_label[j] := arr_size;  { store Record Type index }
            ptr_depth[j] := lo_bound;
            ptr_ultimate_type[j] := TYPE_RECORD;
            ptr_ultimate_rec[j] := arr_size
          End;
          NextToken
        End
        Else
          Error(14)  { expected Type identifier }
      End
      Else
        Error(14)  { expected Type identifier }
    End
    Else If tok_type = TOK_IDENT Then
    Begin
      { May be a Record, enum, subrange, Or Set Type name }
      arr_size := SymLookup;  { reuse arr_size as type_idx temporarily }
      If (arr_size >= 0) And (sym_kind[arr_size] = SYM_TYPEDEF) Then
      Begin
        If sym_type[arr_size] = TYPE_RECORD Then
        Begin
          { Allocate space For Record }
          lo_bound := sym_label[arr_size];  { reuse lo_bound For Record size }
          { Adjust local_offset: we already allocated 8 bytes, need rest }
          local_offset := local_offset - (lo_bound - 8);
          For j := first_idx To idx Do
          Begin
            sym_type[j] := TYPE_RECORD;
            sym_const_val[j] := arr_size  { link To Type definition }
          End
        End
        Else If sym_type[arr_size] = TYPE_ENUM Then
        Begin
          { Enum variables use 8 bytes (same as Integer) }
          For j := first_idx To idx Do
          Begin
            sym_type[j] := TYPE_ENUM;
            sym_const_val[j] := arr_size  { link To Type definition }
          End
        End
        Else If sym_type[arr_size] = TYPE_SUBRANGE Then
        Begin
          { Subrange variables use 8 bytes (same as Integer) }
          For j := first_idx To idx Do
          Begin
            sym_type[j] := TYPE_SUBRANGE;
            sym_const_val[j] := arr_size  { link To Type definition }
          End
        End
        Else If sym_type[arr_size] = TYPE_SET Then
        Begin
          { Set variables use 8 bytes (64-bit bitmask) }
          For j := first_idx To idx Do
          Begin
            sym_type[j] := TYPE_SET;
            sym_const_val[j] := arr_size  { link To Type definition }
          End
        End
        Else
          Error(9);
        NextToken
      End
      Else
        Error(9)
    End
    Else
      Error(9);
    Expect(TOK_SEMICOLON)
  End
End;

Procedure ParseConstDeclarations;
Var
  idx: Integer;
Begin
  NextToken;  { consume 'Const' }
  While tok_type = TOK_IDENT Do
  Begin
    idx := SymAdd(SYM_CONST, TYPE_INTEGER, scope_level, 0);
    NextToken;
    Expect(TOK_EQ);
    If tok_type = TOK_INTEGER Then
    Begin
      sym_const_val[idx] := tok_int;
      NextToken
    End
    Else
      Error(10);
    Expect(TOK_SEMICOLON)
  End
End;

Procedure ParseTypeDeclarations;
Var
  type_idx, fld_start, fld_offset, fld_type: Integer;
  i, base_idx, nested_idx, nested_size, first_fld: Integer;
  enum_val, lo_val, hi_val, set_base_type: Integer;
Begin
  NextToken;  { consume 'Type' }
  While tok_type = TOK_IDENT Do
  Begin
    { Create Type symbol - will Set actual Type later }
    type_idx := SymAdd(SYM_TYPEDEF, TYPE_RECORD, scope_level, 0);
    NextToken;
    Expect(TOK_EQ);

    If tok_type = TOK_RECORD Then
    Begin
      { Record Type }
      NextToken;  { consume 'Record' }
      fld_start := field_count;
      fld_offset := 0;
      sym_const_val[type_idx] := fld_start;  { first field index }

      { Parse fields Until 'End' Or 'Case' (variant part) }
      While (tok_type <> TOK_END) And (tok_type <> TOK_CASE) Do
      Begin
        { Collect field names (may be comma-separated) }
        first_fld := field_count;
        Repeat
          If tok_type <> TOK_IDENT Then
            Error(11);

          { Save field name }
          base_idx := field_count * 32;
          For i := 0 To tok_len - 1 Do
            field_name[base_idx + i] := tok_str[i];
          field_name[base_idx + tok_len] := 0;
          field_count := field_count + 1;

          NextToken;
          If tok_type = TOK_COMMA Then
            NextToken
        Until tok_type = TOK_COLON;

        Expect(TOK_COLON);

        { Parse field Type }
        nested_size := 8;  { default For basic types }
        If tok_type = TOK_INTEGER_TYPE Then
        Begin
          fld_type := TYPE_INTEGER;
          nested_idx := 0;
          NextToken
        End
        Else If tok_type = TOK_CHAR_TYPE Then
        Begin
          fld_type := TYPE_CHAR;
          nested_idx := 0;
          NextToken
        End
        Else If tok_type = TOK_BOOLEAN_TYPE Then
        Begin
          fld_type := TYPE_BOOLEAN;
          nested_idx := 0;
          NextToken
        End
        Else If tok_type = TOK_REAL_TYPE Then
        Begin
          fld_type := TYPE_REAL;
          nested_idx := 0;
          NextToken
        End
        Else If tok_type = TOK_IDENT Then
        Begin
          { Nested Record Type Or enum Type }
          nested_idx := SymLookup;
          If (nested_idx >= 0) And (sym_kind[nested_idx] = SYM_TYPEDEF) Then
          Begin
            If sym_type[nested_idx] = TYPE_RECORD Then
            Begin
              fld_type := TYPE_RECORD;
              nested_size := sym_label[nested_idx]
            End
            Else If sym_type[nested_idx] = TYPE_ENUM Then
              fld_type := TYPE_ENUM
            Else If sym_type[nested_idx] = TYPE_SUBRANGE Then
              fld_type := TYPE_SUBRANGE
            Else If sym_type[nested_idx] = TYPE_SET Then
              fld_type := TYPE_SET
            Else
              Error(9);
            NextToken
          End
          Else
            Error(9)
        End
        Else
          Error(9);

        { Apply Type To all collected field names }
        For i := first_fld To field_count - 1 Do
        Begin
          field_type[i] := fld_type;
          field_offset[i] := fld_offset;
          field_rec_idx[i] := type_idx;
          field_rec_type[i] := nested_idx;
          If fld_type = TYPE_RECORD Then
            fld_offset := fld_offset + nested_size
          Else
            fld_offset := fld_offset + 8
        End;

        { Expect semicolon Or End }
        If tok_type = TOK_SEMICOLON Then
          NextToken
      End;

      { Check For variant part }
      If tok_type = TOK_CASE Then
      Begin
        NextToken;  { consume 'Case' }

        { Check For tag field: "Case tag: Type Of" vs "Case Type Of" }
        If tok_type = TOK_IDENT Then
        Begin
          { Could be "tag: Type" Or just "Type" - peek ahead }
          { Save identifier For potential tag field }
          base_idx := field_count * 32;
          For i := 0 To tok_len - 1 Do
            field_name[base_idx + i] := tok_str[i];
          field_name[base_idx + tok_len] := 0;

          NextToken;
          If tok_type = TOK_COLON Then
          Begin
            { It's a tag field }
            field_count := field_count + 1;
            NextToken;
            { Parse tag Type }
            If tok_type = TOK_INTEGER_TYPE Then
              fld_type := TYPE_INTEGER
            Else If tok_type = TOK_CHAR_TYPE Then
              fld_type := TYPE_CHAR
            Else If tok_type = TOK_BOOLEAN_TYPE Then
              fld_type := TYPE_BOOLEAN
            Else
              fld_type := TYPE_INTEGER;  { default }
            NextToken;

            { Record tag field }
            field_type[field_count - 1] := fld_type;
            field_offset[field_count - 1] := fld_offset;
            field_rec_idx[field_count - 1] := type_idx;
            field_rec_type[field_count - 1] := 0;
            fld_offset := fld_offset + 8
          End
          { Else: identifier was the Type name, already consumed }
        End
        Else
          NextToken;  { consume Type keyword }

        Expect(TOK_OF);

        { variant_start marks where all variants Begin }
        nested_idx := fld_offset;  { reuse as variant_start }
        nested_size := 0;          { reuse as max_variant_size }

        { Parse variant alternatives }
        While tok_type <> TOK_END Do
        Begin
          { Skip constant label(s) }
          While (tok_type <> TOK_COLON) And (tok_type <> TOK_END) Do
            NextToken;

          If tok_type = TOK_COLON Then
          Begin
          NextToken;  { consume ':' }
          Expect(TOK_LPAREN);

          { Parse fields In this variant - all start at variant_start }
          fld_offset := nested_idx;  { reset To variant start }

          While tok_type <> TOK_RPAREN Do
          Begin
            { Collect field names }
            first_fld := field_count;
            Repeat
              If tok_type <> TOK_IDENT Then
                Error(11);
              base_idx := field_count * 32;
              For i := 0 To tok_len - 1 Do
                field_name[base_idx + i] := tok_str[i];
              field_name[base_idx + tok_len] := 0;
              field_count := field_count + 1;
              NextToken;
              If tok_type = TOK_COMMA Then
                NextToken
            Until tok_type = TOK_COLON;

            Expect(TOK_COLON);

            { Parse field Type }
            lo_val := 8;  { reuse as field_size }
            If tok_type = TOK_INTEGER_TYPE Then
              fld_type := TYPE_INTEGER
            Else If tok_type = TOK_CHAR_TYPE Then
              fld_type := TYPE_CHAR
            Else If tok_type = TOK_BOOLEAN_TYPE Then
              fld_type := TYPE_BOOLEAN
            Else If tok_type = TOK_REAL_TYPE Then
              fld_type := TYPE_REAL
            Else If tok_type = TOK_IDENT Then
            Begin
              hi_val := SymLookup;  { reuse as lookup result }
              If (hi_val >= 0) And (sym_kind[hi_val] = SYM_TYPEDEF) Then
              Begin
                If sym_type[hi_val] = TYPE_RECORD Then
                Begin
                  fld_type := TYPE_RECORD;
                  lo_val := sym_label[hi_val]
                End
                Else
                  fld_type := sym_type[hi_val]
              End
              Else
                fld_type := TYPE_INTEGER
            End
            Else
              fld_type := TYPE_INTEGER;
            NextToken;

            { Record fields }
            For i := first_fld To field_count - 1 Do
            Begin
              field_type[i] := fld_type;
              field_offset[i] := fld_offset;
              field_rec_idx[i] := type_idx;
              If fld_type = TYPE_RECORD Then
                field_rec_type[i] := hi_val
              Else
                field_rec_type[i] := 0;
              fld_offset := fld_offset + lo_val
            End;

            If tok_type = TOK_SEMICOLON Then
              NextToken
          End;

          NextToken;  { consume ')' }

          { Track max variant size }
          If (fld_offset - nested_idx) > nested_size Then
            nested_size := fld_offset - nested_idx;

          If tok_type = TOK_SEMICOLON Then
            NextToken
          End  { End If tok_type = TOK_COLON }
        End;

        fld_offset := nested_idx + nested_size  { final size }
      End;

      NextToken;  { consume 'End' }
      sym_label[type_idx] := fld_offset  { total Record size }
    End
    Else If tok_type = TOK_LPAREN Then
    Begin
      { Enumerated Type: (Red, Green, Blue) }
      sym_type[type_idx] := TYPE_ENUM;
      sym_const_val[type_idx] := enum_count;  { index into enum arrays }
      enum_low[enum_count] := 0;
      enum_val := 0;
      NextToken;  { consume '(' }
      While tok_type <> TOK_RPAREN Do
      Begin
        If tok_type <> TOK_IDENT Then
          Error(11);
        { Add enum value as a constant }
        base_idx := SymAdd(SYM_CONST, TYPE_ENUM, scope_level, 0);
        sym_const_val[base_idx] := enum_val;
        sym_label[base_idx] := type_idx;  { link To enum Type }
        enum_val := enum_val + 1;
        NextToken;
        If tok_type = TOK_COMMA Then
          NextToken
      End;
      enum_high[enum_count] := enum_val - 1;
      sym_label[type_idx] := enum_val;  { number Of values }
      enum_count := enum_count + 1;
      NextToken  { consume ')' }
    End
    Else If tok_type = TOK_SET Then
    Begin
      { Set Type: Set Of Char / Set Of EnumType / Set Of 0..63 }
      sym_type[type_idx] := TYPE_SET;
      NextToken;  { consume 'Set' }
      Expect(TOK_OF);
      sym_const_val[type_idx] := set_count;  { index into Set arrays }
      If tok_type = TOK_CHAR_TYPE Then
      Begin
        set_base[set_count] := TYPE_CHAR;
        set_low[set_count] := 0;
        set_high[set_count] := 255;
        NextToken
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        base_idx := SymLookup;
        If (base_idx >= 0) And (sym_kind[base_idx] = SYM_TYPEDEF) Then
        Begin
          If sym_type[base_idx] = TYPE_ENUM Then
          Begin
            set_base[set_count] := TYPE_ENUM;
            nested_idx := sym_const_val[base_idx];  { enum index }
            set_low[set_count] := enum_low[nested_idx];
            set_high[set_count] := enum_high[nested_idx]
          End
          Else If sym_type[base_idx] = TYPE_SUBRANGE Then
          Begin
            set_base[set_count] := TYPE_SUBRANGE;
            nested_idx := sym_const_val[base_idx];  { subrange index }
            set_low[set_count] := subr_low[nested_idx];
            set_high[set_count] := subr_high[nested_idx]
          End
          Else
            Error(9);
          NextToken
        End
        Else
          Error(9)
      End
      Else If tok_type = TOK_INTEGER Then
      Begin
        { Set Of 0..63 - inline subrange }
        lo_val := tok_int;
        NextToken;
        Expect(TOK_DOTDOT);
        If tok_type <> TOK_INTEGER Then
          Error(9);
        hi_val := tok_int;
        If hi_val - lo_val > 63 Then
          Error(16);  { Set too large }
        set_base[set_count] := TYPE_SUBRANGE;
        set_low[set_count] := lo_val;
        set_high[set_count] := hi_val;
        NextToken
      End
      Else
        Error(9);
      set_count := set_count + 1
    End
    Else If (tok_type = TOK_INTEGER) Or (tok_type = TOK_MINUS) Then
    Begin
      { Subrange Type: 0..9 Or -10..10 }
      If tok_type = TOK_MINUS Then
      Begin
        NextToken;
        If tok_type <> TOK_INTEGER Then
          Error(9);
        lo_val := 0 - tok_int
      End
      Else
        lo_val := tok_int;
      NextToken;
      Expect(TOK_DOTDOT);
      If tok_type = TOK_MINUS Then
      Begin
        NextToken;
        If tok_type <> TOK_INTEGER Then
          Error(9);
        hi_val := 0 - tok_int
      End
      Else If tok_type = TOK_INTEGER Then
        hi_val := tok_int
      Else
        Error(9);
      sym_type[type_idx] := TYPE_SUBRANGE;
      sym_const_val[type_idx] := subr_count;  { index into subrange arrays }
      subr_low[subr_count] := lo_val;
      subr_high[subr_count] := hi_val;
      subr_base[subr_count] := TYPE_INTEGER;
      sym_label[type_idx] := hi_val - lo_val + 1;  { number Of values }
      subr_count := subr_count + 1;
      NextToken
    End
    Else If tok_type = TOK_IDENT Then
    Begin
      { Could be alias To another Type, Or subrange Of enum }
      base_idx := SymLookup;
      If base_idx >= 0 Then
      Begin
        If sym_kind[base_idx] = SYM_CONST Then
        Begin
          { Subrange starting With enum constant }
          lo_val := sym_const_val[base_idx];
          nested_idx := sym_label[base_idx];  { enum Type }
          NextToken;
          Expect(TOK_DOTDOT);
          If tok_type <> TOK_IDENT Then
            Error(9);
          base_idx := SymLookup;
          If (base_idx < 0) Or (sym_kind[base_idx] <> SYM_CONST) Then
            Error(9);
          hi_val := sym_const_val[base_idx];
          sym_type[type_idx] := TYPE_SUBRANGE;
          sym_const_val[type_idx] := subr_count;
          subr_low[subr_count] := lo_val;
          subr_high[subr_count] := hi_val;
          subr_base[subr_count] := TYPE_ENUM;
          sym_label[type_idx] := hi_val - lo_val + 1;
          subr_count := subr_count + 1;
          NextToken
        End
        Else If sym_kind[base_idx] = SYM_TYPEDEF Then
        Begin
          { Type alias - copy the Type info }
          sym_type[type_idx] := sym_type[base_idx];
          sym_const_val[type_idx] := sym_const_val[base_idx];
          sym_label[type_idx] := sym_label[base_idx];
          NextToken
        End
        Else
          Error(9)
      End
      Else
        Error(9)
    End
    Else
      Error(9);
    Expect(TOK_SEMICOLON)
  End
End;

Procedure ParseProcedureDeclaration; Forward;
Procedure ParseFunctionDeclaration; Forward;

Procedure ParseBlock;
Var
  saved_offset: Integer;
  alloc_size: Integer;
  body_label: Integer;
Begin
  saved_offset := local_offset;
  body_label := 0;

  While (tok_type = TOK_CONST) Or (tok_type = TOK_VAR) Or (tok_type = TOK_TYPE_KW) Do
  Begin
    If tok_type = TOK_CONST Then
      ParseConstDeclarations
    Else If tok_type = TOK_TYPE_KW Then
      ParseTypeDeclarations
    Else
      ParseVarDeclarations
  End;

  { If there are Procedure/Function declarations, jump over them }
  If (tok_type = TOK_PROCEDURE) Or (tok_type = TOK_FUNCTION) Then
  Begin
    body_label := NewLabel;
    EmitBranchLabel(body_label)
  End;

  While (tok_type = TOK_PROCEDURE) Or (tok_type = TOK_FUNCTION) Do
  Begin
    If tok_type = TOK_PROCEDURE Then
      ParseProcedureDeclaration
    Else
      ParseFunctionDeclaration
  End;

  If body_label > 0 Then
    EmitLabel(body_label);

  { Allocate stack space - round up To 16 For alignment }
  alloc_size := 0;
  If local_offset < saved_offset Then
  Begin
    alloc_size := saved_offset - local_offset;
    alloc_size := ((alloc_size + 15) Div 16) * 16;
    EmitSubSP(alloc_size)
  End;

  Expect(TOK_BEGIN);
  ParseStatement;
  While tok_type = TOK_SEMICOLON Do
  Begin
    NextToken;
    ParseStatement
  End;
  Expect(TOK_END);

  { Deallocate stack space }
  If alloc_size > 0 Then
  Begin
    EmitAddSP(alloc_size)
  End
End;

Procedure ParseProcedureDeclaration;
Var
  idx, proc_label: Integer;
  saved_level, saved_offset: Integer;
  param_count, param_idx, i, j: Integer;
  param_indices: Array[0..7] Of Integer;
  is_var_group: Integer;
  saved_exit_label, proc_exit_label: Integer;
Begin
  NextToken;  { consume 'Procedure' }

  If tok_type <> TOK_IDENT Then
    Error(11);

  { Check If Procedure already exists (Forward declaration) }
  idx := SymLookup;
  If (idx >= 0) And (sym_kind[idx] = SYM_PROCEDURE) Then
  Begin
    { Reuse existing Forward-declared Procedure }
    proc_label := sym_label[idx]
  End
  Else
  Begin
    { Create New Procedure symbol }
    idx := SymAdd(SYM_PROCEDURE, TYPE_VOID, scope_level, 0);
    proc_label := NewLabel;
    sym_label[idx] := proc_label
  End;
  NextToken;

  { Save state And enter New scope For parameters }
  saved_level := scope_level;
  saved_offset := local_offset;
  scope_level := scope_level + 1;
  local_offset := -8;  { Reserve -8 For static link }
  param_count := 0;

  { Handle optional parameters }
  If tok_type = TOK_LPAREN Then
  Begin
    NextToken;
    If tok_type <> TOK_RPAREN Then
    Begin
      Repeat
        If tok_type = TOK_SEMICOLON Then NextToken;
        { Check For 'Var' keyword - applies To all idents In this group }
        is_var_group := 0;
        If tok_type = TOK_VAR Then
        Begin
          is_var_group := 1;
          NextToken
        End;
        { Parse identifier list For this parameter group }
        If tok_type <> TOK_IDENT Then Error(11);
        Repeat
          If tok_type = TOK_COMMA Then NextToken;
          If tok_type <> TOK_IDENT Then Error(11);
          { Add parameter }
          local_offset := local_offset - 8;
          param_idx := SymAdd(SYM_PARAM, TYPE_INTEGER, scope_level, local_offset);
          If is_var_group = 1 Then
            sym_is_var_param[param_idx] := 1;
          If param_count < 8 Then
            param_indices[param_count] := param_idx;
          param_count := param_count + 1;
          NextToken
        Until tok_type <> TOK_COMMA;
        { Parse Type annotation And Set param types }
        If tok_type = TOK_COLON Then
        Begin
          NextToken;
          If tok_type = TOK_REAL_TYPE Then
          Begin
            { Set all params In this group To TYPE_REAL }
            { Find starting index Of this group And Set types }
            { param_indices has the indices, but we need To know where this group started }
            { For simplicity, Set all recent params To Real - this is approximate }
            sym_type[param_idx] := TYPE_REAL;
            NextToken
          End
          Else If tok_type = TOK_STRING_TYPE Then
          Begin
            { String parameter }
            sym_type[param_idx] := TYPE_STRING;
            If is_var_group = 0 Then
            Begin
              { Value parameter - need 256 bytes total, already have 8, need 248 more }
              local_offset := local_offset - 248;
              sym_offset[param_idx] := local_offset;  { Update offset To start Of 256-byte area }
              sym_label[param_idx] := 256  { Mark as needing copy-In }
            End;
            { Var String params keep the 8-byte slot For address }
            NextToken
          End
          Else If (tok_type = TOK_INTEGER_TYPE) Or (tok_type = TOK_CHAR_TYPE) Or
             (tok_type = TOK_BOOLEAN_TYPE) Then
            NextToken
        End
      Until tok_type <> TOK_SEMICOLON
    End;
    Expect(TOK_RPAREN)
  End;

  { Build Var-param bitmap And store on Procedure symbol }
  sym_var_param_flags[idx] := 0;
  For i := 0 To param_count - 1 Do
    If sym_is_var_param[param_indices[i]] = 1 Then
    Begin
      If i = 0 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 1;
      If i = 1 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 2;
      If i = 2 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 4;
      If i = 3 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 8;
      If i = 4 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 16;
      If i = 5 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 32;
      If i = 6 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 64;
      If i = 7 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 128
    End;

  Expect(TOK_SEMICOLON);

  { Check For Forward declaration Or Interface declaration }
  If (tok_type = TOK_FORWARD) Or (in_interface = 1) Then
  Begin
    If tok_type = TOK_FORWARD Then
    Begin
      NextToken;
      Expect(TOK_SEMICOLON)
    End;
    PopScope(scope_level);
    scope_level := saved_level;
    local_offset := saved_offset
  End
  Else
  Begin

  { Emit Procedure label And prolog - save x29, x30, static link }
  { For units, use named labels To avoid conflicts when linking }
  If compiling_unit = 1 Then
  Begin
    EmitGloblCurrentUnit(idx);  { Export the symbol }
    EmitUnitLabel(idx)
  End
  Else
    EmitLabel(proc_label);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);  { Allocate space For static link }
  EmitStoreStaticLink;

  { Allocate space For parameters And copy from registers }
  If param_count > 0 Then
  Begin
    { Allocate based on actual space needed (local_offset tracks it) }
    { param_space = -(local_offset) - 8 (static link already counted separately) }
    j := 0 - local_offset - 8;
    j := ((j + 15) Div 16) * 16;  { Align To 16 }
    If j > 0 Then
      EmitSubSP(j);
    For i := 0 To param_count - 1 Do
    Begin
      If i < 8 Then
      Begin
        { Check If this is a String value parameter (needs copy) }
        If (sym_type[param_indices[i]] = TYPE_STRING) And (sym_is_var_param[param_indices[i]] = 0) Then
        Begin
          { String value param: xi has address Of source, copy To local storage }
          { x9 = source (passed address), compute x8 = dest using large offset handling }
          EmitIndent;
          WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
          WriteChar(120); WriteChar(57); WriteChar(44); WriteChar(32);  { x9, }
          WriteChar(120); WriteChar(48 + i);  { xi }
          EmitNL;
          { Compute dest address: x8 = x29 - offset (handle large negative offset) }
          EmitIndent;
          WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
          WriteChar(120); WriteChar(56); WriteChar(44); WriteChar(32);  { x8, }
          WriteChar(35);
          Write(0 - sym_offset[param_indices[i]]);  { positive value }
          EmitNL;
          EmitIndent;
          WriteChar(115); WriteChar(117); WriteChar(98); WriteChar(32);  { sub }
          WriteChar(120); WriteChar(56); WriteChar(44); WriteChar(32);  { x8, }
          WriteChar(120); WriteChar(50); WriteChar(57); WriteChar(44); WriteChar(32);  { x29, }
          WriteChar(120); WriteChar(56);  { x8 }
          EmitNL;
          EmitBL(rt_str_copy)
        End
        Else
        Begin
          { Normal param: store register To stack }
          EmitIndent;
          WriteChar(115); WriteChar(116); WriteChar(117); WriteChar(114); WriteChar(32);  { stur }
          WriteChar(120); WriteChar(48 + i); WriteChar(44); WriteChar(32);  { xi, }
          WriteChar(91); WriteChar(120); WriteChar(50); WriteChar(57);  { [x29 }
          WriteChar(44); WriteChar(32); WriteChar(35);  { , # }
          Write(sym_offset[param_indices[i]]);
          WriteChar(93);  { ] }
          EmitNL
        End
      End
    End
  End;

  { Set up Exit label For this Procedure }
  saved_exit_label := exit_label;
  proc_exit_label := NewLabel;
  exit_label := proc_exit_label;

  { Parse Procedure body }
  ParseBlock;

  { Pop local symbols And restore scope }
  PopScope(scope_level);
  scope_level := saved_level;
  local_offset := saved_offset;

  { Emit Exit label For Exit statements }
  EmitLabel(proc_exit_label);
  exit_label := saved_exit_label;

  { Restore sp To frame pointer (undoes static link + params + local allocations) }
  EmitIndent;
  WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
  WriteChar(115); WriteChar(112); WriteChar(44); WriteChar(32);  { sp, }
  WriteChar(120); WriteChar(50); WriteChar(57);  { x29 }
  EmitNL;

  { Restore frame And return }
  EmitLdp;
  EmitIndent;
  WriteChar(114); WriteChar(101); WriteChar(116);  { ret }
  EmitNL;

  Expect(TOK_SEMICOLON)
  End  { End Of Else For non-Forward declaration }
End;

Procedure ParseFunctionDeclaration;
Var
  idx, func_label: Integer;
  saved_level, saved_offset: Integer;
  param_count, param_idx, i, j: Integer;
  param_indices: Array[0..7] Of Integer;
  is_var_group: Integer;
  saved_exit_label, func_exit_label: Integer;
Begin
  NextToken;  { consume 'Function' }

  If tok_type <> TOK_IDENT Then
    Error(11);

  { Check If Function already exists (Forward declaration) }
  idx := SymLookup;
  If (idx >= 0) And (sym_kind[idx] = SYM_FUNCTION) Then
  Begin
    { Reuse existing Forward-declared Function }
    func_label := sym_label[idx]
  End
  Else
  Begin
    { Create New Function symbol }
    idx := SymAdd(SYM_FUNCTION, TYPE_INTEGER, scope_level, 0);
    func_label := NewLabel;
    sym_label[idx] := func_label
  End;
  NextToken;

  { Save state And enter New scope For parameters }
  saved_level := scope_level;
  saved_offset := local_offset;
  scope_level := scope_level + 1;
  local_offset := -16;  { Reserve -8 For static link, -16 For result }
  param_count := 0;

  { Handle optional parameters }
  If tok_type = TOK_LPAREN Then
  Begin
    NextToken;
    If tok_type <> TOK_RPAREN Then
    Begin
      Repeat
        If tok_type = TOK_SEMICOLON Then NextToken;
        { Check For 'Var' keyword - applies To all idents In this group }
        is_var_group := 0;
        If tok_type = TOK_VAR Then
        Begin
          is_var_group := 1;
          NextToken
        End;
        { Parse identifier list For this parameter group }
        If tok_type <> TOK_IDENT Then Error(11);
        Repeat
          If tok_type = TOK_COMMA Then NextToken;
          If tok_type <> TOK_IDENT Then Error(11);
          { Add parameter }
          local_offset := local_offset - 8;
          param_idx := SymAdd(SYM_PARAM, TYPE_INTEGER, scope_level, local_offset);
          If is_var_group = 1 Then
            sym_is_var_param[param_idx] := 1;
          If param_count < 8 Then
            param_indices[param_count] := param_idx;
          param_count := param_count + 1;
          NextToken
        Until tok_type <> TOK_COMMA;
        { Parse Type annotation And Set param types }
        If tok_type = TOK_COLON Then
        Begin
          NextToken;
          If tok_type = TOK_REAL_TYPE Then
          Begin
            sym_type[param_idx] := TYPE_REAL;
            NextToken
          End
          Else If tok_type = TOK_STRING_TYPE Then
          Begin
            { String parameter }
            sym_type[param_idx] := TYPE_STRING;
            If is_var_group = 0 Then
            Begin
              { Value parameter - need 256 bytes total, already have 8, need 248 more }
              local_offset := local_offset - 248;
              sym_offset[param_idx] := local_offset;  { Update offset To start Of 256-byte area }
              sym_label[param_idx] := 256  { Mark as needing copy-In }
            End;
            { Var String params keep the 8-byte slot For address }
            NextToken
          End
          Else If (tok_type = TOK_INTEGER_TYPE) Or (tok_type = TOK_CHAR_TYPE) Or
             (tok_type = TOK_BOOLEAN_TYPE) Then
            NextToken
        End
      Until tok_type <> TOK_SEMICOLON
    End;
    Expect(TOK_RPAREN)
  End;

  { Build Var-param bitmap And store on Function symbol }
  sym_var_param_flags[idx] := 0;
  For i := 0 To param_count - 1 Do
    If sym_is_var_param[param_indices[i]] = 1 Then
    Begin
      If i = 0 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 1;
      If i = 1 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 2;
      If i = 2 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 4;
      If i = 3 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 8;
      If i = 4 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 16;
      If i = 5 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 32;
      If i = 6 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 64;
      If i = 7 Then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 128
    End;

  { Parse return Type }
  Expect(TOK_COLON);
  If (tok_type = TOK_INTEGER_TYPE) Or (tok_type = TOK_CHAR_TYPE) Or
     (tok_type = TOK_BOOLEAN_TYPE) Then
    NextToken
  Else If tok_type = TOK_REAL_TYPE Then
  Begin
    sym_type[idx] := TYPE_REAL;
    NextToken
  End
  Else If tok_type = TOK_STRING_TYPE Then
  Begin
    sym_type[idx] := TYPE_STRING;
    NextToken
  End
  Else
    Error(9);

  Expect(TOK_SEMICOLON);

  { Check For Forward declaration Or Interface declaration }
  If (tok_type = TOK_FORWARD) Or (in_interface = 1) Then
  Begin
    If tok_type = TOK_FORWARD Then
    Begin
      NextToken;
      Expect(TOK_SEMICOLON)
    End;
    PopScope(scope_level);
    scope_level := saved_level;
    local_offset := saved_offset
  End
  Else
  Begin

  { Emit Function label And prolog - save x29, x30, static link }
  { For units, use named labels To avoid conflicts when linking }
  If compiling_unit = 1 Then
  Begin
    EmitGloblCurrentUnit(idx);  { Export the symbol }
    EmitUnitLabel(idx)
  End
  Else
    EmitLabel(func_label);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);  { Allocate space For static link }
  EmitStoreStaticLink;

  { Allocate space For parameters And copy from registers }
  If param_count > 0 Then
  Begin
    { Allocate based on actual space needed (local_offset tracks it) }
    { For functions, local_offset includes return value at -16, params start at -24 }
    j := 0 - local_offset - 16;  { -16 For return value slot }
    j := ((j + 15) Div 16) * 16;  { Align To 16 }
    If j > 0 Then
      EmitSubSP(j);
    For i := 0 To param_count - 1 Do
    Begin
      If i < 8 Then
      Begin
        { Check If this is a String value parameter (needs copy) }
        If (sym_type[param_indices[i]] = TYPE_STRING) And (sym_is_var_param[param_indices[i]] = 0) Then
        Begin
          { String value param: xi has address Of source, copy To local storage }
          { x9 = source (passed address), compute x8 = dest using large offset handling }
          EmitIndent;
          WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
          WriteChar(120); WriteChar(57); WriteChar(44); WriteChar(32);  { x9, }
          WriteChar(120); WriteChar(48 + i);  { xi }
          EmitNL;
          { Compute dest address: x8 = x29 - offset (handle large negative offset) }
          EmitIndent;
          WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
          WriteChar(120); WriteChar(56); WriteChar(44); WriteChar(32);  { x8, }
          WriteChar(35);
          Write(0 - sym_offset[param_indices[i]]);  { positive value }
          EmitNL;
          EmitIndent;
          WriteChar(115); WriteChar(117); WriteChar(98); WriteChar(32);  { sub }
          WriteChar(120); WriteChar(56); WriteChar(44); WriteChar(32);  { x8, }
          WriteChar(120); WriteChar(50); WriteChar(57); WriteChar(44); WriteChar(32);  { x29, }
          WriteChar(120); WriteChar(56);  { x8 }
          EmitNL;
          EmitBL(rt_str_copy)
        End
        Else
        Begin
          { Normal param: store register To stack }
          EmitIndent;
          WriteChar(115); WriteChar(116); WriteChar(117); WriteChar(114); WriteChar(32);  { stur }
          WriteChar(120); WriteChar(48 + i); WriteChar(44); WriteChar(32);  { xi, }
          WriteChar(91); WriteChar(120); WriteChar(50); WriteChar(57);  { [x29 }
          WriteChar(44); WriteChar(32); WriteChar(35);  { , # }
          Write(sym_offset[param_indices[i]]);
          WriteChar(93);  { ] }
          EmitNL
        End
      End
    End
  End;

  { Set up Exit label For this Function }
  saved_exit_label := exit_label;
  func_exit_label := NewLabel;
  exit_label := func_exit_label;

  { Parse Function body }
  ParseBlock;

  { Pop local symbols And restore scope }
  PopScope(scope_level);
  scope_level := saved_level;
  local_offset := saved_offset;

  { Emit Exit label For Exit statements }
  EmitLabel(func_exit_label);
  exit_label := saved_exit_label;

  { Load result from local variable into x0 Or d0 }
  If sym_type[idx] = TYPE_REAL Then
    EmitLdurD0(-16)
  Else
    EmitLdurX0(-16);

  { Restore sp To frame pointer (undoes static link + params + local allocations) }
  EmitIndent;
  WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
  WriteChar(115); WriteChar(112); WriteChar(44); WriteChar(32);  { sp, }
  WriteChar(120); WriteChar(50); WriteChar(57);  { x29 }
  EmitNL;

  { Restore frame And return }
  EmitLdp;
  EmitIndent;
  WriteChar(114); WriteChar(101); WriteChar(116);  { ret }
  EmitNL;

  Expect(TOK_SEMICOLON)
  End  { End Of Else For non-Forward declaration }
End;

{ Write Unit name To TPU file (from current_unit_name) }
Procedure TPUWriteUnitName;
Var
  i: Integer;
Begin
  For i := 0 To current_unit_len - 1 Do
    writefilechar(tpu_file, current_unit_name[i])
End;

{ Write an Integer To TPU file }
Procedure TPUWriteInt(n: Integer);
Var
  neg: Integer;
  digits: Array[0..19] Of Integer;
  count, i: Integer;
Begin
  neg := 0;
  If n < 0 Then
  Begin
    neg := 1;
    n := 0 - n
  End;
  count := 0;
  If n = 0 Then
  Begin
    digits[0] := 48;
    count := 1
  End
  Else
  Begin
    While n > 0 Do
    Begin
      digits[count] := 48 + (n Mod 10);
      n := n Div 10;
      count := count + 1
    End
  End;
  If neg = 1 Then
    writefilechar(tpu_file, 45);  { '-' }
  For i := count - 1 DownTo 0 Do
    writefilechar(tpu_file, digits[i])
End;

{ Write symbol name To TPU file }
Procedure TPUWriteSymName(idx: Integer);
Var
  base, i, c: Integer;
Begin
  base := idx * 32;
  i := 0;
  c := sym_name[base];
  While c <> 0 Do
  Begin
    writefilechar(tpu_file, c);
    i := i + 1;
    c := sym_name[base + i]
  End
End;

{ Write Type name To TPU file }
Procedure TPUWriteType(t: Integer);
Begin
  If t = TYPE_INTEGER Then
    Write(tpu_file, 'Integer')
  Else If t = TYPE_CHAR Then
    Write(tpu_file, 'Char')
  Else If t = TYPE_BOOLEAN Then
    Write(tpu_file, 'Boolean')
  Else If t = TYPE_STRING Then
    Write(tpu_file, 'String')
  Else If t = TYPE_REAL Then
    Write(tpu_file, 'Real')
  Else If t = TYPE_VOID Then
    Write(tpu_file, 'VOID')
  Else If t = TYPE_POINTER Then
    Write(tpu_file, 'POINTER')
  Else If t = TYPE_RECORD Then
    Write(tpu_file, 'Record')
  Else If t = TYPE_ARRAY Then
    Write(tpu_file, 'Array')
  Else
    Write(tpu_file, 'UNKNOWN')
End;

{ Write TPU file For current Unit }
Procedure WriteTPUFile;
Var
  i, base, c, j: Integer;
Begin
  { Build filename In tok_str: unitname.tpu (lowercase) }
  For i := 0 To current_unit_len - 1 Do
    tok_str[i] := ToLower(current_unit_name[i]);
  tok_str[current_unit_len] := 46;      { . }
  tok_str[current_unit_len + 1] := 116; { t }
  tok_str[current_unit_len + 2] := 112; { p }
  tok_str[current_unit_len + 3] := 117; { u }
  tok_str[current_unit_len + 4] := 0;

  { Open file For writing using assigntokstr }
  assigntokstr(tpu_file, 0, current_unit_len + 4);
  rewrite(tpu_file);

  { Write header }
  WriteLn(tpu_file, 'TUXPASCAL_UNIT_V1');

  { Write Unit name }
  Write(tpu_file, 'Unit ');
  TPUWriteUnitName;
  WriteLn(tpu_file);

  { Write Interface section }
  WriteLn(tpu_file, 'Interface');

  { Write Interface symbols }
  For i := interface_start To interface_end Do
  Begin
    If sym_kind[i] = SYM_CONST Then
    Begin
      Write(tpu_file, 'Const ');
      TPUWriteSymName(i);
      Write(tpu_file, ' ');
      TPUWriteType(sym_type[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_const_val[i]);
      WriteLn(tpu_file)
    End
    Else If sym_kind[i] = SYM_VAR Then
    Begin
      Write(tpu_file, 'Var ');
      TPUWriteSymName(i);
      Write(tpu_file, ' ');
      TPUWriteType(sym_type[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_offset[i]);
      WriteLn(tpu_file)
    End
    Else If sym_kind[i] = SYM_PROCEDURE Then
    Begin
      Write(tpu_file, 'PROC ');
      TPUWriteSymName(i);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_label[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_var_param_flags[i]);
      WriteLn(tpu_file)
    End
    Else If sym_kind[i] = SYM_FUNCTION Then
    Begin
      Write(tpu_file, 'FUNC ');
      TPUWriteSymName(i);
      Write(tpu_file, ' ');
      TPUWriteType(sym_type[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_label[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_var_param_flags[i]);
      WriteLn(tpu_file)
    End
    Else If sym_kind[i] = SYM_TYPEDEF Then
    Begin
      Write(tpu_file, 'Type ');
      TPUWriteSymName(i);
      Write(tpu_file, ' ');
      TPUWriteType(sym_type[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_const_val[i]);
      Write(tpu_file, ' ');
      TPUWriteInt(sym_label[i]);
      WriteLn(tpu_file)
    End
  End;

  WriteLn(tpu_file, 'ENDINTERFACE');

  close(tpu_file)
End;

{ Read a line from TPU file into tpu_line buffer }
Function TPUReadLine: Integer;
Var
  c: Integer;
  done: Integer;
Begin
  tpu_line_len := 0;
  TPUReadLine := 0;
  done := 0;
  If Not eof(tpu_file) Then
  Begin
    While (Not eof(tpu_file)) And (done = 0) Do
    Begin
      Read(tpu_file, c);
      If c = 10 Then
        done := 1
      Else If c <> 13 Then
      Begin
        tpu_line[tpu_line_len] := c;
        tpu_line_len := tpu_line_len + 1
      End
    End;
    tpu_line[tpu_line_len] := 0;
    If tpu_line_len > 0 Then
      TPUReadLine := 1
    Else If done = 1 Then
      TPUReadLine := 1  { empty line is still a line }
  End
End;

{ Note: TPULineStartsWith removed - checks are inlined using first 3 chars }

{ Parse Integer from tpu_line at tpu_pos (updates tpu_pos) }
Function TPUParseInt: Integer;
Var
  neg, result: Integer;
Begin
  neg := 0;
  result := 0;
  If tpu_line[tpu_pos] = 45 Then  { '-' }
  Begin
    neg := 1;
    tpu_pos := tpu_pos + 1
  End;
  While (tpu_line[tpu_pos] >= 48) And (tpu_line[tpu_pos] <= 57) Do
  Begin
    result := result * 10 + (tpu_line[tpu_pos] - 48);
    tpu_pos := tpu_pos + 1
  End;
  If neg = 1 Then
    TPUParseInt := 0 - result
  Else
    TPUParseInt := result
End;

{ Skip spaces In tpu_line at tpu_pos (updates tpu_pos) }
Procedure TPUSkipSpaces;
Begin
  While tpu_line[tpu_pos] = 32 Do
    tpu_pos := tpu_pos + 1
End;

{ Parse identifier from tpu_line at tpu_pos, store In tok_str (updates tpu_pos) }
Procedure TPUParseIdent;
Begin
  tok_len := 0;
  TPUSkipSpaces;
  While (tpu_line[tpu_pos] <> 32) And (tpu_line[tpu_pos] <> 0) Do
  Begin
    tok_str[tok_len] := tpu_line[tpu_pos];
    tok_len := tok_len + 1;
    tpu_pos := tpu_pos + 1
  End;
  tok_str[tok_len] := 0
End;

{ Parse Type from tpu_line at tpu_pos, return TYPE_* constant }
Function TPUParseType: Integer;
Begin
  TPUParseIdent;
  If (tok_str[0] = 73) And (tok_str[1] = 78) And (tok_str[2] = 84) Then  { INT }
    TPUParseType := TYPE_INTEGER
  Else If (tok_str[0] = 67) And (tok_str[1] = 72) And (tok_str[2] = 65) Then  { CHA }
    TPUParseType := TYPE_CHAR
  Else If (tok_str[0] = 66) And (tok_str[1] = 79) And (tok_str[2] = 79) Then  { BOO }
    TPUParseType := TYPE_BOOLEAN
  Else If (tok_str[0] = 83) And (tok_str[1] = 84) And (tok_str[2] = 82) Then  { STR }
    TPUParseType := TYPE_STRING
  Else If (tok_str[0] = 82) And (tok_str[1] = 69) And (tok_str[2] = 65) Then  { REA }
    TPUParseType := TYPE_REAL
  Else If (tok_str[0] = 86) And (tok_str[1] = 79) And (tok_str[2] = 73) Then  { VOI }
    TPUParseType := TYPE_VOID
  Else If (tok_str[0] = 80) And (tok_str[1] = 79) And (tok_str[2] = 73) Then  { POI }
    TPUParseType := TYPE_POINTER
  Else If (tok_str[0] = 82) And (tok_str[1] = 69) And (tok_str[2] = 67) Then  { REC }
    TPUParseType := TYPE_RECORD
  Else If (tok_str[0] = 65) And (tok_str[1] = 82) And (tok_str[2] = 82) Then  { ARR }
    TPUParseType := TYPE_ARRAY
  Else
    TPUParseType := TYPE_INTEGER
End;

{ Load Interface from a TPU file }
Function LoadTPUInterface: Integer;
Var
  idx: Integer;
  unit_base, i: Integer;
Begin
  LoadTPUInterface := 0;

  { Read And verify header }
  If TPUReadLine = 0 Then
  Begin
    LoadTPUInterface := -1
  End
  Else
  Begin
    { Check For TUXPASCAL_UNIT_V1 }
    If (tpu_line[0] <> 84) Or (tpu_line[1] <> 85) Or (tpu_line[2] <> 88) Then
    Begin
      LoadTPUInterface := -1
    End
    Else
    Begin
      { Read Unit line }
      If TPUReadLine = 0 Then
        LoadTPUInterface := -1
      Else
      Begin
        { Store Unit name For symbol prefixing }
        unit_base := loaded_count * 32;
        tpu_pos := 5;  { Skip 'Unit ' }
        i := 0;
        While (tpu_line[tpu_pos] <> 0) And (i < 31) Do
        Begin
          loaded_units[unit_base + i] := tpu_line[tpu_pos];
          tpu_pos := tpu_pos + 1;
          i := i + 1
        End;
        loaded_units[unit_base + i] := 0;

        unit_sym_start[loaded_count] := sym_count;

        { Read Interface line }
        If TPUReadLine = 0 Then
          LoadTPUInterface := -1
        Else
        Begin
          { Parse Interface declarations Until ENDINTERFACE }
          While TPUReadLine = 1 Do
          Begin
            If (tpu_line[0] = 69) And (tpu_line[1] = 78) And (tpu_line[2] = 68) Then
            Begin
              { ENDINTERFACE }
              unit_sym_end[loaded_count] := sym_count - 1;
              loaded_count := loaded_count + 1;
              LoadTPUInterface := 1
            End
            Else If (tpu_line[0] = 67) And (tpu_line[1] = 79) And (tpu_line[2] = 78) Then
            Begin
              { Const name Type value }
              tpu_pos := 6;  { Skip 'Const ' }
              TPUParseIdent;
              idx := SymAdd(SYM_CONST, TYPE_INTEGER, 0, 0);
              TPUSkipSpaces;
              sym_type[idx] := TPUParseType;
              TPUSkipSpaces;
              sym_const_val[idx] := TPUParseInt
            End
            Else If (tpu_line[0] = 86) And (tpu_line[1] = 65) And (tpu_line[2] = 82) Then
            Begin
              { Var name Type offset }
              tpu_pos := 4;  { Skip 'Var ' }
              TPUParseIdent;
              idx := SymAdd(SYM_VAR, TYPE_INTEGER, 0, 0);
              TPUSkipSpaces;
              sym_type[idx] := TPUParseType;
              TPUSkipSpaces;
              sym_offset[idx] := TPUParseInt
            End
            Else If (tpu_line[0] = 80) And (tpu_line[1] = 82) And (tpu_line[2] = 79) Then
            Begin
              { PROC name label flags }
              tpu_pos := 5;  { Skip 'PROC ' }
              TPUParseIdent;
              idx := SymAdd(SYM_PROCEDURE, TYPE_VOID, 0, 0);
              sym_unit_idx[idx] := loaded_count;  { Mark as imported from this Unit }
              TPUSkipSpaces;
              sym_label[idx] := TPUParseInt;
              TPUSkipSpaces;
              sym_var_param_flags[idx] := TPUParseInt
            End
            Else If (tpu_line[0] = 70) And (tpu_line[1] = 85) And (tpu_line[2] = 78) Then
            Begin
              { FUNC name Type label flags }
              tpu_pos := 5;  { Skip 'FUNC ' }
              TPUParseIdent;
              idx := SymAdd(SYM_FUNCTION, TYPE_INTEGER, 0, 0);
              sym_unit_idx[idx] := loaded_count;  { Mark as imported from this Unit }
              TPUSkipSpaces;
              sym_type[idx] := TPUParseType;
              TPUSkipSpaces;
              sym_label[idx] := TPUParseInt;
              TPUSkipSpaces;
              sym_var_param_flags[idx] := TPUParseInt
            End
            Else If (tpu_line[0] = 84) And (tpu_line[1] = 89) And (tpu_line[2] = 80) Then
            Begin
              { Type name kind const_val label }
              tpu_pos := 5;  { Skip 'Type ' }
              TPUParseIdent;
              idx := SymAdd(SYM_TYPEDEF, TYPE_RECORD, 0, 0);
              TPUSkipSpaces;
              sym_type[idx] := TPUParseType;
              TPUSkipSpaces;
              sym_const_val[idx] := TPUParseInt;
              TPUSkipSpaces;
              sym_label[idx] := TPUParseInt
            End
          End
        End
      End
    End
  End
End;

{ Parse Uses clause: Uses Unit1, Unit2, ...; }
Procedure ParseUsesClause;
Var
  i: Integer;
Begin
  NextToken;  { consume 'Uses' }
  Repeat
    If tok_type <> TOK_IDENT Then
      Error(6);  { expected identifier }

    { Build TPU filename In tok_str: unitname.tpu (lowercase For Case-insensitivity) }
    For i := 0 To tok_len - 1 Do
      tok_str[i] := ToLower(tok_str[i]);
    tok_str[tok_len] := 46;      { . }
    tok_str[tok_len + 1] := 116; { t }
    tok_str[tok_len + 2] := 112; { p }
    tok_str[tok_len + 3] := 117; { u }
    tok_str[tok_len + 4] := 0;

    { Open TPU file }
    assigntokstr(tpu_file, 0, tok_len + 4);
    reset(tpu_file);

    { Load Interface symbols }
    If LoadTPUInterface < 0 Then
      Error(18);  { Unit Not found Or invalid }

    close(tpu_file);

    NextToken;
    If tok_type = TOK_COMMA Then
      NextToken
  Until tok_type = TOK_SEMICOLON;
  Expect(TOK_SEMICOLON)
End;

{ Parse a Unit: Unit Name; Interface ... Implementation ... End. }
Procedure ParseUnit;
Var
  unit_lbl: Integer;
  i: Integer;
Begin
  { Unit Name; }
  NextToken;  { consume 'Unit' }
  If tok_type <> TOK_IDENT Then
    Error(6);  { expected identifier }

  { Store Unit name For qualified symbol names }
  current_unit_len := tok_len;
  For i := 0 To tok_len - 1 Do
    current_unit_name[i] := tok_str[i];
  current_unit_name[tok_len] := 0;

  compiling_unit := 1;
  NextToken;
  Expect(TOK_SEMICOLON);

  { Emit Unit header - no _main, just label For Unit init }
  EmitAlign4;

  { Jump over runtime routines (units still need runtime For their code) }
  unit_lbl := NewLabel;
  EmitBranchLabel(unit_lbl);

  { Emit runtime routines needed by Unit code }
  rt_print_int := NewLabel;
  rt_newline := NewLabel;
  rt_readchar := NewLabel;
  rt_print_char := NewLabel;
  rt_write_char_fd := NewLabel;
  rt_read_int := NewLabel;
  rt_skip_line := NewLabel;
  rt_print_string := NewLabel;
  rt_print_real := NewLabel;
  rt_read_real := NewLabel;
  rt_read_string := NewLabel;
  rt_heap_init := NewLabel;
  rt_alloc := NewLabel;
  rt_free := NewLabel;
  rt_str_copy := NewLabel;
  rt_str_compare := NewLabel;
  rt_str_concat := NewLabel;
  rt_str_cmp := NewLabel;
  rt_str_pos := NewLabel;
  rt_str_delete := NewLabel;
  rt_str_insert := NewLabel;
  rt_int_to_str := NewLabel;
  rt_str_to_int := NewLabel;
  rt_str_ltrim := NewLabel;
  rt_str_rtrim := NewLabel;
  rt_str_trim := NewLabel;
  rt_clrscr := NewLabel;
  rt_gotoxy := NewLabel;
  rt_clreol := NewLabel;
  rt_textcolor := NewLabel;
  rt_textbackground := NewLabel;
  rt_normvideo := NewLabel;
  rt_highvideo := NewLabel;
  rt_lowvideo := NewLabel;
  rt_hidecursor := NewLabel;
  rt_showcursor := NewLabel;
  rt_sleep := NewLabel;
  rt_keypressed := NewLabel;
  rt_initkeyboard := NewLabel;
  rt_donekeyboard := NewLabel;
  rt_sin := NewLabel;
  rt_cos := NewLabel;
  rt_tan := NewLabel;
  rt_exp := NewLabel;
  rt_ln := NewLabel;
  rt_random := NewLabel;
  rt_arctan := NewLabel;
  rt_arcsin := NewLabel;
  rt_arccos := NewLabel;
  rt_paramstr := NewLabel;

  EmitPrintIntRuntime;
  EmitNewlineRuntime;
  EmitReadcharRuntime;
  EmitPrintCharRuntime;
  EmitWriteCharFdRuntime;
  EmitReadIntRuntime;
  EmitSkipLineRuntime;
  EmitPrintStringRuntime;
  EmitPrintRealRuntime;
  EmitReadRealRuntime;
  EmitReadStringRuntime;
  EmitHeapInitRuntime;
  EmitAllocRuntime;
  EmitFreeRuntime;
  EmitStrCopyRuntime;
  EmitStrCompareRuntime;
  EmitStrConcatRuntime;
  EmitStrCmpRuntime;
  EmitStrPosRuntime;
  EmitStrDeleteRuntime;
  EmitStrInsertRuntime;
  EmitIntToStrRuntime;
  EmitStrToIntRuntime;
  EmitStrLtrimRuntime;
  EmitStrRtrimRuntime;
  EmitStrTrimRuntime;
  EmitClrScrRuntime;
  EmitGotoXYRuntime;
  EmitClrEolRuntime;
  EmitTextColorRuntime;
  EmitTextBackgroundRuntime;
  EmitNormVideoRuntime;
  EmitHighVideoRuntime;
  EmitLowVideoRuntime;
  EmitHideCursorRuntime;
  EmitShowCursorRuntime;
  EmitSleepRuntime;
  EmitKeyPressedRuntime;
  EmitInitKeyboardRuntime;
  EmitDoneKeyboardRuntime;
  EmitSinRuntime;
  EmitCosRuntime;
  EmitTanRuntime;
  EmitExpRuntime;
  EmitLnRuntime;
  EmitRandomRuntime;
  EmitArctanRuntime;
  EmitArcsinRuntime;
  EmitArccosRuntime;
  EmitParamStrRuntime;

  EmitLabel(unit_lbl);

  { Interface section }
  Expect(TOK_INTERFACE);
  in_interface := 1;
  interface_start := sym_count;  { Track first Interface symbol }

  { Parse Interface declarations (Const, Type, Var, Procedure/Function headers) }
  While (tok_type = TOK_CONST) Or (tok_type = TOK_TYPE_KW) Or
        (tok_type = TOK_VAR) Or (tok_type = TOK_PROCEDURE) Or
        (tok_type = TOK_FUNCTION) Do
  Begin
    If tok_type = TOK_CONST Then
      ParseConstDeclarations
    Else If tok_type = TOK_TYPE_KW Then
      ParseTypeDeclarations
    Else If tok_type = TOK_VAR Then
      ParseVarDeclarations
    Else If tok_type = TOK_PROCEDURE Then
    Begin
      { In Interface, only parse the header }
      ParseProcedureDeclaration
    End
    Else If tok_type = TOK_FUNCTION Then
    Begin
      ParseFunctionDeclaration
    End
  End;

  interface_end := sym_count - 1;  { Track last Interface symbol }
  in_interface := 0;

  { Implementation section }
  Expect(TOK_IMPLEMENTATION);

  { Parse Implementation (full Procedure/Function bodies) }
  While (tok_type = TOK_CONST) Or (tok_type = TOK_TYPE_KW) Or
        (tok_type = TOK_VAR) Or (tok_type = TOK_PROCEDURE) Or
        (tok_type = TOK_FUNCTION) Do
  Begin
    If tok_type = TOK_CONST Then
      ParseConstDeclarations
    Else If tok_type = TOK_TYPE_KW Then
      ParseTypeDeclarations
    Else If tok_type = TOK_VAR Then
      ParseVarDeclarations
    Else If tok_type = TOK_PROCEDURE Then
      ParseProcedureDeclaration
    Else If tok_type = TOK_FUNCTION Then
      ParseFunctionDeclaration
  End;

  { Unit initialization entry point - always emit even If no init code }
  EmitGloblUnitInit;
  EmitUnitInitLabel;
  EmitStp;
  EmitMovFP;

  { Optional initialization section }
  If tok_type = TOK_BEGIN Then
  Begin
    NextToken;
    While tok_type <> TOK_END Do
    Begin
      ParseStatement;
      If tok_type = TOK_SEMICOLON Then
        NextToken
    End;
    Expect(TOK_END)
  End;

  Expect(TOK_DOT);

  { Unit init returns To caller }
  EmitLdp;
  EmitRet;

  { Write TPU file With Interface information }
  WriteTPUFile;

  compiling_unit := 0
End;

Procedure ParseProgram;
Var
  main_lbl, i: Integer;
Begin
  { Check If this is a Unit Or Program }
  If tok_type = TOK_UNIT Then
  Begin
    ParseUnit;
    Halt(0)  { Unit compilation done }
  End;

  Expect(TOK_PROGRAM);
  If tok_type <> TOK_IDENT Then
    Error(11);
  NextToken;
  Expect(TOK_SEMICOLON);

  { Check For Uses clause }
  If tok_type = TOK_USES Then
    ParseUsesClause;

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
  rt_write_char_fd := NewLabel;
  rt_read_int := NewLabel;
  rt_skip_line := NewLabel;
  rt_print_string := NewLabel;
  rt_print_real := NewLabel;
  rt_read_real := NewLabel;
  rt_read_string := NewLabel;
  rt_heap_init := NewLabel;
  rt_alloc := NewLabel;
  rt_free := NewLabel;
  rt_str_copy := NewLabel;
  rt_str_compare := NewLabel;
  rt_str_concat := NewLabel;
  rt_str_cmp := NewLabel;
  rt_str_pos := NewLabel;
  rt_str_delete := NewLabel;
  rt_str_insert := NewLabel;
  rt_int_to_str := NewLabel;
  rt_str_to_int := NewLabel;
  rt_str_ltrim := NewLabel;
  rt_str_rtrim := NewLabel;
  rt_str_trim := NewLabel;
  rt_clrscr := NewLabel;
  rt_gotoxy := NewLabel;
  rt_clreol := NewLabel;
  rt_textcolor := NewLabel;
  rt_textbackground := NewLabel;
  rt_normvideo := NewLabel;
  rt_highvideo := NewLabel;
  rt_lowvideo := NewLabel;
  rt_hidecursor := NewLabel;
  rt_showcursor := NewLabel;
  rt_sleep := NewLabel;
  rt_keypressed := NewLabel;
  rt_initkeyboard := NewLabel;
  rt_donekeyboard := NewLabel;
  rt_sin := NewLabel;
  rt_cos := NewLabel;
  rt_tan := NewLabel;
  rt_exp := NewLabel;
  rt_ln := NewLabel;
  rt_random := NewLabel;
  rt_arctan := NewLabel;
  rt_arcsin := NewLabel;
  rt_arccos := NewLabel;
  rt_paramstr := NewLabel;

  EmitPrintIntRuntime;
  EmitNewlineRuntime;
  EmitReadcharRuntime;
  EmitPrintCharRuntime;
  EmitWriteCharFdRuntime;
  EmitReadIntRuntime;
  EmitSkipLineRuntime;
  EmitPrintStringRuntime;
  EmitPrintRealRuntime;
  EmitReadRealRuntime;
  EmitReadStringRuntime;
  EmitHeapInitRuntime;
  EmitAllocRuntime;
  EmitFreeRuntime;
  EmitStrCopyRuntime;
  EmitStrCompareRuntime;
  EmitStrConcatRuntime;
  EmitStrCmpRuntime;
  EmitStrPosRuntime;
  EmitStrDeleteRuntime;
  EmitStrInsertRuntime;
  EmitIntToStrRuntime;
  EmitStrToIntRuntime;
  EmitStrLtrimRuntime;
  EmitStrRtrimRuntime;
  EmitStrTrimRuntime;
  EmitClrScrRuntime;
  EmitGotoXYRuntime;
  EmitClrEolRuntime;
  EmitTextColorRuntime;
  EmitTextBackgroundRuntime;
  EmitNormVideoRuntime;
  EmitHighVideoRuntime;
  EmitLowVideoRuntime;
  EmitHideCursorRuntime;
  EmitShowCursorRuntime;
  EmitSleepRuntime;
  EmitKeyPressedRuntime;
  EmitInitKeyboardRuntime;
  EmitDoneKeyboardRuntime;
  EmitSinRuntime;
  EmitCosRuntime;
  EmitTanRuntime;
  EmitExpRuntime;
  EmitLnRuntime;
  EmitRandomRuntime;
  EmitArctanRuntime;
  EmitArcsinRuntime;
  EmitArccosRuntime;
  EmitParamStrRuntime;

  { Main Program entry }
  EmitLabel(main_lbl);
  EmitStp;
  EmitMovFP;

  { Save argc To x25, argv To x26 (before any calls clobber x0/x1) }
  { mov x25, x0 }
  EmitIndent;
  WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
  WriteChar(120); WriteChar(50); WriteChar(53); WriteChar(44); WriteChar(32);  { x25, }
  WriteChar(120); WriteChar(48);  { x0 }
  EmitNL;
  { mov x26, x1 }
  EmitIndent;
  WriteChar(109); WriteChar(111); WriteChar(118); WriteChar(32);  { mov }
  WriteChar(120); WriteChar(50); WriteChar(54); WriteChar(44); WriteChar(32);  { x26, }
  WriteChar(120); WriteChar(49);  { x1 }
  EmitNL;

  EmitFileOpenInit;
  EmitBL(rt_heap_init);

  { Call Unit initialization routines }
  For i := 0 To loaded_count - 1 Do
    EmitBLUnitInit(i);

  ParseBlock;

  Expect(TOK_DOT);

  { Exit syscall }
  EmitMovX0(0);
  EmitMovX16(33554433);  { 0x2000001 }
  EmitSvc
End;

{ ----- Main ----- }

Begin
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
  rt_read_int := 0;
  rt_skip_line := 0;
  rt_print_string := 0;
  rt_print_real := 0;
  rt_read_real := 0;
  rt_read_string := 0;
  rt_heap_init := 0;
  rt_str_copy := 0;
  rt_str_compare := 0;
  rt_str_concat := 0;
  string_temp_idx := 0;
  out_fd := 1;
  pushback_ch := -1;
  expr_type := TYPE_INTEGER;
  ptr_base_type := TYPE_INTEGER;
  field_count := 0;
  with_rec_idx := -1;
  with_rec_type := 0;
  break_label := 0;
  continue_label := 0;
  exit_label := 0;
  ptr_arr_count := 0;
  file_count := 0;
  enum_count := 0;
  subr_count := 0;
  set_count := 0;
  rt_alloc := 0;
  rt_free := 0;
  include_depth := 0;
  compiling_unit := 0;
  in_interface := 0;
  current_unit_len := 0;
  interface_start := 0;
  interface_end := 0;
  loaded_count := 0;
  tpu_line_len := 0;
  tpu_pos := 0;

