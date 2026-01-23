{ ----- Declarations ----- }

procedure ParseVarDeclarations;
var
  idx, first_idx, arr_size, lo_bound, hi_bound, j, base_idx: integer;
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
    else if tok_type = TOK_REAL_TYPE then
    begin
      { Set type to real for all vars in this group }
      for j := first_idx to idx do
        sym_type[j] := TYPE_REAL;
      NextToken
    end
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
      begin
        NextToken;
        arr_size := (hi_bound - lo_bound + 1) * 8;
        local_offset := local_offset - (arr_size - 8);
        sym_type[first_idx] := TYPE_ARRAY;
        sym_const_val[first_idx] := lo_bound;
        sym_label[first_idx] := arr_size;
        sym_var_param_flags[first_idx] := 0  { 0 = basic type element }
      end
      else if tok_type = TOK_IDENT then
      begin
        { Array of record type }
        base_idx := SymLookup;
        if (base_idx >= 0) and (sym_kind[base_idx] = SYM_TYPEDEF) and (sym_type[base_idx] = TYPE_RECORD) then
        begin
          arr_size := (hi_bound - lo_bound + 1) * sym_label[base_idx];
          local_offset := local_offset - (arr_size - 8);
          sym_type[first_idx] := TYPE_ARRAY;
          sym_const_val[first_idx] := lo_bound;
          sym_label[first_idx] := arr_size;
          sym_var_param_flags[first_idx] := base_idx + 1;  { record type index + 1 (0 means basic) }
          NextToken
        end
        else
          Error(9)
      end
      else
        Error(9)
    end
    else if tok_type = TOK_STRING_TYPE then
    begin
      { String type: 256 bytes (1 length byte + 255 char bytes) }
      NextToken;
      { Handle all variables in the list }
      for j := first_idx to idx do
      begin
        sym_type[j] := TYPE_STRING;
        sym_label[j] := 256;  { Store size for reference }
        { Adjust offset: already allocated 8 bytes, need 248 more }
        if j = first_idx then
        begin
          local_offset := local_offset - 248;
          sym_offset[j] := local_offset  { Update offset to start of 256-byte area }
        end
        else
        begin
          local_offset := local_offset - 256;
          sym_offset[j] := local_offset
        end
      end
    end
    else if tok_type = TOK_TEXT then
    begin
      { Text file type: 272 bytes (fd + mode + filename) }
      NextToken;
      for j := first_idx to idx do
      begin
        sym_type[j] := TYPE_TEXT;
        sym_const_val[j] := TYPE_CHAR;  { element type is char }
        sym_label[j] := 1;  { element size is 1 byte }
        { Adjust offset: already allocated 8 bytes, need 264 more for 272 total }
        if j = first_idx then
        begin
          local_offset := local_offset - 264;
          sym_offset[j] := local_offset
        end
        else
        begin
          local_offset := local_offset - 272;
          sym_offset[j] := local_offset
        end
      end
    end
    else if tok_type = TOK_FILE then
    begin
      { Typed file: file of T - 272 bytes (fd + mode + filename) }
      NextToken;
      Expect(TOK_OF);
      { Parse element type }
      if tok_type = TOK_INTEGER_TYPE then
      begin
        base_idx := TYPE_INTEGER;
        arr_size := 8;
        NextToken
      end
      else if tok_type = TOK_CHAR_TYPE then
      begin
        base_idx := TYPE_CHAR;
        arr_size := 1;
        NextToken
      end
      else if tok_type = TOK_BOOLEAN_TYPE then
      begin
        base_idx := TYPE_BOOLEAN;
        arr_size := 1;
        NextToken
      end
      else if tok_type = TOK_REAL_TYPE then
      begin
        base_idx := TYPE_REAL;
        arr_size := 8;
        NextToken
      end
      else if tok_type = TOK_IDENT then
      begin
        { Record type }
        lo_bound := SymLookup;
        if (lo_bound >= 0) and (sym_kind[lo_bound] = SYM_TYPEDEF) and (sym_type[lo_bound] = TYPE_RECORD) then
        begin
          base_idx := TYPE_RECORD;
          arr_size := sym_label[lo_bound];  { record size }
          file_rec_idx[file_count] := lo_bound;
          NextToken
        end
        else
          Error(14)
      end
      else
        Error(9);
      { Store file type info }
      file_elem_type[file_count] := base_idx;
      file_elem_size[file_count] := arr_size;
      for j := first_idx to idx do
      begin
        sym_type[j] := TYPE_FILE;
        sym_const_val[j] := file_count;  { index into file arrays }
        sym_label[j] := arr_size;  { element size }
        { Adjust offset for 272 bytes }
        if j = first_idx then
        begin
          local_offset := local_offset - 264;
          sym_offset[j] := local_offset
        end
        else
        begin
          local_offset := local_offset - 272;
          sym_offset[j] := local_offset
        end
      end;
      file_count := file_count + 1
    end
    else if tok_type = TOK_SET then
    begin
      { Inline set type: set of char / set of 0..63 }
      NextToken;  { consume 'set' }
      Expect(TOK_OF);
      if tok_type = TOK_CHAR_TYPE then
      begin
        { set of char - uses 64-bit bitmask }
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_SET;
          sym_const_val[j] := set_count
        end;
        set_base[set_count] := TYPE_CHAR;
        set_low[set_count] := 0;
        set_high[set_count] := 63;  { limited to 64 elements }
        set_count := set_count + 1;
        NextToken
      end
      else if tok_type = TOK_INTEGER then
      begin
        { set of 0..N - inline subrange }
        lo_bound := tok_int;
        NextToken;
        Expect(TOK_DOTDOT);
        if tok_type <> TOK_INTEGER then Error(9);
        hi_bound := tok_int;
        NextToken;
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_SET;
          sym_const_val[j] := set_count
        end;
        set_base[set_count] := TYPE_INTEGER;
        set_low[set_count] := lo_bound;
        set_high[set_count] := hi_bound;
        set_count := set_count + 1
      end
      else if tok_type = TOK_IDENT then
      begin
        { set of EnumType }
        base_idx := SymLookup;
        if (base_idx >= 0) and (sym_kind[base_idx] = SYM_TYPEDEF) and
           (sym_type[base_idx] = TYPE_ENUM) then
        begin
          for j := first_idx to idx do
          begin
            sym_type[j] := TYPE_SET;
            sym_const_val[j] := set_count
          end;
          set_base[set_count] := TYPE_ENUM;
          set_low[set_count] := 0;
          set_high[set_count] := sym_label[base_idx] - 1;  { enum count - 1 }
          set_count := set_count + 1;
          NextToken
        end
        else
          Error(9)
      end
      else
        Error(9)
    end
    else if tok_type = TOK_CARET then
    begin
      { Pointer type: ^BaseType or ^^BaseType or ^array[lo..hi] of T }
      NextToken;
      lo_bound := 1;  { reuse lo_bound for depth count }
      while tok_type = TOK_CARET do
      begin
        lo_bound := lo_bound + 1;
        NextToken
      end;
      { Check for pointer to array }
      if tok_type = TOK_ARRAY then
      begin
        { ^array[lo..hi] of T }
        NextToken;  { consume 'array' }
        Expect(TOK_LBRACKET);
        if tok_type <> TOK_INTEGER then
          Error(9);
        hi_bound := tok_int;  { reuse hi_bound temporarily for low }
        NextToken;
        Expect(TOK_DOTDOT);
        if tok_type <> TOK_INTEGER then
          Error(9);
        arr_size := tok_int;  { high bound }
        NextToken;
        Expect(TOK_RBRACKET);
        Expect(TOK_OF);
        { Parse element type }
        if tok_type = TOK_INTEGER_TYPE then
          base_idx := TYPE_INTEGER
        else if tok_type = TOK_CHAR_TYPE then
          base_idx := TYPE_CHAR
        else if tok_type = TOK_BOOLEAN_TYPE then
          base_idx := TYPE_BOOLEAN
        else if tok_type = TOK_REAL_TYPE then
          base_idx := TYPE_REAL
        else
          base_idx := TYPE_INTEGER;  { default }
        { Store in ptr_arr arrays }
        ptr_arr_lo[ptr_arr_count] := hi_bound;
        ptr_arr_hi[ptr_arr_count] := arr_size;
        ptr_arr_elem[ptr_arr_count] := base_idx;
        ptr_arr_rec[ptr_arr_count] := 0;
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_POINTER;
          sym_const_val[j] := TYPE_ARRAY;  { immediate base is array }
          sym_label[j] := ptr_arr_count;   { index into ptr_arr arrays }
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_ARRAY;
          ptr_ultimate_rec[j] := 0
        end;
        ptr_arr_count := ptr_arr_count + 1;
        NextToken
      end
      { Now parse ultimate base type }
      else if tok_type = TOK_INTEGER_TYPE then
      begin
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_POINTER;
          if lo_bound = 1 then
            sym_const_val[j] := TYPE_INTEGER
          else
            sym_const_val[j] := TYPE_POINTER;  { immediate base is pointer }
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_INTEGER;
          ptr_ultimate_rec[j] := 0
        end;
        NextToken
      end
      else if tok_type = TOK_CHAR_TYPE then
      begin
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_POINTER;
          if lo_bound = 1 then
            sym_const_val[j] := TYPE_CHAR
          else
            sym_const_val[j] := TYPE_POINTER;
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_CHAR;
          ptr_ultimate_rec[j] := 0
        end;
        NextToken
      end
      else if tok_type = TOK_BOOLEAN_TYPE then
      begin
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_POINTER;
          if lo_bound = 1 then
            sym_const_val[j] := TYPE_BOOLEAN
          else
            sym_const_val[j] := TYPE_POINTER;
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_BOOLEAN;
          ptr_ultimate_rec[j] := 0
        end;
        NextToken
      end
      else if tok_type = TOK_REAL_TYPE then
      begin
        for j := first_idx to idx do
        begin
          sym_type[j] := TYPE_POINTER;
          if lo_bound = 1 then
            sym_const_val[j] := TYPE_REAL
          else
            sym_const_val[j] := TYPE_POINTER;
          ptr_depth[j] := lo_bound;
          ptr_ultimate_type[j] := TYPE_REAL;
          ptr_ultimate_rec[j] := 0
        end;
        NextToken
      end
      else if tok_type = TOK_IDENT then
      begin
        { Pointer to record type: ^RecordType or ^^RecordType etc }
        arr_size := SymLookup;  { reuse arr_size as type_idx }
        if (arr_size >= 0) and (sym_kind[arr_size] = SYM_TYPEDEF) and (sym_type[arr_size] = TYPE_RECORD) then
        begin
          for j := first_idx to idx do
          begin
            sym_type[j] := TYPE_POINTER;
            if lo_bound = 1 then
              sym_const_val[j] := TYPE_RECORD
            else
              sym_const_val[j] := TYPE_POINTER;
            sym_label[j] := arr_size;  { store record type index }
            ptr_depth[j] := lo_bound;
            ptr_ultimate_type[j] := TYPE_RECORD;
            ptr_ultimate_rec[j] := arr_size
          end;
          NextToken
        end
        else
          Error(14)  { expected type identifier }
      end
      else
        Error(14)  { expected type identifier }
    end
    else if tok_type = TOK_IDENT then
    begin
      { May be a record, enum, subrange, or set type name }
      arr_size := SymLookup;  { reuse arr_size as type_idx temporarily }
      if (arr_size >= 0) and (sym_kind[arr_size] = SYM_TYPEDEF) then
      begin
        if sym_type[arr_size] = TYPE_RECORD then
        begin
          { Allocate space for record }
          lo_bound := sym_label[arr_size];  { reuse lo_bound for record size }
          { Adjust local_offset: we already allocated 8 bytes, need rest }
          local_offset := local_offset - (lo_bound - 8);
          for j := first_idx to idx do
          begin
            sym_type[j] := TYPE_RECORD;
            sym_const_val[j] := arr_size  { link to type definition }
          end
        end
        else if sym_type[arr_size] = TYPE_ENUM then
        begin
          { Enum variables use 8 bytes (same as integer) }
          for j := first_idx to idx do
          begin
            sym_type[j] := TYPE_ENUM;
            sym_const_val[j] := arr_size  { link to type definition }
          end
        end
        else if sym_type[arr_size] = TYPE_SUBRANGE then
        begin
          { Subrange variables use 8 bytes (same as integer) }
          for j := first_idx to idx do
          begin
            sym_type[j] := TYPE_SUBRANGE;
            sym_const_val[j] := arr_size  { link to type definition }
          end
        end
        else if sym_type[arr_size] = TYPE_SET then
        begin
          { Set variables use 8 bytes (64-bit bitmask) }
          for j := first_idx to idx do
          begin
            sym_type[j] := TYPE_SET;
            sym_const_val[j] := arr_size  { link to type definition }
          end
        end
        else
          Error(9);
        NextToken
      end
      else
        Error(9)
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

procedure ParseTypeDeclarations;
var
  type_idx, fld_start, fld_offset, fld_type: integer;
  i, base_idx, nested_idx, nested_size, first_fld: integer;
  enum_val, lo_val, hi_val, set_base_type: integer;
begin
  NextToken;  { consume 'type' }
  while tok_type = TOK_IDENT do
  begin
    { Create type symbol - will set actual type later }
    type_idx := SymAdd(SYM_TYPEDEF, TYPE_RECORD, scope_level, 0);
    NextToken;
    Expect(TOK_EQ);

    if tok_type = TOK_RECORD then
    begin
      { Record type }
      NextToken;  { consume 'record' }
      fld_start := field_count;
      fld_offset := 0;
      sym_const_val[type_idx] := fld_start;  { first field index }

      { Parse fields until 'end' or 'case' (variant part) }
      while (tok_type <> TOK_END) and (tok_type <> TOK_CASE) do
      begin
        { Collect field names (may be comma-separated) }
        first_fld := field_count;
        repeat
          if tok_type <> TOK_IDENT then
            Error(11);

          { Save field name }
          base_idx := field_count * 32;
          for i := 0 to tok_len - 1 do
            field_name[base_idx + i] := tok_str[i];
          field_name[base_idx + tok_len] := 0;
          field_count := field_count + 1;

          NextToken;
          if tok_type = TOK_COMMA then
            NextToken
        until tok_type = TOK_COLON;

        Expect(TOK_COLON);

        { Parse field type }
        nested_size := 8;  { default for basic types }
        if tok_type = TOK_INTEGER_TYPE then
        begin
          fld_type := TYPE_INTEGER;
          nested_idx := 0;
          NextToken
        end
        else if tok_type = TOK_CHAR_TYPE then
        begin
          fld_type := TYPE_CHAR;
          nested_idx := 0;
          NextToken
        end
        else if tok_type = TOK_BOOLEAN_TYPE then
        begin
          fld_type := TYPE_BOOLEAN;
          nested_idx := 0;
          NextToken
        end
        else if tok_type = TOK_REAL_TYPE then
        begin
          fld_type := TYPE_REAL;
          nested_idx := 0;
          NextToken
        end
        else if tok_type = TOK_IDENT then
        begin
          { Nested record type or enum type }
          nested_idx := SymLookup;
          if (nested_idx >= 0) and (sym_kind[nested_idx] = SYM_TYPEDEF) then
          begin
            if sym_type[nested_idx] = TYPE_RECORD then
            begin
              fld_type := TYPE_RECORD;
              nested_size := sym_label[nested_idx]
            end
            else if sym_type[nested_idx] = TYPE_ENUM then
              fld_type := TYPE_ENUM
            else if sym_type[nested_idx] = TYPE_SUBRANGE then
              fld_type := TYPE_SUBRANGE
            else if sym_type[nested_idx] = TYPE_SET then
              fld_type := TYPE_SET
            else
              Error(9);
            NextToken
          end
          else
            Error(9)
        end
        else
          Error(9);

        { Apply type to all collected field names }
        for i := first_fld to field_count - 1 do
        begin
          field_type[i] := fld_type;
          field_offset[i] := fld_offset;
          field_rec_idx[i] := type_idx;
          field_rec_type[i] := nested_idx;
          if fld_type = TYPE_RECORD then
            fld_offset := fld_offset + nested_size
          else
            fld_offset := fld_offset + 8
        end;

        { Expect semicolon or end }
        if tok_type = TOK_SEMICOLON then
          NextToken
      end;

      { Check for variant part }
      if tok_type = TOK_CASE then
      begin
        NextToken;  { consume 'case' }

        { Check for tag field: "case tag: type of" vs "case type of" }
        if tok_type = TOK_IDENT then
        begin
          { Could be "tag: type" or just "type" - peek ahead }
          { Save identifier for potential tag field }
          base_idx := field_count * 32;
          for i := 0 to tok_len - 1 do
            field_name[base_idx + i] := tok_str[i];
          field_name[base_idx + tok_len] := 0;

          NextToken;
          if tok_type = TOK_COLON then
          begin
            { It's a tag field }
            field_count := field_count + 1;
            NextToken;
            { Parse tag type }
            if tok_type = TOK_INTEGER_TYPE then
              fld_type := TYPE_INTEGER
            else if tok_type = TOK_CHAR_TYPE then
              fld_type := TYPE_CHAR
            else if tok_type = TOK_BOOLEAN_TYPE then
              fld_type := TYPE_BOOLEAN
            else
              fld_type := TYPE_INTEGER;  { default }
            NextToken;

            { Record tag field }
            field_type[field_count - 1] := fld_type;
            field_offset[field_count - 1] := fld_offset;
            field_rec_idx[field_count - 1] := type_idx;
            field_rec_type[field_count - 1] := 0;
            fld_offset := fld_offset + 8
          end
          { else: identifier was the type name, already consumed }
        end
        else
          NextToken;  { consume type keyword }

        Expect(TOK_OF);

        { variant_start marks where all variants begin }
        nested_idx := fld_offset;  { reuse as variant_start }
        nested_size := 0;          { reuse as max_variant_size }

        { Parse variant alternatives }
        while tok_type <> TOK_END do
        begin
          { Skip constant label(s) }
          while (tok_type <> TOK_COLON) and (tok_type <> TOK_END) do
            NextToken;

          if tok_type = TOK_COLON then
          begin
          NextToken;  { consume ':' }
          Expect(TOK_LPAREN);

          { Parse fields in this variant - all start at variant_start }
          fld_offset := nested_idx;  { reset to variant start }

          while tok_type <> TOK_RPAREN do
          begin
            { Collect field names }
            first_fld := field_count;
            repeat
              if tok_type <> TOK_IDENT then
                Error(11);
              base_idx := field_count * 32;
              for i := 0 to tok_len - 1 do
                field_name[base_idx + i] := tok_str[i];
              field_name[base_idx + tok_len] := 0;
              field_count := field_count + 1;
              NextToken;
              if tok_type = TOK_COMMA then
                NextToken
            until tok_type = TOK_COLON;

            Expect(TOK_COLON);

            { Parse field type }
            lo_val := 8;  { reuse as field_size }
            if tok_type = TOK_INTEGER_TYPE then
              fld_type := TYPE_INTEGER
            else if tok_type = TOK_CHAR_TYPE then
              fld_type := TYPE_CHAR
            else if tok_type = TOK_BOOLEAN_TYPE then
              fld_type := TYPE_BOOLEAN
            else if tok_type = TOK_REAL_TYPE then
              fld_type := TYPE_REAL
            else if tok_type = TOK_IDENT then
            begin
              hi_val := SymLookup;  { reuse as lookup result }
              if (hi_val >= 0) and (sym_kind[hi_val] = SYM_TYPEDEF) then
              begin
                if sym_type[hi_val] = TYPE_RECORD then
                begin
                  fld_type := TYPE_RECORD;
                  lo_val := sym_label[hi_val]
                end
                else
                  fld_type := sym_type[hi_val]
              end
              else
                fld_type := TYPE_INTEGER
            end
            else
              fld_type := TYPE_INTEGER;
            NextToken;

            { Record fields }
            for i := first_fld to field_count - 1 do
            begin
              field_type[i] := fld_type;
              field_offset[i] := fld_offset;
              field_rec_idx[i] := type_idx;
              if fld_type = TYPE_RECORD then
                field_rec_type[i] := hi_val
              else
                field_rec_type[i] := 0;
              fld_offset := fld_offset + lo_val
            end;

            if tok_type = TOK_SEMICOLON then
              NextToken
          end;

          NextToken;  { consume ')' }

          { Track max variant size }
          if (fld_offset - nested_idx) > nested_size then
            nested_size := fld_offset - nested_idx;

          if tok_type = TOK_SEMICOLON then
            NextToken
          end  { end if tok_type = TOK_COLON }
        end;

        fld_offset := nested_idx + nested_size  { final size }
      end;

      NextToken;  { consume 'end' }
      sym_label[type_idx] := fld_offset  { total record size }
    end
    else if tok_type = TOK_LPAREN then
    begin
      { Enumerated type: (Red, Green, Blue) }
      sym_type[type_idx] := TYPE_ENUM;
      sym_const_val[type_idx] := enum_count;  { index into enum arrays }
      enum_low[enum_count] := 0;
      enum_val := 0;
      NextToken;  { consume '(' }
      while tok_type <> TOK_RPAREN do
      begin
        if tok_type <> TOK_IDENT then
          Error(11);
        { Add enum value as a constant }
        base_idx := SymAdd(SYM_CONST, TYPE_ENUM, scope_level, 0);
        sym_const_val[base_idx] := enum_val;
        sym_label[base_idx] := type_idx;  { link to enum type }
        enum_val := enum_val + 1;
        NextToken;
        if tok_type = TOK_COMMA then
          NextToken
      end;
      enum_high[enum_count] := enum_val - 1;
      sym_label[type_idx] := enum_val;  { number of values }
      enum_count := enum_count + 1;
      NextToken  { consume ')' }
    end
    else if tok_type = TOK_SET then
    begin
      { Set type: set of char / set of EnumType / set of 0..63 }
      sym_type[type_idx] := TYPE_SET;
      NextToken;  { consume 'set' }
      Expect(TOK_OF);
      sym_const_val[type_idx] := set_count;  { index into set arrays }
      if tok_type = TOK_CHAR_TYPE then
      begin
        set_base[set_count] := TYPE_CHAR;
        set_low[set_count] := 0;
        set_high[set_count] := 255;
        NextToken
      end
      else if tok_type = TOK_IDENT then
      begin
        base_idx := SymLookup;
        if (base_idx >= 0) and (sym_kind[base_idx] = SYM_TYPEDEF) then
        begin
          if sym_type[base_idx] = TYPE_ENUM then
          begin
            set_base[set_count] := TYPE_ENUM;
            nested_idx := sym_const_val[base_idx];  { enum index }
            set_low[set_count] := enum_low[nested_idx];
            set_high[set_count] := enum_high[nested_idx]
          end
          else if sym_type[base_idx] = TYPE_SUBRANGE then
          begin
            set_base[set_count] := TYPE_SUBRANGE;
            nested_idx := sym_const_val[base_idx];  { subrange index }
            set_low[set_count] := subr_low[nested_idx];
            set_high[set_count] := subr_high[nested_idx]
          end
          else
            Error(9);
          NextToken
        end
        else
          Error(9)
      end
      else if tok_type = TOK_INTEGER then
      begin
        { set of 0..63 - inline subrange }
        lo_val := tok_int;
        NextToken;
        Expect(TOK_DOTDOT);
        if tok_type <> TOK_INTEGER then
          Error(9);
        hi_val := tok_int;
        if hi_val - lo_val > 63 then
          Error(16);  { set too large }
        set_base[set_count] := TYPE_SUBRANGE;
        set_low[set_count] := lo_val;
        set_high[set_count] := hi_val;
        NextToken
      end
      else
        Error(9);
      set_count := set_count + 1
    end
    else if (tok_type = TOK_INTEGER) or (tok_type = TOK_MINUS) then
    begin
      { Subrange type: 0..9 or -10..10 }
      if tok_type = TOK_MINUS then
      begin
        NextToken;
        if tok_type <> TOK_INTEGER then
          Error(9);
        lo_val := 0 - tok_int
      end
      else
        lo_val := tok_int;
      NextToken;
      Expect(TOK_DOTDOT);
      if tok_type = TOK_MINUS then
      begin
        NextToken;
        if tok_type <> TOK_INTEGER then
          Error(9);
        hi_val := 0 - tok_int
      end
      else if tok_type = TOK_INTEGER then
        hi_val := tok_int
      else
        Error(9);
      sym_type[type_idx] := TYPE_SUBRANGE;
      sym_const_val[type_idx] := subr_count;  { index into subrange arrays }
      subr_low[subr_count] := lo_val;
      subr_high[subr_count] := hi_val;
      subr_base[subr_count] := TYPE_INTEGER;
      sym_label[type_idx] := hi_val - lo_val + 1;  { number of values }
      subr_count := subr_count + 1;
      NextToken
    end
    else if tok_type = TOK_IDENT then
    begin
      { Could be alias to another type, or subrange of enum }
      base_idx := SymLookup;
      if base_idx >= 0 then
      begin
        if sym_kind[base_idx] = SYM_CONST then
        begin
          { Subrange starting with enum constant }
          lo_val := sym_const_val[base_idx];
          nested_idx := sym_label[base_idx];  { enum type }
          NextToken;
          Expect(TOK_DOTDOT);
          if tok_type <> TOK_IDENT then
            Error(9);
          base_idx := SymLookup;
          if (base_idx < 0) or (sym_kind[base_idx] <> SYM_CONST) then
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
        end
        else if sym_kind[base_idx] = SYM_TYPEDEF then
        begin
          { Type alias - copy the type info }
          sym_type[type_idx] := sym_type[base_idx];
          sym_const_val[type_idx] := sym_const_val[base_idx];
          sym_label[type_idx] := sym_label[base_idx];
          NextToken
        end
        else
          Error(9)
      end
      else
        Error(9)
    end
    else
      Error(9);
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

  while (tok_type = TOK_CONST) or (tok_type = TOK_VAR) or (tok_type = TOK_TYPE_KW) do
  begin
    if tok_type = TOK_CONST then
      ParseConstDeclarations
    else if tok_type = TOK_TYPE_KW then
      ParseTypeDeclarations
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
  param_count, param_idx, i, j: integer;
  param_indices: array[0..7] of integer;
  is_var_group: integer;
  saved_exit_label, proc_exit_label: integer;
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
        if tok_type = TOK_SEMICOLON then NextToken;
        { Check for 'var' keyword - applies to all idents in this group }
        is_var_group := 0;
        if tok_type = TOK_VAR then
        begin
          is_var_group := 1;
          NextToken
        end;
        { Parse identifier list for this parameter group }
        if tok_type <> TOK_IDENT then Error(11);
        repeat
          if tok_type = TOK_COMMA then NextToken;
          if tok_type <> TOK_IDENT then Error(11);
          { Add parameter }
          local_offset := local_offset - 8;
          param_idx := SymAdd(SYM_PARAM, TYPE_INTEGER, scope_level, local_offset);
          if is_var_group = 1 then
            sym_is_var_param[param_idx] := 1;
          if param_count < 8 then
            param_indices[param_count] := param_idx;
          param_count := param_count + 1;
          NextToken
        until tok_type <> TOK_COMMA;
        { Parse type annotation and set param types }
        if tok_type = TOK_COLON then
        begin
          NextToken;
          if tok_type = TOK_REAL_TYPE then
          begin
            { Set all params in this group to TYPE_REAL }
            { Find starting index of this group and set types }
            { param_indices has the indices, but we need to know where this group started }
            { For simplicity, set all recent params to real - this is approximate }
            sym_type[param_idx] := TYPE_REAL;
            NextToken
          end
          else if tok_type = TOK_STRING_TYPE then
          begin
            { String parameter }
            sym_type[param_idx] := TYPE_STRING;
            if is_var_group = 0 then
            begin
              { Value parameter - need 256 bytes total, already have 8, need 248 more }
              local_offset := local_offset - 248;
              sym_offset[param_idx] := local_offset;  { Update offset to start of 256-byte area }
              sym_label[param_idx] := 256  { Mark as needing copy-in }
            end;
            { var string params keep the 8-byte slot for address }
            NextToken
          end
          else if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
             (tok_type = TOK_BOOLEAN_TYPE) then
            NextToken
        end
      until tok_type <> TOK_SEMICOLON
    end;
    Expect(TOK_RPAREN)
  end;

  { Build var-param bitmap and store on procedure symbol }
  sym_var_param_flags[idx] := 0;
  for i := 0 to param_count - 1 do
    if sym_is_var_param[param_indices[i]] = 1 then
    begin
      if i = 0 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 1;
      if i = 1 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 2;
      if i = 2 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 4;
      if i = 3 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 8;
      if i = 4 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 16;
      if i = 5 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 32;
      if i = 6 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 64;
      if i = 7 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 128
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
    { Allocate based on actual space needed (local_offset tracks it) }
    { param_space = -(local_offset) - 8 (static link already counted separately) }
    j := 0 - local_offset - 8;
    j := ((j + 15) div 16) * 16;  { Align to 16 }
    if j > 0 then
      EmitSubSP(j);
    for i := 0 to param_count - 1 do
    begin
      if i < 8 then
      begin
        { Check if this is a string value parameter (needs copy) }
        if (sym_type[param_indices[i]] = TYPE_STRING) and (sym_is_var_param[param_indices[i]] = 0) then
        begin
          { String value param: xi has address of source, copy to local storage }
          { x9 = source (passed address), compute x8 = dest using large offset handling }
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
          writechar(120); writechar(48 + i);  { xi }
          EmitNL;
          { Compute dest address: x8 = x29 - offset (handle large negative offset) }
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
          writechar(35);
          write(0 - sym_offset[param_indices[i]]);  { positive value }
          EmitNL;
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
          writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
          writechar(120); writechar(56);  { x8 }
          EmitNL;
          EmitBL(rt_str_copy)
        end
        else
        begin
          { Normal param: store register to stack }
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
    end
  end;

  { Set up exit label for this procedure }
  saved_exit_label := exit_label;
  proc_exit_label := NewLabel;
  exit_label := proc_exit_label;

  { Parse procedure body }
  ParseBlock;

  { Pop local symbols and restore scope }
  PopScope(scope_level);
  scope_level := saved_level;
  local_offset := saved_offset;

  { Emit exit label for exit statements }
  EmitLabel(proc_exit_label);
  exit_label := saved_exit_label;

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
  param_count, param_idx, i, j: integer;
  param_indices: array[0..7] of integer;
  is_var_group: integer;
  saved_exit_label, func_exit_label: integer;
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
        if tok_type = TOK_SEMICOLON then NextToken;
        { Check for 'var' keyword - applies to all idents in this group }
        is_var_group := 0;
        if tok_type = TOK_VAR then
        begin
          is_var_group := 1;
          NextToken
        end;
        { Parse identifier list for this parameter group }
        if tok_type <> TOK_IDENT then Error(11);
        repeat
          if tok_type = TOK_COMMA then NextToken;
          if tok_type <> TOK_IDENT then Error(11);
          { Add parameter }
          local_offset := local_offset - 8;
          param_idx := SymAdd(SYM_PARAM, TYPE_INTEGER, scope_level, local_offset);
          if is_var_group = 1 then
            sym_is_var_param[param_idx] := 1;
          if param_count < 8 then
            param_indices[param_count] := param_idx;
          param_count := param_count + 1;
          NextToken
        until tok_type <> TOK_COMMA;
        { Parse type annotation and set param types }
        if tok_type = TOK_COLON then
        begin
          NextToken;
          if tok_type = TOK_REAL_TYPE then
          begin
            sym_type[param_idx] := TYPE_REAL;
            NextToken
          end
          else if tok_type = TOK_STRING_TYPE then
          begin
            { String parameter }
            sym_type[param_idx] := TYPE_STRING;
            if is_var_group = 0 then
            begin
              { Value parameter - need 256 bytes total, already have 8, need 248 more }
              local_offset := local_offset - 248;
              sym_offset[param_idx] := local_offset;  { Update offset to start of 256-byte area }
              sym_label[param_idx] := 256  { Mark as needing copy-in }
            end;
            { var string params keep the 8-byte slot for address }
            NextToken
          end
          else if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
             (tok_type = TOK_BOOLEAN_TYPE) then
            NextToken
        end
      until tok_type <> TOK_SEMICOLON
    end;
    Expect(TOK_RPAREN)
  end;

  { Build var-param bitmap and store on function symbol }
  sym_var_param_flags[idx] := 0;
  for i := 0 to param_count - 1 do
    if sym_is_var_param[param_indices[i]] = 1 then
    begin
      if i = 0 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 1;
      if i = 1 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 2;
      if i = 2 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 4;
      if i = 3 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 8;
      if i = 4 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 16;
      if i = 5 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 32;
      if i = 6 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 64;
      if i = 7 then sym_var_param_flags[idx] := sym_var_param_flags[idx] + 128
    end;

  { Parse return type }
  Expect(TOK_COLON);
  if (tok_type = TOK_INTEGER_TYPE) or (tok_type = TOK_CHAR_TYPE) or
     (tok_type = TOK_BOOLEAN_TYPE) then
    NextToken
  else if tok_type = TOK_REAL_TYPE then
  begin
    sym_type[idx] := TYPE_REAL;
    NextToken
  end
  else if tok_type = TOK_STRING_TYPE then
  begin
    sym_type[idx] := TYPE_STRING;
    NextToken
  end
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
    { Allocate based on actual space needed (local_offset tracks it) }
    { For functions, local_offset includes return value at -16, params start at -24 }
    j := 0 - local_offset - 16;  { -16 for return value slot }
    j := ((j + 15) div 16) * 16;  { Align to 16 }
    if j > 0 then
      EmitSubSP(j);
    for i := 0 to param_count - 1 do
    begin
      if i < 8 then
      begin
        { Check if this is a string value parameter (needs copy) }
        if (sym_type[param_indices[i]] = TYPE_STRING) and (sym_is_var_param[param_indices[i]] = 0) then
        begin
          { String value param: xi has address of source, copy to local storage }
          { x9 = source (passed address), compute x8 = dest using large offset handling }
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
          writechar(120); writechar(48 + i);  { xi }
          EmitNL;
          { Compute dest address: x8 = x29 - offset (handle large negative offset) }
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
          writechar(35);
          write(0 - sym_offset[param_indices[i]]);  { positive value }
          EmitNL;
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
          writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
          writechar(120); writechar(56);  { x8 }
          EmitNL;
          EmitBL(rt_str_copy)
        end
        else
        begin
          { Normal param: store register to stack }
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
    end
  end;

  { Set up exit label for this function }
  saved_exit_label := exit_label;
  func_exit_label := NewLabel;
  exit_label := func_exit_label;

  { Parse function body }
  ParseBlock;

  { Pop local symbols and restore scope }
  PopScope(scope_level);
  scope_level := saved_level;
  local_offset := saved_offset;

  { Emit exit label for exit statements }
  EmitLabel(func_exit_label);
  exit_label := saved_exit_label;

  { Load result from local variable into x0 or d0 }
  if sym_type[idx] = TYPE_REAL then
    EmitLdurD0(-16)
  else
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

  { Main program entry }
  EmitLabel(main_lbl);
  EmitStp;
  EmitMovFP;

  { Save argc to x25, argv to x26 (before any calls clobber x0/x1) }
  { mov x25, x0 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);  { x25, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  { mov x26, x1 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(54); writechar(44); writechar(32);  { x26, }
  writechar(120); writechar(49);  { x1 }
  EmitNL;

  EmitFileOpenInit;
  EmitBL(rt_heap_init);

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

