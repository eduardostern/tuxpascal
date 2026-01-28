{ ----- Parser ----- }

Procedure ParseExpression; Forward;
Procedure ParseStatement; Forward;

Procedure Expect(t: Integer);
Begin
  If tok_type <> t Then
    Error(2);
  NextToken
End;

Function Match(t: Integer): Integer;
Begin
  If tok_type = t Then
  Begin
    NextToken;
    Match := 1
  End
  Else
    Match := 0
End;

Procedure ParseFactor;
Var
  idx, arg_count, i, lbl1, lbl2: Integer;
  var_flags, var_arg_idx: Integer;
  dim_idx, dim_count, dim_lo, dim_size: Integer;
Begin
  If tok_type = TOK_INTEGER Then
  Begin
    EmitMovX0(tok_int);
    expr_type := TYPE_INTEGER;
    NextToken
  End
  Else If tok_type = TOK_FLOAT_LITERAL Then
  Begin
    { Construct float at runtime: int_part + frac_part/1000000 }
    { Load Integer part And convert To float }
    EmitMovX0(tok_float_int);
    EmitScvtfD0X0;
    { Load fractional part And convert To float }
    EmitPushD0;
    EmitMovX0(tok_float_frac);
    EmitScvtfD0X0;
    { Divide by 1000000 }
    EmitPushD0;
    EmitMovX0(1000000);
    EmitScvtfD0X0;
    EmitPopD1;
    EmitFDiv;  { d0 = d1 / d0 = frac / 1000000 }
    { Add Integer part }
    EmitPopD1;
    EmitFAdd;  { d0 = d1 + d0 = int + frac }
    expr_type := TYPE_REAL;
    NextToken
  End
  Else If tok_type = TOK_TRUE Then
  Begin
    EmitMovX0(1);
    expr_type := TYPE_INTEGER;
    NextToken
  End
  Else If tok_type = TOK_FALSE Then
  Begin
    EmitMovX0(0);
    expr_type := TYPE_INTEGER;
    NextToken
  End
  Else If tok_type = TOK_LPAREN Then
  Begin
    NextToken;
    ParseExpression;
    Expect(TOK_RPAREN)
    { expr_type is already Set by ParseExpression }
  End
  Else If tok_type = TOK_NOT Then
  Begin
    NextToken;
    ParseFactor;
    EmitEorX0(1);
    expr_type := TYPE_INTEGER  { Not always returns Boolean/int }
  End
  Else If tok_type = TOK_LBRACKET Then
  Begin
    { Set constructor: [1, 3, 5] Or [1..5] Or ['a'..'z'] }
    NextToken;
    EmitMovX0(0);  { start With empty Set }
    If tok_type <> TOK_RBRACKET Then
    Begin
      Repeat
        If tok_type = TOK_COMMA Then NextToken;
        { Save Set so far }
        EmitPushX0;
        { Parse first value }
        ParseExpression;
        If tok_type = TOK_DOTDOT Then
        Begin
          { Range: lo..hi - create mask For all bits from lo To hi }
          EmitPushX0;  { save lo }
          NextToken;
          ParseExpression;  { hi In x0 }
          { x0 = hi, need To Set bits from lo To hi inclusive }
          { Algorithm: create mask With bits Set from lo To hi }
          { For simplicity, use a loop: For i := lo To hi Do Set |= (1 << i) }
          WriteLn('    mov x2, x0');
          EmitPopX1;  { x1 = lo }
          EmitPopX0;  { x0 = Set so far }
          { Loop: While x1 <= x2 Do x0 |= (1 << x1); x1++ }
          lbl1 := NewLabel;
          lbl2 := NewLabel;
          EmitLabel(lbl1);
          { cmp x1, x2 }
          WriteLn('    cmp x1, x2');
          { b.gt done }
          Write('    b.gt L'); WriteLn(lbl2);
          { x3 = 1 << x1 }
          WriteLn('    mov x3, #1');
          WriteLn('    lsl x3, x3, x1');
          { x0 |= x3 }
          WriteLn('    orr x0, x0, x3');
          { x1++ }
          WriteLn('    add x1, x1, #1');
          EmitBranchLabel(lbl1);
          EmitLabel(lbl2)
        End
        Else
        Begin
          { Single element: Set the bit }
          { x0 has the element value, stack has Set so far }
          { mask = 1 << x0, Then Or With saved Set }
          WriteLn('    mov x1, #1');
          WriteLn('    lsl x1, x1, x0');
          EmitPopX0;  { saved Set }
          EmitOrrX0X1  { Set | (1 << element) }
        End
      Until tok_type = TOK_RBRACKET
    End;
    NextToken;
    expr_type := TYPE_SET
  End
  Else If tok_type = TOK_NIL Then
  Begin
    EmitMovX0(0);  { Nil = 0 }
    expr_type := TYPE_POINTER;
    NextToken
  End
  Else If tok_type = TOK_STRING Then
  Begin
    { String literal In expression }
    If tok_len = 1 Then
    Begin
      { Single character - treat as Char/Integer }
      EmitMovX0(tok_str[0]);
      expr_type := TYPE_CHAR
    End
    Else
    Begin
      { Multi-Char String - allocate temp from heap And store String }
      { mov x8, x21 ; store String base To x8 }
      WriteLn('    mov x8, x21');
      { Store Length byte }
      EmitMovX0(tok_len);
      WriteLn('    strb w0, [x8]');
      { Store each character }
      For i := 0 To tok_len - 1 Do
      Begin
        EmitMovX0(tok_str[i]);
        Write('    strb w0, [x8, #'); Write(i + 1); WriteLn(']');
      End;
      { mov x0, x21 ; return String address }
      WriteLn('    mov x0, x21');
      { add x21, x21, #256 ; advance heap pointer }
      WriteLn('    add x21, x21, #256');
      expr_type := TYPE_STRING
    End;
    NextToken
  End
  Else If tok_type = TOK_AT Then
  Begin
    NextToken;
    If tok_type <> TOK_IDENT Then
      Error(6);  { expected identifier }
    idx := SymLookup;
    If idx < 0 Then
      Error(3);  { undefined identifier }
    NextToken;
    If (sym_type[idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
    Begin
      { Address Of Array element: @arr[index] Or @arr[i,j,...] }
      NextToken;  { consume '[' }
      ParseExpression;  { first index In x0 }
      { Subtract low bound for first dimension }
      dim_lo := arr_info[idx * 8];
      EmitPushX0;
      EmitMovX0(dim_lo);
      EmitPopX1;
      WriteLn('    sub x0, x1, x0');
      { Handle multi-dimensional arrays }
      dim_count := arr_dims[idx];
      If dim_count < 1 Then dim_count := 1;
      dim_idx := 1;
      While (dim_idx < dim_count) And (tok_type = TOK_COMMA) Do
      Begin
        NextToken;  { consume ',' }
        dim_size := arr_info[idx * 8 + dim_idx * 2 + 1];
        EmitPushX0;
        EmitMovX0(dim_size);
        EmitPopX1;
        WriteLn('    mul x0, x1, x0');
        EmitPushX0;
        ParseExpression;
        dim_lo := arr_info[idx * 8 + dim_idx * 2];
        EmitPushX0;
        EmitMovX0(dim_lo);
        EmitPopX1;
        WriteLn('    sub x0, x1, x0');
        EmitPopX1;
        WriteLn('    add x0, x1, x0');
        dim_idx := dim_idx + 1
      End;
      Expect(TOK_RBRACKET);
      { Multiply by element size }
      If sym_var_param_flags[idx] > 0 Then
      Begin
        lbl1 := sym_label[sym_var_param_flags[idx] - 1];
        EmitPushX0;
        EmitMovX0(lbl1);
        EmitPopX1;
        WriteLn('    mul x0, x1, x0');
      End
      Else If sym_var_param_flags[idx] = -1 Then
        WriteLn('    lsl x0, x0, #8')
      Else
        WriteLn('    lsl x0, x0, #3');
      { Get base address }
      If sym_level[idx] < scope_level Then
      Begin
        EmitFollowChain(sym_level[idx], scope_level);
        EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
      End
      Else
        EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
      { Address = base - element_offset }
      WriteLn('    sub x0, x1, x0');
    End
    Else
      EmitVarAddr(idx, scope_level);
    expr_type := TYPE_POINTER;
    ptr_base_type := sym_type[idx]
  End
  Else If tok_type = TOK_IDENT Then
  Begin
    { First check if this is a user-defined symbol (variable, function, etc.) }
    { User symbols take precedence over builtin functions }
    idx := SymLookup;
    If (idx >= 0) Or (with_rec_idx >= 0) Then
    Begin
      { Handle user-defined symbol or with-field }
      If with_rec_idx >= 0 Then
      Begin
        arg_count := FindField(with_rec_type);
        If arg_count >= 0 Then
        Begin
          { Found field - generate access code }
          NextToken;
          If sym_level[with_rec_idx] < scope_level Then
          Begin
            EmitFollowChain(sym_level[with_rec_idx], scope_level);
            EmitAddrOffset(0, 8, sym_offset[with_rec_idx] + field_offset[arg_count])
          End
          Else
            EmitAddrOffset(0, 29, sym_offset[with_rec_idx] + field_offset[arg_count]);
          If field_type[arg_count] = TYPE_REAL Then
          Begin
            WriteLn('    ldr d0, [x0]');
            expr_type := TYPE_REAL
          End
          Else
          Begin
            WriteLn('    ldr x0, [x0]');
            expr_type := field_type[arg_count]
          End
        End
        Else
        Begin
          { Field not found, use idx if valid }
          If idx < 0 Then
            Error(3)
        End
      End;
      If (with_rec_idx < 0) Or (arg_count < 0) Then
      Begin
      NextToken;
      If sym_kind[idx] = SYM_CONST Then
      Begin
        EmitMovX0(sym_const_val[idx]);
        expr_type := TYPE_INTEGER
      End
      Else If (sym_kind[idx] = SYM_VAR) Or (sym_kind[idx] = SYM_PARAM) Then
      Begin
        If (sym_type[idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
        Begin
          { Array element access: arr[index] Or arr[i,j,...] }
          NextToken;  { consume '[' }
          ParseExpression;  { first index In x0 }
          { Subtract low bound for first dimension }
          dim_lo := arr_info[idx * 8];  { lo bound for dim 0 }
          EmitPushX0;
          EmitMovX0(dim_lo);
          EmitPopX1;
          WriteLn('    sub x0, x1, x0');  { x0 = index - lo_bound }
          { Handle multi-dimensional arrays }
          dim_count := arr_dims[idx];
          If dim_count < 1 Then dim_count := 1;  { default to 1D }
          dim_idx := 1;
          While (dim_idx < dim_count) And (tok_type = TOK_COMMA) Do
          Begin
            NextToken;  { consume ',' }
            { Multiply current linear index by this dimension's size }
            dim_size := arr_info[idx * 8 + dim_idx * 2 + 1];
            EmitPushX0;  { save current linear index }
            EmitMovX0(dim_size);
            EmitPopX1;
            WriteLn('    mul x0, x1, x0');  { x0 = linear_index * dim_size }
            EmitPushX0;  { save multiplied result }
            { Parse next index }
            ParseExpression;  { new index in x0 }
            { Subtract low bound for this dimension }
            dim_lo := arr_info[idx * 8 + dim_idx * 2];
            EmitPushX0;
            EmitMovX0(dim_lo);
            EmitPopX1;
            WriteLn('    sub x0, x1, x0');  { x0 = index - lo_bound }
            { Add to previous linear index }
            EmitPopX1;  { restore multiplied linear index }
            WriteLn('    add x0, x1, x0');  { x0 = linear_index + new_index }
            dim_idx := dim_idx + 1
          End;
          Expect(TOK_RBRACKET);
          { Multiply linear index by element size }
          If sym_var_param_flags[idx] > 0 Then
          Begin
            { Array Of records - multiply by Record size }
            lbl1 := sym_label[sym_var_param_flags[idx] - 1];  { Record size }
            EmitPushX0;
            EmitMovX0(lbl1);
            EmitPopX1;
            WriteLn('    mul x0, x1, x0');
          End
          Else If sym_var_param_flags[idx] = -1 Then
          Begin
            { Array Of strings - multiply by 256 using lsl #8 }
            WriteLn('    lsl x0, x0, #8');
          End
          Else
          Begin
            { Basic Type - multiply by 8 using lsl #3 }
            WriteLn('    lsl x0, x0, #3');
          End;
          { Get base address: frame + offset (offset is negative) }
          If sym_level[idx] < scope_level Then
          Begin
            EmitFollowChain(sym_level[idx], scope_level);
            EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
          End
          Else
            EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
          { Compute element address: x1 - x0 }
          WriteLn('    sub x1, x1, x0');
          { Check For field access on Array Of records }
          If (sym_var_param_flags[idx] > 0) And (tok_type = TOK_DOT) Then
          Begin
            NextToken;  { consume '.' }
            arg_count := FindField(sym_var_param_flags[idx] - 1);
            If arg_count < 0 Then
              Error(11);
            NextToken;  { consume field name }
            { Add field offset: x1 = x1 + offset }
            If field_offset[arg_count] > 0 Then
              EmitAddrOffset(1, 1, field_offset[arg_count]);
            { Load field value }
            If field_type[arg_count] = TYPE_REAL Then
            Begin
              WriteLn('    ldr d0, [x1]');
              expr_type := TYPE_REAL
            End
            Else
            Begin
              WriteLn('    ldr x0, [x1]');
              expr_type := field_type[arg_count]
            End
          End
          Else If sym_var_param_flags[idx] = -1 Then
          Begin
            { Array Of strings - return address Of String element }
            WriteLn('    mov x0, x1');
            expr_type := TYPE_STRING
          End
          Else
          Begin
            { Load element value }
            WriteLn('    ldr x0, [x1]');
          End
        End
        Else If (sym_type[idx] = TYPE_RECORD) And (tok_type = TOK_DOT) Then
        Begin
          { Record field access: rec.field Or rec.field.subfield... }
          NextToken;  { consume '.' }
          If tok_type <> TOK_IDENT Then
            Error(11);
          { Find the field In the Record Type }
          arg_count := FindField(sym_const_val[idx]);  { reuse arg_count For field_idx }
          If arg_count < 0 Then
            Error(15);  { undefined field }
          { Compute address: base + field_offset }
          { Base is at [x29 + sym_offset] For local, field at positive offset from there }
          If sym_level[idx] < scope_level Then
          Begin
            EmitFollowChain(sym_level[idx], scope_level);
            EmitAddrOffset(0, 8, sym_offset[idx] + field_offset[arg_count])
          End
          Else
            EmitAddrOffset(0, 29, sym_offset[idx] + field_offset[arg_count]);
          NextToken;
          { Handle nested Record fields: x0 has address, check For more dots }
          While (field_type[arg_count] = TYPE_RECORD) And (tok_type = TOK_DOT) Do
          Begin
            NextToken;  { consume '.' }
            If tok_type <> TOK_IDENT Then
              Error(11);
            { Find sub-field In the nested Record Type }
            lbl1 := arg_count;  { save current field index }
            arg_count := FindField(field_rec_type[lbl1]);
            If arg_count < 0 Then
              Error(15);
            { Add sub-field offset To x0 }
            If field_offset[arg_count] > 0 Then
              EmitAddrOffset(0, 0, field_offset[arg_count]);
            NextToken
          End;
          { Load from final computed address }
          If field_type[arg_count] = TYPE_REAL Then
          Begin
            WriteLn('    ldr d0, [x0]');
            expr_type := TYPE_REAL
          End
          Else If field_type[arg_count] = TYPE_RECORD Then
          Begin
            { Accessing a nested Record as a whole - keep address In x0 }
            expr_type := TYPE_RECORD
          End
          Else
          Begin
            WriteLn('    ldr x0, [x0]');
            expr_type := field_type[arg_count]
          End
        End
        Else
        Begin
          { Load variable value - check For Var param And Type }
          If sym_type[idx] = TYPE_REAL Then
          Begin
            If sym_level[idx] < scope_level Then
              EmitLdurD0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitLdurD0(sym_offset[idx]);
            { If Var param, d0 contains address - dereference it }
            If sym_is_var_param[idx] = 1 Then
            Begin
              { fmov x0, d0; ldr d0, [x0] }
              EmitFmovX0D0;
              WriteLn('    ldr d0, [x0]');
            End;
            expr_type := TYPE_REAL
          End
          Else If sym_type[idx] = TYPE_STRING Then
          Begin
            { String variable - return address }
            EmitVarAddr(idx, scope_level);
            expr_type := TYPE_STRING;
            { Check For String indexing s[i] }
            If tok_type = TOK_LBRACKET Then
            Begin
              NextToken;  { consume '[' }
              EmitPushX0;  { save String base address }
              ParseExpression;  { index In x0 }
              Expect(TOK_RBRACKET);
              { x0 = index, stack has base address }
              EmitPopX1;  { x1 = base address }
              { Add index To base: x0 = x1 + x0 }
              WriteLn('    add x0, x1, x0');
              { Load byte at [x0]: ldrb w0, [x0] }
              WriteLn('    ldrb w0, [x0]');
              expr_type := TYPE_CHAR
            End
          End
          Else If sym_type[idx] = TYPE_POINTER Then
          Begin
            { Pointer variable }
            If sym_level[idx] < scope_level Then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitLdurX0(sym_offset[idx]);
            { If Var param, x0 contains address - dereference it }
            If sym_is_var_param[idx] = 1 Then
              WriteLn('    ldr x0, [x0]');
            { Check For dereference operator ^ }
            If tok_type = TOK_CARET Then
            Begin
              NextToken;
              { Check For Array dereference With index: pa^[i] }
              If (ptr_ultimate_type[idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
              Begin
                NextToken;
                ParseExpression;
                Expect(TOK_RBRACKET);
                { x0 = index, stack has base address }
                EmitPushX0;
                If sym_level[idx] < scope_level Then
                  EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
                Else
                  EmitLdurX0(sym_offset[idx]);
                If sym_is_var_param[idx] = 1 Then
                  WriteLn('    ldr x0, [x0]');
                EmitPushX0;
                EmitPopX1;
                EmitPopX0;
                { Subtract low bound }
                arg_count := sym_label[idx];  { ptr_arr index }
                EmitPushX0;
                EmitMovX0(ptr_arr_lo[arg_count]);
                EmitPopX1;
                { x0 = x1 - x0 = index - low_bound }
                WriteLn('    sub x0, x1, x0');
                { Multiply by 8 using lsl #3 }
                WriteLn('    lsl x0, x0, #3');
                { Add To base: x1 = base + offset }
                EmitPopX1;  { x1 = base address }
                WriteLn('    add x0, x1, x0');
                { Load element value }
                If ptr_arr_elem[arg_count] = TYPE_REAL Then
                Begin
                  WriteLn('    ldr d0, [x0]');
                  expr_type := TYPE_REAL
                End
                Else
                Begin
                  WriteLn('    ldr x0, [x0]');
                  expr_type := ptr_arr_elem[arg_count]
                End
              End
              Else If ptr_ultimate_type[idx] = TYPE_REAL Then
              Begin
                expr_type := TYPE_REAL
              End
              Else
              Begin
                expr_type := ptr_ultimate_type[idx]
              End
            End
          End
          Else If sym_kind[idx] = SYM_FUNCTION Then
          Begin
            { Function call - pass args In x0-x7 }
            arg_count := 0;
            var_flags := sym_var_param_flags[idx];
            If tok_type = TOK_LPAREN Then
            Begin
              NextToken;
              If tok_type <> TOK_RPAREN Then
              Begin
                { Evaluate all args And push To stack }
                Repeat
                  If tok_type = TOK_COMMA Then NextToken;
                  { Check If this is a Var parameter }
                  If IsVarParam(var_flags, arg_count) = 1 Then
                  Begin
                    { Var param - pass address Of variable }
                    If tok_type <> TOK_IDENT Then
                      Error(6);  { Var param requires variable }
                    var_arg_idx := SymLookup;
                    If var_arg_idx < 0 Then
                      Error(3);
                    NextToken;
                    { Check For Array element - pass address Of element }
                    If (sym_type[var_arg_idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
                    Begin
                      NextToken;  { consume '[' }
                      ParseExpression;  { first index In x0 }
                      { Subtract low bound for first dimension }
                      dim_lo := arr_info[var_arg_idx * 8];
                      EmitPushX0;
                      EmitMovX0(dim_lo);
                      EmitPopX1;
                      WriteLn('    sub x0, x1, x0');
                      { Handle multi-dimensional arrays }
                      dim_count := arr_dims[var_arg_idx];
                      If dim_count < 1 Then dim_count := 1;
                      dim_idx := 1;
                      While (dim_idx < dim_count) And (tok_type = TOK_COMMA) Do
                      Begin
                        NextToken;
                        dim_size := arr_info[var_arg_idx * 8 + dim_idx * 2 + 1];
                        EmitPushX0;
                        EmitMovX0(dim_size);
                        EmitPopX1;
                        WriteLn('    mul x0, x1, x0');
                        EmitPushX0;
                        ParseExpression;
                        dim_lo := arr_info[var_arg_idx * 8 + dim_idx * 2];
                        EmitPushX0;
                        EmitMovX0(dim_lo);
                        EmitPopX1;
                        WriteLn('    sub x0, x1, x0');
                        EmitPopX1;
                        WriteLn('    add x0, x1, x0');
                        dim_idx := dim_idx + 1
                      End;
                      Expect(TOK_RBRACKET);
                      { Multiply by element size }
                      If sym_var_param_flags[var_arg_idx] > 0 Then
                      Begin
                        lbl1 := sym_label[sym_var_param_flags[var_arg_idx] - 1];
                        EmitPushX0;
                        EmitMovX0(lbl1);
                        EmitPopX1;
                        WriteLn('    mul x0, x1, x0');
                      End
                      Else If sym_var_param_flags[var_arg_idx] = -1 Then
                        WriteLn('    lsl x0, x0, #8')
                      Else
                        WriteLn('    lsl x0, x0, #3');
                      { Get base address And subtract element offset }
                      If sym_level[var_arg_idx] < scope_level Then
                      Begin
                        EmitFollowChain(sym_level[var_arg_idx], scope_level);
                        EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                      End
                      Else
                        EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                      { Address = base - element_offset }
                      WriteLn('    sub x0, x1, x0');
                    End
                    Else
                      { Simple variable - emit address }
                      EmitVarAddr(var_arg_idx, scope_level)
                  End
                  Else
                  Begin
                    { Value param - evaluate expression }
                    ParseExpression
                  End;
                  EmitPushX0;
                  arg_count := arg_count + 1
                Until tok_type <> TOK_COMMA
              End;
              Expect(TOK_RPAREN)
            End;
            { Pop args from stack into registers In reverse order }
            For i := arg_count - 1 DownTo 0 Do
            Begin
              Write('    ldr x'); Write(i); WriteLn(', [sp], #16');
            End;
            { Set up static link For callee }
            EmitStaticLink(sym_level[idx], scope_level);
            { Check If calling imported Unit Procedure or external C function }
            If sym_unit_idx[idx] >= 0 Then
              EmitBLUnitProc(sym_unit_idx[idx], idx)
            Else If sym_is_external[idx] = 1 Then
              EmitBLExternal(idx)
            Else
              EmitBL(sym_label[idx]);
            expr_type := sym_type[idx]  { Function return Type }
          End
          Else
          Begin
            { Regular Integer/Char/Boolean variable }
            If sym_level[idx] < scope_level Then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitLdurX0(sym_offset[idx]);
            { If Var param, x0 contains address - dereference it }
            If sym_is_var_param[idx] = 1 Then
              WriteLn('    ldr x0, [x0]');
            expr_type := sym_type[idx]
          End
        End
      End
      Else If sym_kind[idx] = SYM_FUNCTION Then
      Begin
        { Function call - pass args In x0-x7 }
        arg_count := 0;
        var_flags := sym_var_param_flags[idx];
        If tok_type = TOK_LPAREN Then
        Begin
          NextToken;
          If tok_type <> TOK_RPAREN Then
          Begin
            { Evaluate all args And push To stack }
            Repeat
              If tok_type = TOK_COMMA Then NextToken;
              { Check If this is a Var parameter }
              If IsVarParam(var_flags, arg_count) = 1 Then
              Begin
                { Var param - pass address Of variable }
                If tok_type <> TOK_IDENT Then
                  Error(6);  { Var param requires variable }
                var_arg_idx := SymLookup;
                If var_arg_idx < 0 Then
                  Error(3);
                NextToken;
                { Check For Array element - pass address Of element }
                If (sym_type[var_arg_idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
                Begin
                  NextToken;  { consume '[' }
                  ParseExpression;  { first index In x0 }
                  { Subtract low bound for first dimension }
                  dim_lo := arr_info[var_arg_idx * 8];
                  EmitPushX0;
                  EmitMovX0(dim_lo);
                  EmitPopX1;
                  WriteLn('    sub x0, x1, x0');
                  { Handle multi-dimensional arrays }
                  dim_count := arr_dims[var_arg_idx];
                  If dim_count < 1 Then dim_count := 1;
                  dim_idx := 1;
                  While (dim_idx < dim_count) And (tok_type = TOK_COMMA) Do
                  Begin
                    NextToken;
                    dim_size := arr_info[var_arg_idx * 8 + dim_idx * 2 + 1];
                    EmitPushX0;
                    EmitMovX0(dim_size);
                    EmitPopX1;
                    WriteLn('    mul x0, x1, x0');
                    EmitPushX0;
                    ParseExpression;
                    dim_lo := arr_info[var_arg_idx * 8 + dim_idx * 2];
                    EmitPushX0;
                    EmitMovX0(dim_lo);
                    EmitPopX1;
                    WriteLn('    sub x0, x1, x0');
                    EmitPopX1;
                    WriteLn('    add x0, x1, x0');
                    dim_idx := dim_idx + 1
                  End;
                  Expect(TOK_RBRACKET);
                  { Multiply by element size }
                  If sym_var_param_flags[var_arg_idx] > 0 Then
                  Begin
                    lbl1 := sym_label[sym_var_param_flags[var_arg_idx] - 1];
                    EmitPushX0;
                    EmitMovX0(lbl1);
                    EmitPopX1;
                    WriteLn('    mul x0, x1, x0');
                  End
                  Else If sym_var_param_flags[var_arg_idx] = -1 Then
                    WriteLn('    lsl x0, x0, #8')
                  Else
                    WriteLn('    lsl x0, x0, #3');
                  { Get base address And subtract element offset }
                  If sym_level[var_arg_idx] < scope_level Then
                  Begin
                    EmitFollowChain(sym_level[var_arg_idx], scope_level);
                    EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                  End
                  Else
                    EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                  { Address = base - element_offset }
                  WriteLn('    sub x0, x1, x0');
                End
                Else
                  { Simple variable - emit address }
                  EmitVarAddr(var_arg_idx, scope_level)
              End
              Else
              Begin
                { Value param - evaluate expression }
                ParseExpression
              End;
              EmitPushX0;
              arg_count := arg_count + 1
            Until tok_type <> TOK_COMMA
          End;
          Expect(TOK_RPAREN)
        End;
        { Pop args from stack into registers In reverse order }
        For i := arg_count - 1 DownTo 0 Do
        Begin
          Write('    ldr x'); Write(i); WriteLn(', [sp], #16');
        End;
        { Set up static link For callee }
        EmitStaticLink(sym_level[idx], scope_level);
        { Check If calling imported Unit Procedure or external C function }
        If sym_unit_idx[idx] >= 0 Then
          EmitBLUnitProc(sym_unit_idx[idx], idx)
        Else If sym_is_external[idx] = 1 Then
          EmitBLExternal(idx)
        Else
          EmitBL(sym_label[idx]);
        expr_type := sym_type[idx]  { Function return Type }
      End
      Else
        Error(4)
      End  { End Of If (with_rec_idx < 0) Or (arg_count < 0) }
    End  { End Of user-defined symbol handling }
    Else
    Begin
    { Check For built-In functions: ReadChar, keypressed, Ord, Chr }
    { ReadChar = 114,101,97,100,99,104,97,114 }
    If TokIs8(114, 101, 97, 100, 99, 104, 97, 114) = 1 Then
    Begin
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        Expect(TOK_RPAREN)
      End;
      EmitBL(rt_readchar);
      expr_type := TYPE_INTEGER
    End
    { getinputfd = 103,101,116,105,110,112,117,116,102,100 (10 chars) }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 103) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 110) And
            (ToLower(tok_str[5]) = 112) And (ToLower(tok_str[6]) = 117) And (ToLower(tok_str[7]) = 116) And
            (ToLower(tok_str[8]) = 102) And (ToLower(tok_str[9]) = 100) Then
    Begin
      { getinputfd - returns current input file descriptor (x19) }
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        Expect(TOK_RPAREN)
      End;
      { mov x0, x19 }
      WriteLn('    mov x0, x19');
      expr_type := TYPE_INTEGER
    End
    { getoutputfd = 103,101,116,111,117,116,112,117,116,102,100 (11 chars) }
    Else If (tok_len = 11) And (ToLower(tok_str[0]) = 103) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 111) And (ToLower(tok_str[4]) = 117) And
            (ToLower(tok_str[5]) = 116) And (ToLower(tok_str[6]) = 112) And (ToLower(tok_str[7]) = 117) And
            (ToLower(tok_str[8]) = 116) And (ToLower(tok_str[9]) = 102) And (ToLower(tok_str[10]) = 100) Then
    Begin
      { getoutputfd - returns current output file descriptor (x20) }
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        Expect(TOK_RPAREN)
      End;
      { mov x0, x20 }
      WriteLn('    mov x0, x20');
      expr_type := TYPE_INTEGER
    End
    { readfd = 114,101,97,100,102,100 (6 chars) }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 97) And (ToLower(tok_str[3]) = 100) And (ToLower(tok_str[4]) = 102) And
            (ToLower(tok_str[5]) = 100) Then
    Begin
      { readfd(fd) - Read one Char from fd, returns Char Or -1 For EOF }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = fd, save it Then Do Read syscall }
      { sub sp, sp, #16; str x0, [sp] - allocate buffer And save fd }
      WriteLn('    sub sp, sp, #16');
      { mov x1, sp - buffer on stack }
      WriteLn('    mov x1, sp');
      { mov x2, #1 - Read 1 byte }
      WriteLn('    mov x2, #1');
      { Read syscall: x16 = 0x2000003 }
      WriteLn('    movz x16, #3');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Check If Read returned >= 1 }
      { cmp x0, #1; b.ge got_char; mov x0, #-1; b done; got_char: ldrb w0, [sp]; done: }
      WriteLn('    cmp x0, #1');
      Write('    b.ge L'); WriteLn(label_count);
      EmitMovX0(-1);
      EmitBranchLabel(label_count + 1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      { ldrb w0, [sp] }
      WriteLn('    ldrb w0, [sp]');
      EmitLabel(label_count);
      label_count := label_count + 1;
      { add sp, sp, #16 - restore stack }
      WriteLn('    add sp, sp, #16');
      expr_type := TYPE_INTEGER
    End
    { openfile = 111,112,101,110,102,105,108,101 (8 chars) }
    Else If TokIs8(111, 112, 101, 110, 102, 105, 108, 101) = 1 Then
    Begin
      { openfile(filename) - open file For reading, returns fd Or -1 on error }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = Pascal String address, need To skip Length byte For C String }
      { add x0, x0, #1 }
      WriteLn('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_RDONLY(0), x2=mode(0) }
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #0');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      WriteLn('    movz x16, #5');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { On macOS, carry flag Set on error - convert errno To -1 }
      { b.cc skip; mov x0, #-1; skip: }
      Write('    b.cc L'); WriteLn(label_count);
      EmitMovX0(-1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_INTEGER
    End
    { createfile = 99,114,101,97,116,101,102,105,108,101 (10 chars) }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 101) And (ToLower(tok_str[3]) = 97) And (ToLower(tok_str[4]) = 116) And
            (ToLower(tok_str[5]) = 101) And (ToLower(tok_str[6]) = 102) And (ToLower(tok_str[7]) = 105) And
            (ToLower(tok_str[8]) = 108) And (ToLower(tok_str[9]) = 101) Then
    Begin
      { createfile(filename) - create/open file For writing, returns fd Or -1 }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = Pascal String address, need To skip Length byte For C String }
      { add x0, x0, #1 }
      WriteLn('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_WRONLY|O_CREAT|O_TRUNC(1537), x2=mode(420=0644) }
      { mov x1, #1537 }
      WriteLn('    mov x1, #1537');
      { mov x2, #420 }
      WriteLn('    mov x2, #420');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      WriteLn('    movz x16, #5');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { On macOS, carry flag Set on error - convert errno To -1 }
      Write('    b.cc L'); WriteLn(label_count);
      EmitMovX0(-1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_INTEGER
    End
    { keypressed = 107,101,121,112,114,101,115,115,101,100 (10 chars) }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 107) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 121) And (ToLower(tok_str[3]) = 112) And (ToLower(tok_str[4]) = 114) And
            (ToLower(tok_str[5]) = 101) And (ToLower(tok_str[6]) = 115) And (ToLower(tok_str[7]) = 115) And
            (ToLower(tok_str[8]) = 101) And (ToLower(tok_str[9]) = 100) Then
    Begin
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        Expect(TOK_RPAREN)
      End;
      EmitBL(rt_keypressed);
      expr_type := TYPE_BOOLEAN
    End
    { Ord = 111,114,100 }
    Else If TokIs8(111, 114, 100, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Ord() is identity For integers/chars }
      expr_type := TYPE_INTEGER
    End
    { Hi = 104,105 - high byte of word (only if not a variable) }
    Else If (tok_len = 2) And (ToLower(tok_str[0]) = 104) And (ToLower(tok_str[1]) = 105)
            And (SymLookup < 0) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Hi(x) = (x >> 8) AND 255 }
      WriteLn('    lsr x0, x0, #8');
      WriteLn('    and x0, x0, #255');
      expr_type := TYPE_INTEGER
    End
    { Lo = 108,111 - low byte of word (only if not a variable) }
    Else If (tok_len = 2) And (ToLower(tok_str[0]) = 108) And (ToLower(tok_str[1]) = 111)
            And (SymLookup < 0) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Lo(x) = x AND 255 }
      WriteLn('    and x0, x0, #255');
      expr_type := TYPE_INTEGER
    End
    { Swap = 115,119,97,112 - swap bytes of word }
    Else If TokIs8(115, 119, 97, 112, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Swap(x) = ((x AND 255) << 8) OR ((x >> 8) AND 255) }
      WriteLn('    and x1, x0, #255');
      WriteLn('    lsl x1, x1, #8');
      WriteLn('    lsr x0, x0, #8');
      WriteLn('    and x0, x0, #255');
      WriteLn('    orr x0, x0, x1');
      expr_type := TYPE_INTEGER
    End
    { Assigned = 97,115,115,105,103,110,101,100 - test pointer not nil }
    Else If TokIs8(97, 115, 115, 105, 103, 110, 101, 100) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Assigned(p) = p <> nil, return 1 if not nil, 0 if nil }
      WriteLn('    cmp x0, #0');
      WriteLn('    cset x0, ne');
      expr_type := TYPE_INTEGER
    End
    { Chr = 99,104,114 }
    Else If TokIs8(99, 104, 114, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Chr() is identity For integers/chars }
      expr_type := TYPE_INTEGER
    End
    { abs = 97,98,115 }
    Else If TokIs8(97, 98, 115, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { If x < 0, negate it }
      { cmp x0, #0; b.ge skip; neg x0, x0; skip: }
      WriteLn('    cmp x0, #0');
      Write('    b.ge L'); WriteLn(label_count);
      WriteLn('    neg x0, x0');
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_INTEGER
    End
    { odd = 111,100,100 }
    Else If TokIs8(111, 100, 100, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x And 1 }
      WriteLn('    And x0, x0, #1');
      expr_type := TYPE_BOOLEAN
    End
    { sqr = 115,113,114 }
    Else If TokIs8(115, 113, 114, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x * x }
      WriteLn('    mul x0, x0, x0');
      expr_type := TYPE_INTEGER
    End
    { sqrt = 115,113,114,116 }
    Else If TokIs8(115, 113, 114, 116, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Convert To Real If Integer }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { fsqrt d0, d0 }
      WriteLn('    fsqrt d0, d0');
      expr_type := TYPE_REAL
    End
    { round = 114,111,117,110,100 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 117) And (ToLower(tok_str[3]) = 110) And (ToLower(tok_str[4]) = 100) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Convert To Real If Integer }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { fcvtas x0, d0 - round To nearest With ties To away }
      WriteLn('    fcvtas x0, d0');
      expr_type := TYPE_INTEGER
    End
    { trunc = 116,114,117,110,99 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 117) And (ToLower(tok_str[3]) = 110) And (ToLower(tok_str[4]) = 99) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Convert To Real If Integer }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { fcvtzs x0, d0 - truncate toward zero }
      WriteLn('    fcvtzs x0, d0');
      expr_type := TYPE_INTEGER
    End
    { sin = 115,105,110 }
    Else If TokIs8(115, 105, 110, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_sin);
      expr_type := TYPE_REAL
    End
    { cos = 99,111,115 }
    Else If TokIs8(99, 111, 115, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_cos);
      expr_type := TYPE_REAL
    End
    { tan = 116,97,110 }
    Else If TokIs8(116, 97, 110, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_tan);
      expr_type := TYPE_REAL
    End
    { exp = 101,120,112 }
    Else If TokIs8(101, 120, 112, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_exp);
      expr_type := TYPE_REAL
    End
    { ln = 108,110 }
    Else If (tok_len = 2) And (ToLower(tok_str[0]) = 108) And (ToLower(tok_str[1]) = 110) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_ln);
      expr_type := TYPE_REAL
    End
    { random = 114,97,110,100,111,109 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 97) And
            (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 100) And (ToLower(tok_str[4]) = 111) And
            (ToLower(tok_str[5]) = 109) Then
    Begin
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        { random(n) - returns 0..n-1 }
        NextToken;
        If tok_type <> TOK_RPAREN Then
        Begin
          ParseExpression;
          EmitPushX0;
          EmitBL(rt_random);
          EmitPopX1;
          { x0 Mod x1 using udiv And msub }
          WriteLn('    udiv x2, x0, x1');
          WriteLn('    msub x0, x2, x1, x0');
        End
        Else
        Begin
          { random() With no arg - return Real 0.0..1.0 }
          EmitBL(rt_random);
          { Convert To Real And divide by 2^64 }
          EmitScvtfD0X0;
          { Load 2^63 as divisor (since signed, use 2^63) }
          WriteLn('    movz x1, #0x8000, lsl #48');
          WriteLn('    scvtf d1, x1');
          WriteLn('    fdiv d0, d0, d1');
          { Make positive by using fabs }
          WriteLn('    fabs d0, d0');
          expr_type := TYPE_REAL
        End;
        Expect(TOK_RPAREN)
      End
      Else
      Begin
        { random With no parens - return Real 0.0..1.0 }
        EmitBL(rt_random);
        EmitScvtfD0X0;
        WriteLn('    movz x1, #0x8000, lsl #48');
        WriteLn('    scvtf d1, x1');
        WriteLn('    fdiv d0, d0, d1');
        WriteLn('    fabs d0, d0');
        expr_type := TYPE_REAL
      End;
      If expr_type <> TYPE_REAL Then
        expr_type := TYPE_INTEGER
    End
    { pi = 112,105 - returns 3.14159265358979 }
    Else If (tok_len = 2) And (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 105) Then
    Begin
      NextToken;
      { Load pi = 3.14159265358979323846 into d0 }
      { IEEE 754 double: 0x400921FB54442D18 }
      WriteLn('    movz x0, #0x2D18');
      WriteLn('    movk x0, #0x5444, lsl #16');
      WriteLn('    movk x0, #0x21FB, lsl #32');
      WriteLn('    movk x0, #0x4009, lsl #48');
      EmitFmovD0X0;
      expr_type := TYPE_REAL
    End
    { frac = 102,114,97,99 - fractional part }
    Else If TokIs8(102, 114, 97, 99, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { frac(x) = x - trunc(x) }
      { Save x To stack }
      EmitPushD0;
      { Truncate To Integer }
      EmitFcvtzsX0D0;
      { Convert back To float }
      EmitScvtfD0X0;
      { Pop original, now d1 = trunc(x) }
      WriteLn('    fmov d1, d0');
      EmitPopD0;
      { d0 = x - trunc(x) }
      WriteLn('    fsub d0, d0, d1');
      expr_type := TYPE_REAL
    End
    { int = 105,110,116 - Integer part as Real (different from trunc) }
    Else If TokIs8(105, 110, 116, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { int(x) = trunc(x) as float }
      { frintz d0, d0 - round toward zero (truncate) }
      WriteLn('    frintz d0, d0');
      expr_type := TYPE_REAL
    End
    { arctan = 97,114,99,116,97,110 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 99) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 97) And
            (ToLower(tok_str[5]) = 110) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_arctan);
      expr_type := TYPE_REAL
    End
    { arcsin = 97,114,99,115,105,110 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 99) And (ToLower(tok_str[3]) = 115) And (ToLower(tok_str[4]) = 105) And
            (ToLower(tok_str[5]) = 110) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_arcsin);
      expr_type := TYPE_REAL
    End
    { arccos = 97,114,99,99,111,115 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 99) And (ToLower(tok_str[3]) = 99) And (ToLower(tok_str[4]) = 111) And
            (ToLower(tok_str[5]) = 115) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitBL(rt_arccos);
      expr_type := TYPE_REAL
    End
    { log10 = 108,111,103,49,48 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 108) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 103) And (ToLower(tok_str[3]) = 49) And (ToLower(tok_str[4]) = 48) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { log10(x) = ln(x) / ln(10) }
      EmitBL(rt_ln);
      EmitPushD0;
      { Load ln(10) = 2.302585... into d0 }
      WriteLn('    movz x0, #0x634A');
      WriteLn('    movk x0, #0x9BFE, lsl #16');
      WriteLn('    movk x0, #0x66D3, lsl #32');
      WriteLn('    movk x0, #0x4002, lsl #48');
      EmitFmovD0X0;
      EmitPopD1;
      { d0 = ln(x) / ln(10) }
      WriteLn('    fdiv d0, d1, d0');
      expr_type := TYPE_REAL
    End
    { log2 = 108,111,103,50 }
    Else If TokIs8(108, 111, 103, 50, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { log2(x) = ln(x) / ln(2) }
      EmitBL(rt_ln);
      EmitPushD0;
      { Load ln(2) = 0.693147... into d0 }
      WriteLn('    movz x0, #0x39EF');
      WriteLn('    movk x0, #0xFEFA, lsl #16');
      WriteLn('    movk x0, #0x2E42, lsl #32');
      WriteLn('    movk x0, #0x3FE6, lsl #48');
      EmitFmovD0X0;
      EmitPopD1;
      { d0 = ln(x) / ln(2) }
      WriteLn('    fdiv d0, d1, d0');
      expr_type := TYPE_REAL
    End
    { power = 112,111,119,101,114 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 119) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 114) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { base }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      EmitPushD0;
      Expect(TOK_COMMA);
      ParseExpression;  { exponent }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;
      { d0 = exp, stack has base }
      { power(b, e) = exp(e * ln(b)) }
      { Move exp To d1, get base To d0, save exp on stack }
      EmitPushD0;       { stack = [exp, base] }
      EmitPopD1;        { d1 = exp, stack = [base] }
      EmitPopD0;        { d0 = base, stack = [] }
      { Push d1 (exp) To stack - str d1, [sp, #-16]! }
      WriteLn('    str d1, [sp, #-16]!');
      { d0 = base, stack = [exp] }
      EmitBL(rt_ln);    { d0 = ln(base), stack = [exp] }
      EmitPopD1;        { d1 = exp, stack = [] }
      { fmul d0, d0, d1 - d0 = ln(base) * exp }
      WriteLn('    fmul d0, d0, d1');
      EmitBL(rt_exp);   { d0 = exp(ln(base) * exp) = base^exp }
      Expect(TOK_RPAREN);
      expr_type := TYPE_REAL
    End
    { succ = 115,117,99,99 }
    Else If TokIs8(115, 117, 99, 99, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x + 1 }
      WriteLn('    add x0, x0, #1');
      expr_type := TYPE_INTEGER
    End
    { pred = 112,114,101,100 }
    Else If TokIs8(112, 114, 101, 100, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x - 1 }
      WriteLn('    sub x0, x0, #1');
      expr_type := TYPE_INTEGER
    End
    { SizeOf = 115,105,122,101,111,102 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 105) And
            (ToLower(tok_str[2]) = 122) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 111) And
            (ToLower(tok_str[5]) = 102) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      { Check For Type name Or variable }
      If tok_type = TOK_INTEGER_TYPE Then
      Begin
        EmitMovX0(8);
        NextToken
      End
      Else If tok_type = TOK_CHAR_TYPE Then
      Begin
        EmitMovX0(1);
        NextToken
      End
      Else If tok_type = TOK_BOOLEAN_TYPE Then
      Begin
        EmitMovX0(1);
        NextToken
      End
      Else If tok_type = TOK_REAL_TYPE Then
      Begin
        EmitMovX0(8);
        NextToken
      End
      Else If tok_type = TOK_STRING_TYPE Then
      Begin
        EmitMovX0(256);
        NextToken
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        { Look up identifier - could be variable Or Type name }
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_kind[idx] = SYM_TYPEDEF Then
        Begin
          { Type definition - get size from sym_label (Record size) }
          EmitMovX0(sym_label[idx])
        End
        Else If sym_type[idx] = TYPE_INTEGER Then
          EmitMovX0(8)
        Else If sym_type[idx] = TYPE_CHAR Then
          EmitMovX0(1)  { Logical size, Not storage size }
        Else If sym_type[idx] = TYPE_BOOLEAN Then
          EmitMovX0(1)  { Logical size, Not storage size }
        Else If sym_type[idx] = TYPE_REAL Then
          EmitMovX0(8)
        Else If sym_type[idx] = TYPE_STRING Then
          EmitMovX0(256)
        Else If sym_type[idx] = TYPE_POINTER Then
          EmitMovX0(8)
        Else If sym_type[idx] = TYPE_ARRAY Then
        Begin
          { Array: sym_label already contains total size In bytes }
          EmitMovX0(sym_label[idx])
        End
        Else If sym_type[idx] = TYPE_RECORD Then
        Begin
          { Record: sym_const_val has typedef index, get size from typedef's sym_label }
          EmitMovX0(sym_label[sym_const_val[idx]])
        End
        Else
          EmitMovX0(8);  { Default To 8 bytes }
        NextToken
      End
      Else
        Error(9);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    End
    { upcase = 117,112,99,97,115,101 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 117) And (ToLower(tok_str[1]) = 112) And
            (ToLower(tok_str[2]) = 99) And (ToLower(tok_str[3]) = 97) And (ToLower(tok_str[4]) = 115) And
            (ToLower(tok_str[5]) = 101) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { If Char is 'a'-'z' (97-122), subtract 32 }
      { cmp x0, #97; b.lt skip; cmp x0, #122; b.gt skip; sub x0, x0, #32; skip: }
      WriteLn('    cmp x0, #97');
      Write('    b.lt L'); WriteLn(label_count);
      WriteLn('    cmp x0, #122');
      Write('    b.gt L'); WriteLn(label_count);
      WriteLn('    sub x0, x0, #32');
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_CHAR
    End
    { lowercase = 108,111,119,101,114,99,97,115,101 }
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 108) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 119) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 114) And
            (ToLower(tok_str[5]) = 99) And (ToLower(tok_str[6]) = 97) And (ToLower(tok_str[7]) = 115) And
            (ToLower(tok_str[8]) = 101) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { If Char is 'A'-'Z' (65-90), add 32 }
      { cmp x0, #65; b.lt skip; cmp x0, #90; b.gt skip; add x0, x0, #32; skip: }
      WriteLn('    cmp x0, #65');
      Write('    b.lt L'); WriteLn(label_count);
      WriteLn('    cmp x0, #90');
      Write('    b.gt L'); WriteLn(label_count);
      WriteLn('    add x0, x0, #32');
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_CHAR
    End
    { Length = 108,101,110,103,116,104 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 108) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 103) And (ToLower(tok_str[4]) = 116) And
            (ToLower(tok_str[5]) = 104) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type = TOK_STRING Then
      Begin
        { String literal - emit constant Length }
        EmitMovX0(tok_len);
        NextToken
      End
      Else If tok_type = TOK_IDENT Then
      Begin
        { String variable Or Array Of String element }
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] = TYPE_STRING Then
        Begin
          NextToken;
          { Load String address }
          EmitVarAddr(idx, scope_level);
          { Load Length byte from [x0] }
          WriteLn('    ldrb w0, [x0]');
        End
        Else If (sym_type[idx] = TYPE_ARRAY) And (sym_var_param_flags[idx] = -1) Then
        Begin
          { Array Of String - need To parse index And get element address }
          NextToken;
          Expect(TOK_LBRACKET);
          ParseExpression;  { index In x0 }
          Expect(TOK_RBRACKET);
          { Subtract low bound }
          EmitPushX0;
          EmitMovX0(sym_const_val[idx]);
          EmitPopX1;
          WriteLn('    sub x0, x1, x0');
          { Multiply by 256 }
          WriteLn('    lsl x0, x0, #8');
          { Get base address }
          If sym_level[idx] < scope_level Then
          Begin
            EmitFollowChain(sym_level[idx], scope_level);
            EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
          End
          Else
            EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
          { Compute element address }
          WriteLn('    sub x0, x1, x0');
          { Load Length byte from [x0] }
          WriteLn('    ldrb w0, [x0]');
        End
        Else
          Error(9);
      End
      Else
        Error(9);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    End
    { eof = 101,111,102 }
    Else If (tok_len = 3) And (ToLower(tok_str[0]) = 101) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 102) Then
    Begin
      { eof(f) - check If at End Of file }
      { Uses lseek To compare current position With file size }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get file fd And save For later }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      WriteLn('    ldr x0, [x0]');
      { Save fd To x23 (callee-saved) }
      WriteLn('    mov x23, x0');
      { lseek(fd, 0, SEEK_CUR=1) To get current position }
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #1');
      { lseek syscall: 0x20000C7 }
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = current position, save To x24 }
      WriteLn('    mov x24, x0');
      { lseek(fd, 0, SEEK_END=2) To get file size }
      WriteLn('    mov x0, x23');
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #2');
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = file size, x24 = current position }
      { Restore file position: lseek(fd, current_pos, SEEK_SET=0) }
      WriteLn('    mov x25, x0');
      WriteLn('    mov x0, x23');
      WriteLn('    mov x1, x24');
      WriteLn('    mov x2, #0');
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Compare: x24 = current_pos, x25 = file_size }
      { cmp x24, x25 }
      WriteLn('    cmp x24, x25');
      { cset x0, ge - x0 = 1 If current_pos >= file_size }
      WriteLn('    cset x0, ge');
      expr_type := TYPE_BOOLEAN
    End
    { filepos = 102,105,108,101,112,111,115 }
    Else If (tok_len = 7) And (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 105) And
            (ToLower(tok_str[2]) = 108) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 112) And
            (ToLower(tok_str[5]) = 111) And (ToLower(tok_str[6]) = 115) Then
    Begin
      { filepos(f) - get current position In file }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get file fd }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      WriteLn('    ldr x0, [x0]');
      { lseek(fd, 0, SEEK_CUR=1) To get current position }
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #1');
      { lseek syscall: 0x20000C7 }
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = current position }
      expr_type := TYPE_INTEGER
    End
    { filesize = 102,105,108,101,115,105,122,101 }
    Else If (tok_len = 8) And (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 105) And
            (ToLower(tok_str[2]) = 108) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 115) And
            (ToLower(tok_str[5]) = 105) And (ToLower(tok_str[6]) = 122) And (ToLower(tok_str[7]) = 101) Then
    Begin
      { filesize(f) - get file size }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get file fd And save For later }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      WriteLn('    ldr x0, [x0]');
      { Save fd To x23 }
      WriteLn('    mov x23, x0');
      { lseek(fd, 0, SEEK_CUR=1) To get current position (To restore later) }
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #1');
      { lseek syscall: 0x20000C7 }
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = current position, save To x24 }
      WriteLn('    mov x24, x0');
      { lseek(fd, 0, SEEK_END=2) To get file size }
      WriteLn('    mov x0, x23');
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #2');
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = file size, save To x25 }
      WriteLn('    mov x25, x0');
      { Restore file position: lseek(fd, current_pos, SEEK_SET=0) }
      WriteLn('    mov x0, x23');
      WriteLn('    mov x1, x24');
      WriteLn('    mov x2, #0');
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Move file size To x0 as return value }
      WriteLn('    mov x0, x25');
      expr_type := TYPE_INTEGER
    End
    { copy = 99,111,112,121 }
    Else If (tok_len = 4) And (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 112) And (ToLower(tok_str[3]) = 121) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: source String }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { source addr In x0 }
        EmitPushX0
      End
      Else
        Error(9);
      Expect(TOK_COMMA);
      { Second arg: start index }
      ParseExpression;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Third arg: count }
      ParseExpression;  { count In x0 }
      { x0 = count, x1 = start, x2 = source addr }
      WriteLn('    mov x3, x0');
      EmitPopX1;  { start }
      EmitPopX0;  { source addr -> x2 }
      WriteLn('    mov x2, x0');
      { Allocate temp String from heap: x0 = x21, x21 += 256 }
      WriteLn('    mov x0, x21');
      WriteLn('    add x21, x21, #256');
      { Save dest addr }
      EmitPushX0;
      { Inline copy logic: copy from source[start] To dest, count bytes }
      { x0 = dest, x1 = start, x2 = source, x3 = count }
      { Store count as Length at dest[0] }
      WriteLn('    strb w3, [x0]');
      { Loop: copy count bytes from source[start+i] To dest[1+i] }
      lbl1 := NewLabel;
      WriteLn('    mov x4, #0');
      EmitLabel(lbl1);
      { cmp x4, x3 }
      WriteLn('    cmp x4, x3');
      { b.ge done }
      Write('    b.ge L'); WriteLn(label_count);
      { x5 = start + x4 (source index) }
      WriteLn('    add x5, x1, x4');
      { ldrb w6, [x2, x5] }
      WriteLn('    ldrb w6, [x2, x5]');
      { x5 = x4 + 1 (dest index) }
      WriteLn('    add x5, x4, #1');
      { strb w6, [x0, x5] }
      WriteLn('    strb w6, [x0, x5]');
      { x4 = x4 + 1 }
      WriteLn('    add x4, x4, #1');
      EmitBranchLabel(lbl1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      { Restore dest addr To x0 }
      EmitPopX0;
      Expect(TOK_RPAREN);
      expr_type := TYPE_STRING
    End
    { concat = 99,111,110,99,97,116 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 99) And (ToLower(tok_str[4]) = 97) And
            (ToLower(tok_str[5]) = 116) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      { First String }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { string1 addr In x0 }
        EmitPushX0
      End
      Else
        Error(9);
      Expect(TOK_COMMA);
      { Second String }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { string2 addr In x0 }
        { x0 = string2, stack top = string1 }
        WriteLn('    mov x2, x0');
        EmitPopX1;  { string1 }
      End
      Else
        Error(9);
      { Allocate temp String from heap: x0 = x21, x21 += 256 }
      WriteLn('    mov x0, x21');
      WriteLn('    add x21, x21, #256');
      { Call rt_str_concat(x0=dest, x1=str1, x2=str2) }
      EmitBL(rt_str_concat);
      Expect(TOK_RPAREN);
      expr_type := TYPE_STRING
    End
    { trim = 116,114,105,109 }
    Else If (tok_len = 4) And (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 109) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      End
      Else
        Error(9);
      Expect(TOK_RPAREN);
      EmitBL(rt_str_trim);
      expr_type := TYPE_STRING
    End
    { ltrim = 108,116,114,105,109 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 108) And (ToLower(tok_str[1]) = 116) And
            (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 109) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      End
      Else
        Error(9);
      Expect(TOK_RPAREN);
      EmitBL(rt_str_ltrim);
      expr_type := TYPE_STRING
    End
    { rtrim = 114,116,114,105,109 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 116) And
            (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 109) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      End
      Else
        Error(9);
      Expect(TOK_RPAREN);
      EmitBL(rt_str_rtrim);
      expr_type := TYPE_STRING
    End
    { pos = 112,111,115 }
    Else If (tok_len = 3) And (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 115) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: substring }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);
        EmitPushX0
      End
      Else If tok_type = TOK_STRING Then
      Begin
        { String literal - allocate temp on heap }
        WriteLn('    mov x8, x21');
        EmitMovX0(tok_len);
        WriteLn('    strb w0, [x8]');
        For i := 0 To tok_len - 1 Do
        Begin
          EmitMovX0(tok_str[i]);
          Write('    strb w0, [x8, #'); Write(i + 1); WriteLn(']');
        End;
        WriteLn('    mov x0, x21');
        WriteLn('    add x21, x21, #256');
        EmitPushX0;
        NextToken
      End
      Else
        Error(9);
      Expect(TOK_COMMA);
      { Second arg: String To search In }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { String addr In x0 }
        { mov x1, x0; pop x0 }
        WriteLn('    mov x1, x0');
        EmitPopX0  { substr In x0 }
      End
      Else
        Error(9);
      { Call rt_str_pos(x0=substr, x1=String) }
      EmitBL(rt_str_pos);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    End
    { paramcount = 112,97,114,97,109,99,111,117,110,116 }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 97) And
            (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 97) And (ToLower(tok_str[4]) = 109) And
            (ToLower(tok_str[5]) = 99) And (ToLower(tok_str[6]) = 111) And (ToLower(tok_str[7]) = 117) And
            (ToLower(tok_str[8]) = 110) And (ToLower(tok_str[9]) = 116) Then
    Begin
      { paramcount - returns argc - 1 (number Of command-line parameters) }
      NextToken;
      { x25 holds argc, return argc - 1 }
      WriteLn('    sub x0, x25, #1');
      expr_type := TYPE_INTEGER
    End
    { paramstr = 112,97,114,97,109,115,116,114 }
    Else If (tok_len = 8) And (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 97) And
            (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 97) And (ToLower(tok_str[4]) = 109) And
            (ToLower(tok_str[5]) = 115) And (ToLower(tok_str[6]) = 116) And (ToLower(tok_str[7]) = 114) Then
    Begin
      { paramstr(n) - returns argv[n] as Pascal String }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { n In x0 }
      Expect(TOK_RPAREN);
      EmitBL(rt_paramstr);
      expr_type := TYPE_STRING
    End
    Else
      Error(3)  { undefined identifier - not a user symbol or builtin }
    End  { End of builtin function checks }
  End  { End Of Else If tok_type = TOK_IDENT }
  Else
    Error(5)
End;

Procedure ParseUnary;
Begin
  If tok_type = TOK_MINUS Then
  Begin
    NextToken;
    ParseFactor;
    If expr_type = TYPE_REAL Then
      EmitFNeg
    Else
      EmitNeg
  End
  Else If tok_type = TOK_PLUS Then
  Begin
    NextToken;
    ParseFactor
  End
  Else
    ParseFactor
End;

Procedure ParseTerm;
Var
  op, left_type, and_skip_label, had_and: Integer;
Begin
  ParseUnary;
  had_and := 0;
  and_skip_label := 0;
  While (tok_type = TOK_STAR) Or (tok_type = TOK_DIV) Or (tok_type = TOK_MOD) Or
        (tok_type = TOK_AND) Or (tok_type = TOK_SLASH) Do
  Begin
    op := tok_type;
    left_type := expr_type;
    NextToken;

    If op = TOK_AND Then
    Begin
      { Short-circuit And: If current value is 0, skip rest }
      If had_and = 0 Then
      Begin
        and_skip_label := NewLabel;
        had_and := 1
      End;
      { cbz x0, .Lskip - branch If zero }
      EmitBranchLabelZ(and_skip_label);
      ParseUnary;
      expr_type := TYPE_INTEGER
    End
    Else
    Begin
      { Regular operators - push left, eval right, pop, compute }
      If left_type = TYPE_REAL Then
        EmitPushD0
      Else
        EmitPushX0;
      ParseUnary;
      { right operand is now In x0 Or d0 depending on expr_type }

      If op = TOK_SLASH Then
      Begin
        { / always produces Real - convert both operands To float }
        If expr_type <> TYPE_REAL Then
          EmitScvtfD0X0;  { convert right To float }
        If left_type = TYPE_REAL Then
          EmitPopD1
        Else
        Begin
          EmitPopX1;
          EmitScvtfD1X1  { convert left To float }
        End;
        EmitFDiv;
        expr_type := TYPE_REAL
      End
      Else If (left_type = TYPE_REAL) Or (expr_type = TYPE_REAL) Then
      Begin
        { Mixed Or both Real - use float ops }
        If expr_type <> TYPE_REAL Then
          EmitScvtfD0X0;  { convert right To float }
        If left_type = TYPE_REAL Then
          EmitPopD1
        Else
        Begin
          EmitPopX1;
          EmitScvtfD1X1  { convert left To float }
        End;
        If op = TOK_STAR Then
          EmitFMul
        Else If op = TOK_DIV Then
        Begin
          { Div on floats - truncate result To Integer }
          EmitFDiv;
          EmitFcvtzsX0D0;
          expr_type := TYPE_INTEGER
        End
        Else If op = TOK_MOD Then
          Error(13);  { Mod Not supported For reals }
        If (op = TOK_STAR) Then
          expr_type := TYPE_REAL
      End
      Else If (left_type = TYPE_SET) Or (expr_type = TYPE_SET) Then
      Begin
        { Set intersection: x0 = x1 And x0 }
        EmitPopX1;
        WriteLn('    And x0, x1, x0');
        expr_type := TYPE_SET
      End
      Else
      Begin
        { Both integers - use Integer ops }
        EmitPopX1;
        If op = TOK_STAR Then
          EmitMul
        Else If op = TOK_DIV Then
          EmitSDiv
        Else If op = TOK_MOD Then
        Begin
          { x1 Mod x0: x1 - (x1 / x0) * x0 }
          EmitPushX0;
          EmitPushX1;
          EmitSDiv;
          EmitMovX2X0;
          EmitPopX1;
          EmitPopX0;
          EmitMsub
        End;
        expr_type := TYPE_INTEGER
      End
    End
  End;

  { Emit skip label For short-circuit And If we had any }
  If had_and = 1 Then
    EmitLabel(and_skip_label)
End;

Procedure ParseSimpleExpr;
Var
  op, left_type, left_ptr_base, or_true_label, or_end_label, had_or: Integer;
Begin
  ParseTerm;
  had_or := 0;
  or_true_label := 0;
  or_end_label := 0;
  While (tok_type = TOK_PLUS) Or (tok_type = TOK_MINUS) Or (tok_type = TOK_OR) Do
  Begin
    op := tok_type;
    left_type := expr_type;
    left_ptr_base := ptr_base_type;
    NextToken;

    If op = TOK_OR Then
    Begin
      { Short-circuit Or: If current value is non-zero, skip rest With result 1 }
      If had_or = 0 Then
      Begin
        or_true_label := NewLabel;
        or_end_label := NewLabel;
        had_or := 1
      End;
      { cbnz x0, .Ltrue - branch If Not zero }
      EmitBranchLabelNZ(or_true_label);
      ParseTerm;
      expr_type := TYPE_INTEGER
    End
    Else
    Begin
      { Regular operators (+, -) - push left, eval right, pop, compute }
      If left_type = TYPE_REAL Then
        EmitPushD0
      Else
        EmitPushX0;
      ParseTerm;

      If (left_type = TYPE_POINTER) And (op = TOK_PLUS) Then
    Begin
      { pointer + Integer: scale Integer by 8 And SUBTRACT (arrays grow downward) }
      EmitPopX1;  { pointer In x1 }
      { x0 has Integer offset, multiply by 8 using lsl #3 }
      WriteLn('    lsl x0, x0, #3');
      EmitSub;  { x0 = x1 - x0 (subtract because arrays grow downward) }
      expr_type := TYPE_POINTER;
      ptr_base_type := left_ptr_base
    End
    Else If (left_type = TYPE_POINTER) And (op = TOK_MINUS) Then
    Begin
      If expr_type = TYPE_POINTER Then
      Begin
        { pointer - pointer: returns Integer count (negated For downward growth) }
        EmitPopX1;  { left pointer In x1 }
        EmitSub;  { x0 = x1 - x0 }
        { Negate And divide by 8 For correct count }
        EmitNeg;  { x0 = -(x1 - x0) = x0 - x1 }
        { Divide by 8 (element size) using asr #3 }
        WriteLn('    asr x0, x0, #3');
        expr_type := TYPE_INTEGER
      End
      Else
      Begin
        { pointer - Integer: scale Integer by 8 And ADD (arrays grow downward) }
        EmitPopX1;  { pointer In x1 }
        { x0 has Integer offset, multiply by 8 using lsl #3 }
        WriteLn('    lsl x0, x0, #3');
        EmitAdd;  { x0 = x1 + x0 (add because arrays grow downward) }
        expr_type := TYPE_POINTER;
        ptr_base_type := left_ptr_base
      End
    End
    Else If (left_type = TYPE_STRING) And (expr_type = TYPE_STRING) And (op = TOK_PLUS) Then
    Begin
      { String concatenation: str1 + str2 }
      { x0 = string2 addr, stack top = string1 addr }
      WriteLn('    mov x2, x0');
      EmitPopX1;  { string1 addr }
      { Allocate temp String from heap }
      WriteLn('    mov x0, x21');
      WriteLn('    add x21, x21, #256');
      { Call rt_str_concat(x0=dest, x1=str1, x2=str2) }
      EmitBL(rt_str_concat);
      expr_type := TYPE_STRING
    End
    Else If (((left_type = TYPE_STRING) And (expr_type = TYPE_CHAR)) Or
             ((left_type = TYPE_CHAR) And (expr_type = TYPE_STRING))) And (op = TOK_PLUS) Then
    Begin
      { String + Char Or Char + String concatenation }
      { Need To convert Char To a single-character String, Then concatenate }
      If left_type = TYPE_STRING Then
      Begin
        { String + Char: x0 = Char, stack top = String addr }
        { Create single-Char String from x0 on heap }
        WriteLn('    mov x3, x0');
        WriteLn('    mov x2, x21');
        EmitMovX0(1);  { Length = 1 }
        WriteLn('    strb w0, [x2]');
        WriteLn('    strb w3, [x2, #1]');
        WriteLn('    add x21, x21, #256');
        EmitPopX1;  { string1 addr }
        { x2 = Char String, x1 = string1, allocate result on heap }
        WriteLn('    mov x0, x21');
        WriteLn('    add x21, x21, #256');
        EmitBL(rt_str_concat)
      End
      Else
      Begin
        { Char + String: x0 = String addr, stack top = Char }
        WriteLn('    mov x2, x0');
        EmitPopX0;  { x0 = Char }
        { Create single-Char String from x0 on heap }
        WriteLn('    mov x1, x21');
        WriteLn('    mov x3, x0');
        EmitMovX0(1);  { Length = 1 }
        WriteLn('    strb w0, [x1]');
        WriteLn('    strb w3, [x1, #1]');
        WriteLn('    add x21, x21, #256');
        { x1 = Char String, x2 = string2, allocate result on heap }
        WriteLn('    mov x0, x21');
        WriteLn('    add x21, x21, #256');
        EmitBL(rt_str_concat)
      End;
      expr_type := TYPE_STRING
    End
    Else If (left_type = TYPE_SET) Or (expr_type = TYPE_SET) Then
    Begin
      { Set operations: + is union, - is difference }
      EmitPopX1;  { left Set In x1 }
      If op = TOK_PLUS Then
      Begin
        { Union: x0 = x1 Or x0 }
        WriteLn('    orr x0, x1, x0');
      End
      Else If op = TOK_MINUS Then
      Begin
        { Difference: x0 = x1 And Not x0 }
        { First: mvn x0, x0 (Not x0) }
        WriteLn('    mvn x0, x0');
        { Then: And x0, x1, x0 }
        WriteLn('    And x0, x1, x0');
      End;
      expr_type := TYPE_SET
    End
    Else If (left_type = TYPE_REAL) Or (expr_type = TYPE_REAL) Then
    Begin
      { Mixed Or both Real - use float ops }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;  { convert right To float }
      If left_type = TYPE_REAL Then
        EmitPopD1
      Else
      Begin
        EmitPopX1;
        EmitScvtfD1X1  { convert left To float }
      End;
      If op = TOK_PLUS Then
        EmitFAdd
      Else If op = TOK_MINUS Then
        EmitFSub
      Else { TOK_OR }
        Error(13);  { Or Not supported For reals }
      expr_type := TYPE_REAL
    End
    Else
    Begin
      { Both integers (only + And - reach here) }
      EmitPopX1;
      If op = TOK_PLUS Then
        EmitAdd
      Else
        EmitSub;
      expr_type := TYPE_INTEGER
    End
    End  { End Of Else For non-Or operators }
  End;

  { Emit labels For short-circuit Or If we had any }
  If had_or = 1 Then
  Begin
    { b .Lend - skip the mov x0, #1 }
    EmitBranchLabel(or_end_label);
    EmitLabel(or_true_label);
    EmitMovX0(1);
    EmitLabel(or_end_label)
  End
End;

Procedure ParseExpression;
Var
  op, cond, left_type: Integer;
Begin
  ParseSimpleExpr;
  If (tok_type = TOK_EQ) Or (tok_type = TOK_NEQ) Or (tok_type = TOK_LT) Or
     (tok_type = TOK_LE) Or (tok_type = TOK_GT) Or (tok_type = TOK_GE) Then
  Begin
    op := tok_type;
    left_type := expr_type;
    NextToken;
    { Push left operand appropriately }
    If left_type = TYPE_REAL Then
      EmitPushD0
    Else
      EmitPushX0;
    ParseSimpleExpr;

    If (left_type = TYPE_STRING) And (expr_type = TYPE_STRING) Then
    Begin
      { String comparison }
      { x0 = string2 addr, stack top = string1 addr }
      WriteLn('    mov x1, x0');
      EmitPopX0;  { string1 addr In x0 }
      If (op = TOK_EQ) Or (op = TOK_NEQ) Then
      Begin
        { Call rt_str_compare(x0=str1, x1=str2) - returns 1 If equal, 0 If Not }
        EmitBL(rt_str_compare);
        If op = TOK_NEQ Then
          EmitEorX0(1)
      End
      Else
      Begin
        { Relational operators: call rt_str_cmp which returns -1/0/1 }
        EmitBL(rt_str_cmp);
        { x0 = -1 If s1<s2, 0 If s1=s2, 1 If s1>s2 }
        If op = TOK_LT Then
        Begin
          { cmp x0, #0; cset x0, lt }
          WriteLn('    cmp x0, #0');
          WriteLn('    cset x0, lt');
        End
        Else If op = TOK_GT Then
        Begin
          { cmp x0, #0; cset x0, gt }
          WriteLn('    cmp x0, #0');
          WriteLn('    cset x0, gt');
        End
        Else If op = TOK_LE Then
        Begin
          { cmp x0, #0; cset x0, le }
          WriteLn('    cmp x0, #0');
          WriteLn('    cset x0, le');
        End
        Else If op = TOK_GE Then
        Begin
          { cmp x0, #0; cset x0, ge }
          WriteLn('    cmp x0, #0');
          WriteLn('    cset x0, ge');
        End
      End;
      expr_type := TYPE_INTEGER
    End
    Else If (left_type = TYPE_REAL) Or (expr_type = TYPE_REAL) Then
    Begin
      { Float comparison }
      If expr_type <> TYPE_REAL Then
        EmitScvtfD0X0;  { convert right To float }
      If left_type = TYPE_REAL Then
        EmitPopD1
      Else
      Begin
        EmitPopX1;
        EmitScvtfD1X1  { convert left To float }
      End;
      EmitFCmp;
      If op = TOK_EQ Then cond := 0
      Else If op = TOK_NEQ Then cond := 1
      Else If op = TOK_LT Then cond := 2
      Else If op = TOK_LE Then cond := 3
      Else If op = TOK_GT Then cond := 4
      Else cond := 5;
      EmitCset(cond);
      expr_type := TYPE_INTEGER
    End
    Else
    Begin
      { Integer comparison }
      EmitPopX1;
      EmitCmpX0X1;
      If op = TOK_EQ Then cond := 0
      Else If op = TOK_NEQ Then cond := 1
      Else If op = TOK_LT Then cond := 2
      Else If op = TOK_LE Then cond := 3
      Else If op = TOK_GT Then cond := 4
      Else cond := 5;
      EmitCset(cond);
      expr_type := TYPE_INTEGER
    End
  End
  Else If tok_type = TOK_IN Then
  Begin
    { Set membership: value In Set }
    { x0 = value, need To check If bit is Set In the Set }
    EmitPushX0;  { push value }
    NextToken;
    ParseSimpleExpr;  { Set In x0 }
    { x0 = Set, stack = value }
    { Result: (Set >> value) & 1 }
    EmitPopX1;  { value In x1 }
    { lsr x0, x0, x1 }
    WriteLn('    lsr x0, x0, x1');
    { And x0, x0, #1 }
    EmitAndImm(1);
    expr_type := TYPE_BOOLEAN
  End
End;

Procedure ParseStatement;
Var
  idx, lbl1, lbl2, lbl3, arg_count, i: Integer;
  var_flags, arg_idx, var_arg_idx: Integer;
  old_break, old_continue: Integer;
  dim_idx, dim_count, dim_lo, dim_size: Integer;
Begin
  If tok_type = TOK_BEGIN Then
  Begin
    NextToken;
    ParseStatement;
    While tok_type = TOK_SEMICOLON Do
    Begin
      NextToken;
      ParseStatement
    End;
    Expect(TOK_END)
  End
  Else If tok_type = TOK_IF Then
  Begin
    NextToken;
    ParseExpression;
    Expect(TOK_THEN);
    lbl1 := NewLabel;
    lbl2 := NewLabel;
    EmitBranchLabelZ(lbl1);
    ParseStatement;
    If tok_type = TOK_ELSE Then
    Begin
      EmitBranchLabel(lbl2);
      EmitLabel(lbl1);
      NextToken;
      ParseStatement;
      EmitLabel(lbl2)
    End
    Else
      EmitLabel(lbl1)
  End
  Else If tok_type = TOK_WHILE Then
  Begin
    lbl1 := NewLabel;  { loop start / Continue target }
    lbl2 := NewLabel;  { loop End / Break target }
    { Save old Break/Continue labels }
    old_break := break_label;
    old_continue := continue_label;
    break_label := lbl2;
    continue_label := lbl1;
    EmitLabel(lbl1);
    NextToken;
    ParseExpression;
    Expect(TOK_DO);
    EmitBranchLabelZ(lbl2);
    ParseStatement;
    EmitBranchLabel(lbl1);
    EmitLabel(lbl2);
    { Restore old Break/Continue labels }
    break_label := old_break;
    continue_label := old_continue
  End
  Else If tok_type = TOK_REPEAT Then
  Begin
    lbl1 := NewLabel;  { loop start / Continue target }
    lbl2 := NewLabel;  { loop End / Break target }
    { Save old Break/Continue labels }
    old_break := break_label;
    old_continue := continue_label;
    break_label := lbl2;
    continue_label := lbl1;
    EmitLabel(lbl1);
    NextToken;
    ParseStatement;
    While tok_type = TOK_SEMICOLON Do
    Begin
      NextToken;
      ParseStatement
    End;
    Expect(TOK_UNTIL);
    ParseExpression;
    EmitBranchLabelZ(lbl1);
    EmitLabel(lbl2);
    { Restore old Break/Continue labels }
    break_label := old_break;
    continue_label := old_continue
  End
  Else If tok_type = TOK_FOR Then
  Begin
    NextToken;
    If tok_type <> TOK_IDENT Then
      Error(6);
    idx := SymLookup;
    If idx < 0 Then
      Error(3);
    NextToken;
    Expect(TOK_ASSIGN);
    ParseExpression;
    If sym_level[idx] < scope_level Then
      EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
    Else
      EmitSturX0(sym_offset[idx]);

    lbl1 := NewLabel;  { condition check }
    lbl2 := NewLabel;  { loop End / Break target }
    lbl3 := NewLabel;  { increment / Continue target }

    { Save old Break/Continue labels }
    old_break := break_label;
    old_continue := continue_label;
    break_label := lbl2;
    continue_label := lbl3;

    If tok_type = TOK_TO Then
    Begin
      NextToken;
      ParseExpression;  { End value into x0 }
      EmitPushX0;       { save End value on stack }
      Expect(TOK_DO);
      EmitLabel(lbl1);
      { load loop Var }
      If sym_level[idx] < scope_level Then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitLdurX0(sym_offset[idx]);
      { ldur x1, [sp] - load End value from stack }
      WriteLn('    ldur x1, [sp]');
      EmitCmpX0X1;
      EmitCset(2);  { lt: Exit when End < i, meaning i > End }
      EmitBranchLabelNZ(lbl2);
      ParseStatement;
      { Continue target - increment }
      EmitLabel(lbl3);
      If sym_level[idx] < scope_level Then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitLdurX0(sym_offset[idx]);
      WriteLn('    add x0, x0, #1');
      If sym_level[idx] < scope_level Then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitSturX0(sym_offset[idx]);
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { Restore old Break/Continue labels }
      break_label := old_break;
      continue_label := old_continue;
      { Pop End value from stack }
      WriteLn('    add sp, sp, #16');
    End
    Else
    Begin
      Expect(TOK_DOWNTO);
      ParseExpression;  { End value into x0 }
      EmitPushX0;       { save End value on stack }
      Expect(TOK_DO);
      EmitLabel(lbl1);
      { load loop Var }
      If sym_level[idx] < scope_level Then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitLdurX0(sym_offset[idx]);
      { ldur x1, [sp] - load End value from stack }
      WriteLn('    ldur x1, [sp]');
      EmitCmpX0X1;
      EmitCset(4);  { gt: Exit when End > i, meaning i < End }
      EmitBranchLabelNZ(lbl2);
      ParseStatement;
      { Continue target - decrement }
      EmitLabel(lbl3);
      If sym_level[idx] < scope_level Then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitLdurX0(sym_offset[idx]);
      WriteLn('    sub x0, x0, #1');
      If sym_level[idx] < scope_level Then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitSturX0(sym_offset[idx]);
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { Restore old Break/Continue labels }
      break_label := old_break;
      continue_label := old_continue;
      { Pop End value from stack }
      WriteLn('    add sp, sp, #16');
    End
  End
  Else If tok_type = TOK_CASE Then
  Begin
    { Case selector Of Const: stmt; ... [Else stmt] End }
    NextToken;
    ParseExpression;  { selector In x0 }
    EmitPushX0;       { save selector on stack }
    Expect(TOK_OF);
    lbl1 := NewLabel;  { End Of Case label }

    While (tok_type <> TOK_END) And (tok_type <> TOK_ELSE) Do
    Begin
      { Parse constant(s) For this Case branch }
      lbl2 := NewLabel;  { next Case branch label }
      lbl3 := NewLabel;  { match found, execute statement }
      Repeat
        If tok_type = TOK_COMMA Then NextToken;
        { Load selector from stack (peek, don't pop): ldr x1, [sp] }
        WriteLn('    ldr x1, [sp]');
        { Parse constant value }
        If tok_type = TOK_INTEGER Then
        Begin
          EmitMovX0(tok_int);
          NextToken
        End
        Else If tok_type = TOK_MINUS Then
        Begin
          NextToken;
          If tok_type = TOK_INTEGER Then
          Begin
            EmitMovX0(0 - tok_int);
            NextToken
          End
          Else
            Error(10)
        End
        Else
          Error(10);  { expected constant }
        { Compare: cmp x1, x0 }
        EmitCmpX0X1;
        { If equal, jump To match label }
        Write('    b.eq L'); WriteLn(lbl3);
      Until tok_type = TOK_COLON;
      { No match In this group, jump To next branch }
      EmitBranchLabel(lbl2);
      { Match found - execute statement }
      EmitLabel(lbl3);
      NextToken;  { consume ':' }
      { Pop selector from stack (discard) }
      WriteLn('    add sp, sp, #16');
      ParseStatement;
      { Jump To End Of Case }
      EmitBranchLabel(lbl1);
      { Repush selector For next comparisons: sub sp, sp, #16; str x0, [sp] }
      { Actually we need To restore selector - this gets tricky }
      { Simpler approach: jump around the next branch's test }
      EmitLabel(lbl2);
      { After ParseStatement the selector was popped, repush it }
      { For simplicity, we push a placeholder And let the next iteration reload }
      { Actually, we need the selector back - let's use a different approach }
      { The cleanest way: keep selector on stack Until we match Or hit Else/End }
      { Re-push selector: we saved it In x0 before? No. Let's rethink. }
      { Better: save selector To a dedicated spot before the loop }
      { For now: don't pop Until match, just peek. Pop only when match Or done. }
      { Skip the repush - we've jumped To lbl1 If matched, lbl2 If Not }
      { At lbl2, we still have selector on stack }
      If tok_type = TOK_SEMICOLON Then NextToken
    End;

    If tok_type = TOK_ELSE Then
    Begin
      NextToken;
      { Pop selector (discard) before Else statement }
      WriteLn('    add sp, sp, #16');
      ParseStatement;
      { Note: no need To jump To lbl1, we fall through }
    End
    Else
    Begin
      { No Else - just pop selector }
      WriteLn('    add sp, sp, #16');
    End;
    EmitLabel(lbl1);
    Expect(TOK_END)
  End
  Else If tok_type = TOK_READ Then
  Begin
    { Read([f,] Var) - reads an Integer Or Real into a variable }
    NextToken;
    Expect(TOK_LPAREN);
    lbl1 := 0;  { flag: 1 If reading from file }
    If tok_type <> TOK_IDENT Then
      Error(6);
    idx := SymLookup;
    If idx < 0 Then
      Error(3);
    { Check If first arg is a file variable }
    If (sym_type[idx] = TYPE_FILE) Or (sym_type[idx] = TYPE_TEXT) Then
    Begin
      { File variable - save x19 And load file's fd }
      lbl1 := 1;
      NextToken;
      { Push x19 To save it }
      WriteLn('    str x19, [sp, #-16]!');
      { Load file's fd into x19 }
      EmitVarAddr(idx, scope_level);
      WriteLn('    ldr x19, [x0]');
      Expect(TOK_COMMA);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3)
    End;
    NextToken;
    If sym_type[idx] = TYPE_REAL Then
    Begin
      { Call read_real runtime - result In d0 }
      EmitBL(rt_read_real);
      { Store result In variable }
      If sym_level[idx] = scope_level Then
        EmitSturD0(sym_offset[idx])
      Else
        EmitSturD0Outer(sym_offset[idx], sym_level[idx], scope_level)
    End
    Else
    Begin
      { Call read_int runtime }
      EmitBL(rt_read_int);
      { Store result In variable }
      If sym_level[idx] = scope_level Then
        EmitSturX0(sym_offset[idx])
      Else
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
    End;
    { Restore x19 If we saved it }
    If lbl1 = 1 Then
    Begin
      WriteLn('    ldr x19, [sp], #16');
    End;
    Expect(TOK_RPAREN)
  End
  Else If tok_type = TOK_READLN Then
  Begin
    { ReadLn([f,] Var) Or ReadLn([f]) - reads Integer/Real/String And skips To End Of line }
    NextToken;
    lbl1 := 0;  { flag: 1 If reading from file }
    If tok_type = TOK_LPAREN Then
    Begin
      NextToken;
      If tok_type <> TOK_RPAREN Then
      Begin
        If tok_type <> TOK_IDENT Then
          Error(6);
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        { Check If first arg is a file variable }
        If (sym_type[idx] = TYPE_FILE) Or (sym_type[idx] = TYPE_TEXT) Then
        Begin
          { File variable - save x19 And load file's fd }
          lbl1 := 1;
          NextToken;
          { Push x19 To save it }
          WriteLn('    str x19, [sp, #-16]!');
          { Load file's fd into x19 }
          EmitVarAddr(idx, scope_level);
          WriteLn('    ldr x19, [x0]');
          { Check For comma (more args) Or just skip To rparen }
          If tok_type = TOK_COMMA Then
          Begin
            NextToken;
            If tok_type <> TOK_IDENT Then
              Error(6);
            idx := SymLookup;
            If idx < 0 Then
              Error(3)
          End
          Else
          Begin
            { Just file arg, no variable To Read into - skip To rparen }
            Expect(TOK_RPAREN);
            { Skip To End Of line }
            EmitBL(rt_skip_line);
            { Restore x19 }
            WriteLn('    ldr x19, [sp], #16');
          End
        End;
        { Only process variable If we have one }
        If tok_type = TOK_IDENT Then
        Begin
          idx := SymLookup;
          If idx < 0 Then
            Error(3);
          lbl2 := sym_type[idx];  { save Type For skip_line decision }
          NextToken;
          If sym_type[idx] = TYPE_REAL Then
          Begin
            { Call read_real runtime - result In d0 }
            EmitBL(rt_read_real);
            { Store result In variable }
            If sym_level[idx] = scope_level Then
              EmitSturD0(sym_offset[idx])
            Else
              EmitSturD0Outer(sym_offset[idx], sym_level[idx], scope_level)
          End
          Else If sym_type[idx] = TYPE_STRING Then
          Begin
            { Load String variable address into x0 }
            EmitVarAddr(idx, scope_level);
            { Call read_string runtime - already consumes newline }
            EmitBL(rt_read_string)
          End
          Else
          Begin
            { Call read_int runtime }
            EmitBL(rt_read_int);
            { Store result In variable }
            If sym_level[idx] = scope_level Then
              EmitSturX0(sym_offset[idx])
            Else
              EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          End;
          Expect(TOK_RPAREN);
          { Skip To End Of line only For Integer/Real (read_string already consumed newline) }
          If lbl2 <> TYPE_STRING Then
            EmitBL(rt_skip_line);
          { Restore x19 If we saved it }
          If lbl1 = 1 Then
          Begin
            WriteLn('    ldr x19, [sp], #16');
          End
        End
      End
      Else
      Begin
        { ReadLn() With no args }
        Expect(TOK_RPAREN);
        EmitBL(rt_skip_line)
      End
    End
    Else
    Begin
      { ReadLn With no parens }
      EmitBL(rt_skip_line)
    End
  End
  Else If tok_type = TOK_WITH Then
  Begin
    { With record_var Do statement }
    NextToken;
    If tok_type <> TOK_IDENT Then
      Error(6);  { expected identifier }
    idx := SymLookup;
    If idx < 0 Then
      Error(3);  { undefined identifier }
    If sym_type[idx] <> TYPE_RECORD Then
      Error(9);  { expected Record Type }
    { Save current With context }
    lbl1 := with_rec_idx;  { reuse lbl1 To save old with_rec_idx }
    lbl2 := with_rec_type;  { reuse lbl2 To save old with_rec_type }
    with_rec_idx := idx;
    with_rec_type := sym_const_val[idx];  { the Type definition index }
    NextToken;
    Expect(TOK_DO);
    ParseStatement;
    { Restore With context }
    with_rec_idx := lbl1;
    with_rec_type := lbl2
  End
  Else If tok_type = TOK_IDENT Then
  Begin
    { Check For built-In procedures first }
    { Break = 98,114,101,97,107 }
    If (tok_len = 5) And (ToLower(tok_str[0]) = 98) And (ToLower(tok_str[1]) = 114) And
       (ToLower(tok_str[2]) = 101) And (ToLower(tok_str[3]) = 97) And (ToLower(tok_str[4]) = 107) Then
    Begin
      NextToken;
      If break_label = 0 Then
        Error(15)  { Break Not inside a loop }
      Else
        EmitBranchLabel(break_label)
    End
    { Continue = 99,111,110,116,105,110,117,101 }
    Else If (tok_len = 8) And (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 105) And
            (ToLower(tok_str[5]) = 110) And (ToLower(tok_str[6]) = 117) And (ToLower(tok_str[7]) = 101) Then
    Begin
      NextToken;
      If continue_label = 0 Then
        Error(15)  { Continue Not inside a loop }
      Else
        EmitBranchLabel(continue_label)
    End
    { Exit = 101,120,105,116 }
    Else If (tok_len = 4) And (ToLower(tok_str[0]) = 101) And (ToLower(tok_str[1]) = 120) And
            (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 116) Then
    Begin
      NextToken;
      If exit_label = 0 Then
      Begin
        { In main Program - just call Halt With Exit code 0 }
        EmitMovX0(0);
        { mov x16, #1 }
        WriteLn('    mov x16, #1');
        { svc #0x80 }
        WriteLn('    svc #0x80');
      End
      Else
        EmitBranchLabel(exit_label)
    End
    { Write = 119,114,105,116,101 }
    { WriteLn = 119,114,105,116,101,108,110 }
    { ReadChar = 114,101,97,100,99,104,97,114 }
    { WriteChar = 119,114,105,116,101,99,104,97 - actually too long, use 8 }
    { Halt = 104,97,108,116 }
    Else If TokIs8(119, 114, 105, 116, 101, 108, 110, 0) = 1 Then
    Begin
      { WriteLn }
      NextToken;
      lbl1 := 0;  { flag: 1 If writing To file }
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        If tok_type <> TOK_RPAREN Then
        Begin
          { Check If first arg is a file variable }
          If tok_type = TOK_IDENT Then
          Begin
            idx := SymLookup;
            If (idx >= 0) And ((sym_type[idx] = TYPE_FILE) Or (sym_type[idx] = TYPE_TEXT)) Then
            Begin
              { File variable - save x20 And load file's fd }
              lbl1 := 1;
              NextToken;
              { Push x20 To save it }
              WriteLn('    str x20, [sp, #-16]!');
              { Load file's fd into x20 }
              EmitVarAddr(idx, scope_level);
              WriteLn('    ldr x20, [x0]');
              If tok_type = TOK_COMMA Then
                NextToken
            End
          End;
          If (lbl1 = 0) Or (tok_type <> TOK_RPAREN) Then
          Begin
            While tok_type <> TOK_RPAREN Do
            Begin
              If tok_type = TOK_STRING Then
              Begin
                { Print String literal character by character }
                idx := 0;
                While idx < tok_len Do
                Begin
                  EmitMovX0(tok_str[idx]);
                  EmitBL(rt_print_char);
                  idx := idx + 1
                End;
                NextToken
              End
              Else If tok_type = TOK_IDENT Then
              Begin
                { Check If it's a String variable }
                idx := SymLookup;
                If (idx >= 0) And (sym_type[idx] = TYPE_STRING) Then
                Begin
                  { String variable - compute address And call print_string }
                  NextToken;
                  EmitVarAddr(idx, scope_level);
                  EmitBL(rt_print_string)
                End
                Else
                Begin
                  { Not a String - parse as expression And print based on Type }
                  ParseExpression;
                  If expr_type = TYPE_REAL Then
                    EmitBL(rt_print_real)
                  Else If expr_type = TYPE_STRING Then
                    EmitBL(rt_print_string)
                  Else
                    EmitBL(rt_print_int)
                End
              End
              Else
              Begin
                ParseExpression;
                If expr_type = TYPE_REAL Then
                  EmitBL(rt_print_real)
                Else If expr_type = TYPE_STRING Then
                  EmitBL(rt_print_string)
                Else
                  EmitBL(rt_print_int)
              End;
              If tok_type = TOK_COMMA Then NextToken
            End
          End
        End;
        Expect(TOK_RPAREN)
      End;
      EmitBL(rt_newline);
      { Restore x20 If we saved it }
      If lbl1 = 1 Then
      Begin
        WriteLn('    ldr x20, [sp], #16');
      End
    End
    Else If TokIs8(119, 114, 105, 116, 101, 0, 0, 0) = 1 Then
    Begin
      { Write }
      NextToken;
      lbl1 := 0;  { flag: 1 If writing To file }
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        If tok_type <> TOK_RPAREN Then
        Begin
          { Check If first arg is a file variable }
          If tok_type = TOK_IDENT Then
          Begin
            idx := SymLookup;
            If (idx >= 0) And ((sym_type[idx] = TYPE_FILE) Or (sym_type[idx] = TYPE_TEXT)) Then
            Begin
              { File variable - save x20 And load file's fd }
              lbl1 := 1;
              NextToken;
              { Push x20 To save it }
              WriteLn('    str x20, [sp, #-16]!');
              { Load file's fd into x20 }
              EmitVarAddr(idx, scope_level);
              WriteLn('    ldr x20, [x0]');
              If tok_type = TOK_COMMA Then
                NextToken
            End
          End;
          If (lbl1 = 0) Or (tok_type <> TOK_RPAREN) Then
          Begin
            While tok_type <> TOK_RPAREN Do
            Begin
              If tok_type = TOK_STRING Then
              Begin
                { Print String literal character by character }
                idx := 0;
                While idx < tok_len Do
                Begin
                  EmitMovX0(tok_str[idx]);
                  EmitBL(rt_print_char);
                  idx := idx + 1
                End;
                NextToken
              End
              Else If tok_type = TOK_IDENT Then
              Begin
                { Check If it's a String variable }
                idx := SymLookup;
                If (idx >= 0) And (sym_type[idx] = TYPE_STRING) Then
                Begin
                  { String variable - compute address And call print_string }
                  NextToken;
                  EmitVarAddr(idx, scope_level);
                  EmitBL(rt_print_string)
                End
                Else
                Begin
                  { Not a String - parse as expression And print based on Type }
                  ParseExpression;
                  If expr_type = TYPE_REAL Then
                    EmitBL(rt_print_real)
                  Else If expr_type = TYPE_STRING Then
                    EmitBL(rt_print_string)
                  Else
                    EmitBL(rt_print_int)
                End
              End
              Else
              Begin
                ParseExpression;
                If expr_type = TYPE_REAL Then
                  EmitBL(rt_print_real)
                Else If expr_type = TYPE_STRING Then
                  EmitBL(rt_print_string)
                Else
                  EmitBL(rt_print_int)
              End;
              If tok_type = TOK_COMMA Then NextToken
            End
          End
        End;
        Expect(TOK_RPAREN)
      End;
      { Restore x20 If we saved it }
      If lbl1 = 1 Then
      Begin
        WriteLn('    ldr x20, [sp], #16');
      End
    End
    Else If TokIs8(104, 97, 108, 116, 0, 0, 0, 0) = 1 Then
    Begin
      { Halt }
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        If tok_type <> TOK_RPAREN Then
          ParseExpression
        Else
          EmitMovX0(0);
        Expect(TOK_RPAREN)
      End
      Else
        EmitMovX0(0);
      EmitMovX16(33554433);  { 0x2000001 = Exit }
      EmitSvc
    End
    { randomize = 114,97,110,100,111,109,105,122,101 }
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 97) And
            (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 100) And (ToLower(tok_str[4]) = 111) And
            (ToLower(tok_str[5]) = 109) And (ToLower(tok_str[6]) = 105) And (ToLower(tok_str[7]) = 122) And
            (ToLower(tok_str[8]) = 101) Then
    Begin
      { randomize - seed the PRNG using stack pointer XOR With a constant }
      NextToken;
      If tok_type = TOK_LPAREN Then
      Begin
        NextToken;
        If tok_type <> TOK_RPAREN Then
        Begin
          { randomize(seed) - use provided seed }
          ParseExpression;
          { mov x27, x0 }
          WriteLn('    mov x27, x0');
        End
        Else
        Begin
          { randomize() - use sp as seed }
          WriteLn('    mov x27, sp');
        End;
        Expect(TOK_RPAREN)
      End
      Else
      Begin
        { randomize With no parens - use sp as seed }
        WriteLn('    mov x27, sp');
      End
    End
    { Inc = 105,110,99 }
    Else If TokIs8(105, 110, 99, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(3);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      NextToken;
      { Load address into x8 }
      EmitVarAddr(idx, scope_level);
      WriteLn('    mov x8, x0');
      { Load current value }
      WriteLn('    ldr x0, [x8]');
      { Add 1 }
      WriteLn('    add x0, x0, #1');
      { Store back }
      WriteLn('    str x0, [x8]');
      Expect(TOK_RPAREN)
    End
    { Dec = 100,101,99 }
    Else If TokIs8(100, 101, 99, 0, 0, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(3);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      NextToken;
      { Load address into x8 }
      EmitVarAddr(idx, scope_level);
      WriteLn('    mov x8, x0');
      { Load current value }
      WriteLn('    ldr x0, [x8]');
      { Subtract 1 }
      WriteLn('    sub x0, x0, #1');
      { Store back }
      WriteLn('    str x0, [x8]');
      Expect(TOK_RPAREN)
    End
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 119) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 101) And
            (ToLower(tok_str[5]) = 99) And (ToLower(tok_str[6]) = 104) And (ToLower(tok_str[7]) = 97) And
            (ToLower(tok_str[8]) = 114) Then
    Begin
      { WriteChar - 119,114,105,116,101,99,104,97,114 }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      EmitBL(rt_print_char)
    End
    { writefilechar(file, Char) - 119,114,105,116,101,102,105,108,101,99,104,97,114 }
    Else If (tok_len = 13) And (ToLower(tok_str[0]) = 119) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 101) And
            (ToLower(tok_str[5]) = 102) And (ToLower(tok_str[6]) = 105) And (ToLower(tok_str[7]) = 108) And
            (ToLower(tok_str[8]) = 101) And (ToLower(tok_str[9]) = 99) And (ToLower(tok_str[10]) = 104) And
            (ToLower(tok_str[11]) = 97) And (ToLower(tok_str[12]) = 114) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: file variable }
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      { Load file fd into x0 }
      EmitVarAddr(idx, scope_level);
      WriteLn('    ldr x0, [x0]');
      { Push fd To stack }
      EmitPushX0;
      NextToken;
      Expect(TOK_COMMA);
      { Second arg: character expression }
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 has Char, pop fd into x1, swap }
      WriteLn('    mov x1, x0');
      WriteLn('    ldr x0, [sp], #16');
      { Call Write Char To fd runtime: x0=fd, x1=Char }
      EmitBL(rt_write_char_fd)
    End
    Else If (tok_len = 8) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 97) And (ToLower(tok_str[3]) = 100) And (ToLower(tok_str[4]) = 99) And
            (ToLower(tok_str[5]) = 104) And (ToLower(tok_str[6]) = 97) And (ToLower(tok_str[7]) = 114) Then
    Begin
      { ReadChar - 114,101,97,100,99,104,97,114 }
      NextToken;
      EmitBL(rt_readchar)
    End
    Else If TokIs8(110, 101, 119, 0, 0, 0, 0, 0) = 1 Then
    Begin
      { New(p) - allocate memory For pointer variable }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);  { expected identifier }
      idx := SymLookup;
      If idx < 0 Then
        Error(3);  { undefined identifier }
      If sym_type[idx] <> TYPE_POINTER Then
        Error(14);  { expected pointer Type }
      NextToken;
      Expect(TOK_RPAREN);
      { Determine allocation size based on base Type }
      If sym_const_val[idx] = TYPE_RECORD Then
        lbl1 := sym_label[sym_label[idx]]  { Record size from Type definition }
      Else If sym_const_val[idx] = TYPE_ARRAY Then
      Begin
        { Pointer To Array: calculate size from bounds }
        arg_count := sym_label[idx];  { ptr_arr index }
        lbl1 := (ptr_arr_hi[arg_count] - ptr_arr_lo[arg_count] + 1) * 8
      End
      Else
        lbl1 := 8;  { basic types are 8 bytes }
      { Align To 8 bytes }
      lbl1 := ((lbl1 + 7) Div 8) * 8;
      { Allocate via rt_alloc: put size In x0, call rt_alloc }
      EmitMovX0(lbl1);
      EmitBL(rt_alloc);
      { Store address In pointer variable }
      If sym_level[idx] < scope_level Then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitSturX0(sym_offset[idx])
    End
    { GetMem = 103,101,116,109,101,109 - allocate memory with size }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 103) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 109) And (ToLower(tok_str[4]) = 101) And
            (ToLower(tok_str[5]) = 109) Then
    Begin
      { GetMem(p, size) - allocate size bytes, store address in p }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);  { expected identifier }
      idx := SymLookup;
      If idx < 0 Then
        Error(3);  { undefined identifier }
      NextToken;
      Expect(TOK_COMMA);
      ParseExpression;  { size in x0 }
      Expect(TOK_RPAREN);
      { Align to 8 bytes }
      WriteLn('    add x0, x0, #7');
      WriteLn('    and x0, x0, #-8');
      { Allocate via rt_alloc: size in x0, returns address in x0 }
      EmitBL(rt_alloc);
      { Store address in pointer variable }
      If sym_level[idx] < scope_level Then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitSturX0(sym_offset[idx])
    End
    { FreeMem = 102,114,101,101,109,101,109 - alias for Dispose }
    Else If (tok_len = 7) And (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 101) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 109) And
            (ToLower(tok_str[5]) = 101) And (ToLower(tok_str[6]) = 109) Then
    Begin
      { FreeMem(p) - same as Dispose }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);  { expected identifier }
      idx := SymLookup;
      If idx < 0 Then
        Error(3);  { undefined identifier }
      NextToken;
      { Optional size parameter - ignore it }
      If tok_type = TOK_COMMA Then
      Begin
        NextToken;
        ParseExpression  { ignore size }
      End;
      Expect(TOK_RPAREN);
      { Load pointer value into x0 }
      If sym_level[idx] < scope_level Then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitLdurX0(sym_offset[idx]);
      EmitBL(rt_free);
      EmitMovX0(0);
      If sym_level[idx] < scope_level Then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitSturX0(sym_offset[idx])
    End
    Else If (tok_len = 7) And (ToLower(tok_str[0]) = 100) And (ToLower(tok_str[1]) = 105) And
            (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 112) And (ToLower(tok_str[4]) = 111) And
            (ToLower(tok_str[5]) = 115) And (ToLower(tok_str[6]) = 101) Then
    Begin
      { Dispose(p) - free memory via free list allocator }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);  { expected identifier }
      idx := SymLookup;
      If idx < 0 Then
        Error(3);  { undefined identifier }
      If sym_type[idx] <> TYPE_POINTER Then
        Error(14);  { expected pointer Type }
      NextToken;
      Expect(TOK_RPAREN);
      { Load pointer value into x0 }
      If sym_level[idx] < scope_level Then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitLdurX0(sym_offset[idx]);
      { Call rt_free To return memory To free list }
      EmitBL(rt_free);
      { Set pointer To Nil For safety }
      EmitMovX0(0);
      If sym_level[idx] < scope_level Then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      Else
        EmitSturX0(sym_offset[idx])
    End
    { FillChar = 102,105,108,108,99,104,97,114 - fill memory with byte }
    Else If TokIs8(102, 105, 108, 108, 99, 104, 97, 114) = 1 Then
    Begin
      { FillChar(var x; count: Word; value: Byte) }
      NextToken;
      Expect(TOK_LPAREN);
      { Get address of variable }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then Error(3);
        EmitVarAddr(idx, scope_level)
      End
      Else
        Error(6);
      NextToken;
      EmitPushX0;  { save address }
      Expect(TOK_COMMA);
      ParseExpression;  { count }
      EmitPushX0;  { save count }
      Expect(TOK_COMMA);
      ParseExpression;  { value }
      { x0 = value, stack has count, address }
      WriteLn('    mov x2, x0');    { x2 = value }
      EmitPopX1;                    { x1 = count }
      EmitPopX0;                    { x0 = address }
      EmitBL(rt_fillchar);
      Expect(TOK_RPAREN)
    End
    { Move = 109,111,118,101 - copy memory }
    Else If TokIs8(109, 111, 118, 101, 0, 0, 0, 0) = 1 Then
    Begin
      { Move(const source; var dest; count: Integer) }
      NextToken;
      Expect(TOK_LPAREN);
      { Get address of source }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then Error(3);
        EmitVarAddr(idx, scope_level)
      End
      Else
        Error(6);
      NextToken;
      EmitPushX0;  { save source address }
      Expect(TOK_COMMA);
      { Get address of dest }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then Error(3);
        EmitVarAddr(idx, scope_level)
      End
      Else
        Error(6);
      NextToken;
      EmitPushX0;  { save dest address }
      Expect(TOK_COMMA);
      ParseExpression;  { count }
      { x0 = count, stack has dest, source }
      WriteLn('    mov x2, x0');    { x2 = count }
      EmitPopX1;                    { x1 = dest }
      EmitPopX0;                    { x0 = source }
      EmitBL(rt_move);
      Expect(TOK_RPAREN)
    End
    { assign = 97,115,115,105,103,110 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 115) And
            (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 103) And
            (ToLower(tok_str[5]) = 110) Then
    Begin
      { assign(f, filename) - associate file variable With filename }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_COMMA);
      { Get address Of file variable }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;  { save file Var address }
      { Parse filename (String expression) }
      ParseExpression;
      { x0 = source String address, stack = file Var address }
      { Copy filename To file Var offset 16 }
      EmitPopX1;  { x1 = file Var address }
      { add x1, x1, #16 - point To filename area }
      WriteLn('    add x1, x1, #16');
      { swap x0 And x1 For str_copy (dest In x0, src In x1) }
      { x0 = src String, x1 = dest (file Var + 16) }
      WriteLn('    mov x2, x0');
      WriteLn('    mov x0, x1');
      WriteLn('    mov x1, x2');
      { Now x0 = dest, x1 = src. Save dest before call. }
      EmitPushX0;
      EmitBL(rt_str_copy);
      EmitPopX0;  { restore dest address }
      { Add null terminator For C String compatibility }
      { ldrb w1, [x0] - load Length }
      WriteLn('    ldrb w1, [x0]');
      { add x1, x1, #1 - position after last Char }
      WriteLn('    add x1, x1, #1');
      { strb wzr, [x0, x1] - store null byte }
      WriteLn('    strb wzr, [x0, x1]');
      { Initialize fd To -1 (Not open) }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      EmitMovX0(-1);
      EmitPopX1;
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      Expect(TOK_RPAREN)
    End
    { assigntokstr = 97,115,115,105,103,110,116,111,107,115,116,114 }
    Else If (tok_len = 12) And (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 115) And
            (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 103) And
            (ToLower(tok_str[5]) = 110) And (ToLower(tok_str[6]) = 116) And (ToLower(tok_str[7]) = 111) And
            (ToLower(tok_str[8]) = 107) And (ToLower(tok_str[9]) = 115) And (ToLower(tok_str[10]) = 116) And
            (ToLower(tok_str[11]) = 114) Then
    Begin
      { assigntokstr(f, start, len) - assign filename from tok_str To file }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_COMMA);
      { Parse start index }
      ParseExpression;
      EmitPushX0;  { save start }
      Expect(TOK_COMMA);
      { Parse Length }
      ParseExpression;
      { x0 = len, stack has start }
      { mov x2, x0 - x2 = len }
      WriteLn('    mov x2, x0');
      EmitPopX1;  { x1 = start }
      { Get address Of tok_str using VarAddr }
      { We need the tok_str global - find it In symbol table }
      { For now, use direct offset calculation since tok_str is a global Array }
      { Get file Var base address first }
      EmitVarAddr(idx, scope_level);
      { add x0, x0, #16 - point To filename area }
      WriteLn('    add x0, x0, #16');
      { Save dest address }
      EmitPushX0;
      { Get tok_str base address - look it up }
      { str x1, [sp, #-16]! - save start index }
      WriteLn('    str x1, [sp, #-16]!');
      { str x2, [sp, #-16]! - save len }
      WriteLn('    str x2, [sp, #-16]!');
      { Find tok_str In symbol table And get its address }
      tok_str[0] := 116; tok_str[1] := 111; tok_str[2] := 107; tok_str[3] := 95;
      tok_str[4] := 115; tok_str[5] := 116; tok_str[6] := 114; tok_str[7] := 0;
      tok_len := 7;
      arg_idx := SymLookup;
      If arg_idx < 0 Then
        Error(3);  { tok_str Not found }
      EmitVarAddr(arg_idx, scope_level);  { x0 = tok_str base }
      { mov x3, x0 - x3 = tok_str base }
      WriteLn('    mov x3, x0');
      { ldr x2, [sp], #16 - restore len }
      WriteLn('    ldr x2, [sp], #16');
      { ldr x1, [sp], #16 - restore start }
      WriteLn('    ldr x1, [sp], #16');
      { ldr x0, [sp], #16 - restore dest }
      EmitPopX0;
      { x3 + x1*8 = source address (tok_str elements are 8 bytes) }
      { lsl x1, x1, #3 }
      WriteLn('    lsl x1, x1, #3');
      { add x3, x3, x1 - x3 = source address }
      WriteLn('    add x3, x3, x1');
      { Copy loop: copy x2 characters from [x3] To [x0], skip Length byte }
      { add x0, x0, #1 - skip Length byte position }
      WriteLn('    add x0, x0, #1');
      { Save x2 (len) For later Length byte Write }
      WriteLn('    mov x4, x2');
      lbl1 := NewLabel;
      lbl2 := NewLabel;
      EmitLabel(lbl1);
      { cbz x2, done }
      Write('    cbz x2, L'); WriteLn(lbl2);
      { ldr x5, [x3], #8 - load 8-byte Integer from tok_str }
      WriteLn('    ldr x5, [x3], #8');
      { strb w5, [x0], #1 - store as byte }
      WriteLn('    strb w5, [x0], #1');
      { sub x2, x2, #1 }
      WriteLn('    sub x2, x2, #1');
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { strb wzr, [x0] - null terminate }
      WriteLn('    strb wzr, [x0]');
      { Write Length byte at start Of filename: x0-x4-1 = start address }
      { sub x0, x0, x4 }
      WriteLn('    sub x0, x0, x4');
      { sub x0, x0, #1 }
      WriteLn('    sub x0, x0, #1');
      { strb w4, [x0] - store Length byte }
      WriteLn('    strb w4, [x0]');
      { Initialize fd To -1 }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      EmitMovX0(-1);
      EmitPopX1;
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      Expect(TOK_RPAREN)
    End
    { reset = 114,101,115,101,116 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 116) Then
    Begin
      { reset(f) - open file For reading }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get filename address (offset 16) And convert To C String }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;  { save file Var base }
      { add x0, x0, #16 - point To filename }
      WriteLn('    add x0, x0, #16');
      { Convert Pascal String To C String: skip Length byte }
      WriteLn('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_RDONLY(0), x2=mode(0) }
      WriteLn('    mov x1, #0');
      WriteLn('    mov x2, #0');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      WriteLn('    movz x16, #5');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Store fd In file variable (offset 0) }
      EmitPopX1;  { x1 = file Var base }
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      { Store mode=1 (Read) at offset 8 }
      WriteLn('    mov x0, #1');
      { str x0, [x1, #8] }
      WriteLn('    str x0, [x1, #8]');
    End
    { rewrite = 114,101,119,114,105,116,101 }
    Else If (tok_len = 7) And (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 119) And (ToLower(tok_str[3]) = 114) And (ToLower(tok_str[4]) = 105) And
            (ToLower(tok_str[5]) = 116) And (ToLower(tok_str[6]) = 101) Then
    Begin
      { rewrite(f) - open/create file For writing }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get filename address (offset 16) And convert To C String }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;  { save file Var base }
      { add x0, x0, #16 - point To filename }
      WriteLn('    add x0, x0, #16');
      { Convert Pascal String To C String: skip Length byte }
      WriteLn('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_WRONLY|O_CREAT|O_TRUNC(1537), x2=mode(420) }
      { mov x1, #1537 }
      WriteLn('    mov x1, #1537');
      { mov x2, #420 }
      WriteLn('    mov x2, #420');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      WriteLn('    movz x16, #5');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Store fd In file variable (offset 0) }
      EmitPopX1;  { x1 = file Var base }
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      { Store mode=2 (Write) at offset 8 }
      WriteLn('    mov x0, #2');
      { str x0, [x1, #8] }
      WriteLn('    str x0, [x1, #8]');
    End
    { close = 99,108,111,115,101 }
    Else If (tok_len = 5) And (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 108) And
            (ToLower(tok_str[2]) = 111) And (ToLower(tok_str[3]) = 115) And (ToLower(tok_str[4]) = 101) Then
    Begin
      { close(f) - close file }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Load fd from file variable }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      WriteLn('    ldr x0, [x0]');
      { close syscall: x0=fd }
      { movz x16, #6; movk x16, #0x200, lsl #16 = 0x2000006 }
      WriteLn('    movz x16, #6');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Set fd To -1 To mark as closed }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      EmitMovX0(-1);
      EmitPopX1;
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      { Set mode To 0 (closed) at offset 8 }
      WriteLn('    mov x0, #0');
      { str x0, [x1, #8] }
      WriteLn('    str x0, [x1, #8]');
    End
    { setinput = 115,101,116,105,110,112,117,116 }
    Else If (tok_len = 8) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 110) And
            (ToLower(tok_str[5]) = 112) And (ToLower(tok_str[6]) = 117) And (ToLower(tok_str[7]) = 116) Then
    Begin
      { setinput(f) - Set input file descriptor from Text file variable }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If sym_type[idx] <> TYPE_TEXT Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Load fd from file variable And store In x19 }
      EmitVarAddr(idx, scope_level);
      { ldr x19, [x0] }
      WriteLn('    ldr x19, [x0]');
    End
    { setoutput = 115,101,116,111,117,116,112,117,116 }
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 111) And (ToLower(tok_str[4]) = 117) And
            (ToLower(tok_str[5]) = 116) And (ToLower(tok_str[6]) = 112) And (ToLower(tok_str[7]) = 117) And
            (ToLower(tok_str[8]) = 116) Then
    Begin
      { setoutput(f) - Set output file descriptor from Text file variable }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If sym_type[idx] <> TYPE_TEXT Then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Load fd from file variable And store In x20 }
      EmitVarAddr(idx, scope_level);
      { ldr x20, [x0] }
      WriteLn('    ldr x20, [x0]');
    End
    { setinputfd = 115,101,116,105,110,112,117,116,102,100 (10 chars) }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 105) And (ToLower(tok_str[4]) = 110) And
            (ToLower(tok_str[5]) = 112) And (ToLower(tok_str[6]) = 117) And (ToLower(tok_str[7]) = 116) And
            (ToLower(tok_str[8]) = 102) And (ToLower(tok_str[9]) = 100) Then
    Begin
      { setinputfd(fd) - Set input file descriptor directly from Integer }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { mov x19, x0 }
      WriteLn('    mov x19, x0');
    End
    { setoutputfd = 115,101,116,111,117,116,112,117,116,102,100 (11 chars) }
    Else If (tok_len = 11) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 111) And (ToLower(tok_str[4]) = 117) And
            (ToLower(tok_str[5]) = 116) And (ToLower(tok_str[6]) = 112) And (ToLower(tok_str[7]) = 117) And
            (ToLower(tok_str[8]) = 116) And (ToLower(tok_str[9]) = 102) And (ToLower(tok_str[10]) = 100) Then
    Begin
      { setoutputfd(fd) - Set output file descriptor directly from Integer }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { mov x20, x0 }
      WriteLn('    mov x20, x0');
    End
    { closefd = 99,108,111,115,101,102,100 (7 chars) }
    Else If (tok_len = 7) And (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 108) And
            (ToLower(tok_str[2]) = 111) And (ToLower(tok_str[3]) = 115) And (ToLower(tok_str[4]) = 101) And
            (ToLower(tok_str[5]) = 102) And (ToLower(tok_str[6]) = 100) Then
    Begin
      { closefd(fd) - close a file descriptor }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = fd To close }
      { close syscall: x16 = 0x2000006 }
      WriteLn('    movz x16, #6');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc
    End
    { writefd = 119,114,105,116,101,102,100 (7 chars) }
    Else If (tok_len = 7) And (ToLower(tok_str[0]) = 119) And (ToLower(tok_str[1]) = 114) And
            (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 101) And
            (ToLower(tok_str[5]) = 102) And (ToLower(tok_str[6]) = 100) Then
    Begin
      { writefd(fd, Char) - Write one Char To fd }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { fd }
      EmitPushX0;
      Expect(TOK_COMMA);
      ParseExpression;  { Char }
      { x0 = Char, stack top = fd }
      { sub sp, sp, #16; strb w0, [sp] - store Char In buffer }
      WriteLn('    sub sp, sp, #16');
      { strb w0, [sp] }
      WriteLn('    strb w0, [sp]');
      { ldr x0, [sp, #16] - load fd from saved position }
      WriteLn('    ldr x0, [sp, #16]');
      { mov x1, sp - buffer address }
      WriteLn('    mov x1, sp');
      { mov x2, #1 - Write 1 byte }
      WriteLn('    mov x2, #1');
      { Write syscall: x16 = 0x2000004 }
      WriteLn('    movz x16, #4');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { add sp, sp, #32 - restore stack (16 For buffer + 16 For saved fd) }
      WriteLn('    add sp, sp, #32');
      Expect(TOK_RPAREN)
    End
    { seek = 115,101,101,107 }
    Else If (tok_len = 4) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 101) And (ToLower(tok_str[3]) = 107) Then
    Begin
      { seek(f, pos) - move To position In file }
      NextToken;
      Expect(TOK_LPAREN);
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If (sym_type[idx] <> TYPE_FILE) And (sym_type[idx] <> TYPE_TEXT) Then
        Error(9);
      NextToken;
      Expect(TOK_COMMA);
      { Get file fd And save To x23 }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      WriteLn('    ldr x0, [x0]');
      { Save fd To x23 }
      WriteLn('    mov x23, x0');
      { Parse position expression }
      ParseExpression;
      { x0 = position, x23 = fd }
      { mov x1, x0 (position To x1) }
      WriteLn('    mov x1, x0');
      { mov x0, x23 (fd To x0) }
      WriteLn('    mov x0, x23');
      { mov x2, #0 (SEEK_SET) }
      WriteLn('    mov x2, #0');
      { lseek syscall: 0x20000C7 }
      WriteLn('    movz x16, #0xC7');
      WriteLn('    movk x16, #0x200, lsl #16');
      EmitSvc;
      Expect(TOK_RPAREN)
    End
    { delete = 100,101,108,101,116,101 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 100) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 108) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 116) And
            (ToLower(tok_str[5]) = 101) Then
    Begin
      { delete(s, start, count) - remove chars from String }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: String variable }
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If sym_type[idx] <> TYPE_STRING Then
        Error(9);
      NextToken;
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      Expect(TOK_COMMA);
      { Second arg: start position }
      ParseExpression;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Third arg: count }
      ParseExpression;
      { x0=count, stack has String addr And start }
      WriteLn('    mov x2, x0');
      EmitPopX1;  { start }
      EmitPopX0;  { String addr }
      EmitBL(rt_str_delete);
      Expect(TOK_RPAREN)
    End
    { insert = 105,110,115,101,114,116 }
    Else If (tok_len = 6) And (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 110) And
            (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 114) And
            (ToLower(tok_str[5]) = 116) Then
    Begin
      { insert(source, dest, pos) - insert source into dest at pos }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: source String }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);
        EmitPushX0
      End
      Else If tok_type = TOK_STRING Then
      Begin
        { String literal - allocate temp on heap }
        WriteLn('    mov x8, x21');
        EmitMovX0(tok_len);
        WriteLn('    strb w0, [x8]');
        For i := 0 To tok_len - 1 Do
        Begin
          EmitMovX0(tok_str[i]);
          Write('    strb w0, [x8, #'); Write(i + 1); WriteLn(']');
        End;
        WriteLn('    mov x0, x21');
        WriteLn('    add x21, x21, #256');
        EmitPushX0;
        NextToken
      End
      Else
        Error(9);
      Expect(TOK_COMMA);
      { Second arg: dest String variable }
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If sym_type[idx] <> TYPE_STRING Then
        Error(9);
      NextToken;
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      Expect(TOK_COMMA);
      { Third arg: position }
      ParseExpression;
      { x0=pos, stack has source And dest }
      WriteLn('    mov x2, x0');
      EmitPopX1;  { dest }
      EmitPopX0;  { source }
      EmitBL(rt_str_insert);
      Expect(TOK_RPAREN)
    End
    { str = 115,116,114 - but only If Not a local variable }
    Else If (tok_len = 3) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 116) And
            (ToLower(tok_str[2]) = 114) And (SymLookup < 0) Then
    Begin
      { str(n, s) - convert Integer n To String s }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: Integer expression }
      ParseExpression;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Second arg: String variable }
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      If sym_type[idx] <> TYPE_STRING Then
        Error(9);
      NextToken;
      EmitVarAddr(idx, scope_level);
      { x0 = String addr, pop Integer into x1 Then swap }
      WriteLn('    mov x1, x0');
      EmitPopX0;  { x0 = Integer value }
      { Now: x0 = Integer, x1 = String addr }
      EmitBL(rt_int_to_str);
      Expect(TOK_RPAREN)
    End
    { val = 118,97,108 - but only If Not a local variable }
    Else If (tok_len = 3) And (ToLower(tok_str[0]) = 118) And (ToLower(tok_str[1]) = 97) And
            (ToLower(tok_str[2]) = 108) And (SymLookup < 0) Then
    Begin
      { val(s, v, code) - convert String s To Integer v, error In code }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: String }
      If tok_type = TOK_IDENT Then
      Begin
        idx := SymLookup;
        If idx < 0 Then
          Error(3);
        If sym_type[idx] <> TYPE_STRING Then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      End
      Else If tok_type = TOK_STRING Then
      Begin
        { String literal - allocate temp on heap }
        WriteLn('    mov x8, x21');
        EmitMovX0(tok_len);
        WriteLn('    strb w0, [x8]');
        For i := 0 To tok_len - 1 Do
        Begin
          EmitMovX0(tok_str[i]);
          Write('    strb w0, [x8, #'); Write(i + 1); WriteLn(']');
        End;
        WriteLn('    mov x0, x21');
        WriteLn('    add x21, x21, #256');
        NextToken
      End
      Else
        Error(9);
      { Call rt_str_to_int: x0=String addr -> x0=value, x1=error }
      EmitBL(rt_str_to_int);
      { Save both results: push error first (x1), Then value (x0) }
      EmitPushX1;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Second arg: Integer variable To receive value }
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      NextToken;
      { Get address Of v into x0, move To x1, pop value, store }
      EmitVarAddr(idx, scope_level);
      WriteLn('    mov x1, x0');
      EmitPopX0;  { value }
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      Expect(TOK_COMMA);
      { Third arg: Integer variable To receive error code }
      If tok_type <> TOK_IDENT Then
        Error(6);
      idx := SymLookup;
      If idx < 0 Then
        Error(3);
      NextToken;
      { Get address Of code into x0, move To x1, pop error, store }
      EmitVarAddr(idx, scope_level);
      WriteLn('    mov x1, x0');
      EmitPopX0;  { error }
      { str x0, [x1] }
      WriteLn('    str x0, [x1]');
      Expect(TOK_RPAREN)
    End
    { clrscr = 99,108,114,115,99,114 }
    Else If TokIs8(99, 108, 114, 115, 99, 114, 0, 0) = 1 Then
    Begin
      NextToken;
      EmitBL(rt_clrscr)
    End
    { gotoxy = 103,111,116,111,120,121 }
    Else If TokIs8(103, 111, 116, 111, 120, 121, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;    { x }
      EmitPushX0;
      Expect(TOK_COMMA);
      ParseExpression;    { y }
      EmitPopX1;          { x1=x, x0=y }
      EmitBL(rt_gotoxy);
      Expect(TOK_RPAREN)
    End
    { clreol = 99,108,114,101,111,108 }
    Else If TokIs8(99, 108, 114, 101, 111, 108, 0, 0) = 1 Then
    Begin
      NextToken;
      EmitBL(rt_clreol)
    End
    { textcolor = 116,101,120,116,99,111,108,111,114 (9 chars) }
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 120) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 99) And
            (ToLower(tok_str[5]) = 111) And (ToLower(tok_str[6]) = 108) And (ToLower(tok_str[7]) = 111) And
            (ToLower(tok_str[8]) = 114) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;    { color code 0-7 }
      EmitBL(rt_textcolor);
      Expect(TOK_RPAREN)
    End
    { textbackground = 116,101,120,116,98,97,99,107,103,114,111,117,110,100 (14 chars) }
    Else If (tok_len = 14) And (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 101) And
            (ToLower(tok_str[2]) = 120) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 98) And
            (ToLower(tok_str[5]) = 97) And (ToLower(tok_str[6]) = 99) And (ToLower(tok_str[7]) = 107) And
            (ToLower(tok_str[8]) = 103) And (ToLower(tok_str[9]) = 114) And (ToLower(tok_str[10]) = 111) And
            (ToLower(tok_str[11]) = 117) And (ToLower(tok_str[12]) = 110) And (ToLower(tok_str[13]) = 100) Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;    { color code 0-7 }
      EmitBL(rt_textbackground);
      Expect(TOK_RPAREN)
    End
    { normvideo = 110,111,114,109,118,105,100,101,111 (9 chars) }
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 110) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 109) And (ToLower(tok_str[4]) = 118) And
            (ToLower(tok_str[5]) = 105) And (ToLower(tok_str[6]) = 100) And (ToLower(tok_str[7]) = 101) And
            (ToLower(tok_str[8]) = 111) Then
    Begin
      NextToken;
      EmitBL(rt_normvideo)
    End
    { highvideo = 104,105,103,104,118,105,100,101,111 (9 chars) }
    Else If (tok_len = 9) And (ToLower(tok_str[0]) = 104) And (ToLower(tok_str[1]) = 105) And
            (ToLower(tok_str[2]) = 103) And (ToLower(tok_str[3]) = 104) And (ToLower(tok_str[4]) = 118) And
            (ToLower(tok_str[5]) = 105) And (ToLower(tok_str[6]) = 100) And (ToLower(tok_str[7]) = 101) And
            (ToLower(tok_str[8]) = 111) Then
    Begin
      NextToken;
      EmitBL(rt_highvideo)
    End
    { lowvideo = 108,111,119,118,105,100,101,111 (8 chars) }
    Else If TokIs8(108, 111, 119, 118, 105, 100, 101, 111) = 1 Then
    Begin
      NextToken;
      EmitBL(rt_lowvideo)
    End
    { hidecursor = 104,105,100,101,99,117,114,115,111,114 (10 chars) }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 104) And (ToLower(tok_str[1]) = 105) And
            (ToLower(tok_str[2]) = 100) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 99) And
            (ToLower(tok_str[5]) = 117) And (ToLower(tok_str[6]) = 114) And (ToLower(tok_str[7]) = 115) And
            (ToLower(tok_str[8]) = 111) And (ToLower(tok_str[9]) = 114) Then
    Begin
      NextToken;
      EmitBL(rt_hidecursor)
    End
    { showcursor = 115,104,111,119,99,117,114,115,111,114 (10 chars) }
    Else If (tok_len = 10) And (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 104) And
            (ToLower(tok_str[2]) = 111) And (ToLower(tok_str[3]) = 119) And (ToLower(tok_str[4]) = 99) And
            (ToLower(tok_str[5]) = 117) And (ToLower(tok_str[6]) = 114) And (ToLower(tok_str[7]) = 115) And
            (ToLower(tok_str[8]) = 111) And (ToLower(tok_str[9]) = 114) Then
    Begin
      NextToken;
      EmitBL(rt_showcursor)
    End
    { sleep = 115,108,101,101,112 (5 chars) - Sleep(ms) }
    Else If TokIs8(115, 108, 101, 101, 112, 0, 0, 0) = 1 Then
    Begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { ms In x0 }
      EmitBL(rt_sleep);
      Expect(TOK_RPAREN)
    End
    { initkeyboard = 105,110,105,116,107,101,121,98,111,97,114,100 (12 chars) }
    Else If (tok_len = 12) And (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 110) And
            (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 116) And (ToLower(tok_str[4]) = 107) And
            (ToLower(tok_str[5]) = 101) And (ToLower(tok_str[6]) = 121) And (ToLower(tok_str[7]) = 98) And
            (ToLower(tok_str[8]) = 111) And (ToLower(tok_str[9]) = 97) And (ToLower(tok_str[10]) = 114) And
            (ToLower(tok_str[11]) = 100) Then
    Begin
      NextToken;
      EmitBL(rt_initkeyboard)
    End
    { donekeyboard = 100,111,110,101,107,101,121,98,111,97,114,100 (12 chars) }
    Else If (tok_len = 12) And (ToLower(tok_str[0]) = 100) And (ToLower(tok_str[1]) = 111) And
            (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 101) And (ToLower(tok_str[4]) = 107) And
            (ToLower(tok_str[5]) = 101) And (ToLower(tok_str[6]) = 121) And (ToLower(tok_str[7]) = 98) And
            (ToLower(tok_str[8]) = 111) And (ToLower(tok_str[9]) = 97) And (ToLower(tok_str[10]) = 114) And
            (ToLower(tok_str[11]) = 100) Then
    Begin
      NextToken;
      EmitBL(rt_donekeyboard)
    End
    Else
    Begin
      { Check For With context - try To find identifier as a field }
      If with_rec_idx >= 0 Then
      Begin
        arg_count := FindField(with_rec_type);  { reuse arg_count For field_idx }
        If arg_count >= 0 Then
        Begin
          { Found field - handle assignment }
          NextToken;
          Expect(TOK_ASSIGN);
          ParseExpression;  { value In x0 Or d0 }
          { Compute address: base + field_offset }
          If field_type[arg_count] = TYPE_REAL Then
          Begin
            { Value is In d0, need To store To field }
            If expr_type <> TYPE_REAL Then
              EmitScvtfD0X0;
            EmitPushD0;  { save value }
          End
          Else
            EmitPushX0;  { save value }
          { Compute address }
          If sym_level[with_rec_idx] < scope_level Then
          Begin
            EmitFollowChain(sym_level[with_rec_idx], scope_level);
            EmitAddrOffset(1, 8, sym_offset[with_rec_idx] + field_offset[arg_count])
          End
          Else
            EmitAddrOffset(1, 29, sym_offset[with_rec_idx] + field_offset[arg_count]);
          { Store value To field }
          If field_type[arg_count] = TYPE_REAL Then
          Begin
            EmitPopD0;
            { str d0, [x1] }
            WriteLn('    str d0, [x1]');
          End
          Else
          Begin
            EmitPopX0;
            { str x0, [x1] }
            WriteLn('    str x0, [x1]');
          End
        End
        Else
        Begin
          { Field Not found, proceed With normal lookup }
          idx := SymLookup;
          If idx < 0 Then
            Error(3)
        End
      End
      Else
      Begin
        { Not a built-In, look up In symbol table }
        idx := SymLookup;
        If idx < 0 Then
          Error(3)
      End;
      { Skip If we already handled With field assignment }
      If (with_rec_idx < 0) Or (arg_count < 0) Then
      Begin
      NextToken;

      If (sym_kind[idx] = SYM_PROCEDURE) Or
         ((sym_kind[idx] = SYM_FUNCTION) And (tok_type = TOK_LPAREN)) Then
      Begin
        { Procedure call - pass args In x0-x7 }
        arg_count := 0;
        var_flags := sym_var_param_flags[idx];
        If tok_type = TOK_LPAREN Then
        Begin
          NextToken;
          If tok_type <> TOK_RPAREN Then
          Begin
            { Evaluate all args And push To stack }
            Repeat
              If tok_type = TOK_COMMA Then NextToken;
              { Check If this is a Var parameter }
              If IsVarParam(var_flags, arg_count) = 1 Then
              Begin
                { Var param - pass address Of variable }
                If tok_type <> TOK_IDENT Then
                  Error(6);  { Var param requires variable }
                var_arg_idx := SymLookup;
                If var_arg_idx < 0 Then
                  Error(3);
                NextToken;
                { Check For Array element - pass address Of element }
                If (sym_type[var_arg_idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
                Begin
                  NextToken;  { consume '[' }
                  ParseExpression;  { first index In x0 }
                  { Subtract low bound for first dimension }
                  dim_lo := arr_info[var_arg_idx * 8];
                  EmitPushX0;
                  EmitMovX0(dim_lo);
                  EmitPopX1;
                  WriteLn('    sub x0, x1, x0');
                  { Handle multi-dimensional arrays }
                  dim_count := arr_dims[var_arg_idx];
                  If dim_count < 1 Then dim_count := 1;
                  dim_idx := 1;
                  While (dim_idx < dim_count) And (tok_type = TOK_COMMA) Do
                  Begin
                    NextToken;
                    dim_size := arr_info[var_arg_idx * 8 + dim_idx * 2 + 1];
                    EmitPushX0;
                    EmitMovX0(dim_size);
                    EmitPopX1;
                    WriteLn('    mul x0, x1, x0');
                    EmitPushX0;
                    ParseExpression;
                    dim_lo := arr_info[var_arg_idx * 8 + dim_idx * 2];
                    EmitPushX0;
                    EmitMovX0(dim_lo);
                    EmitPopX1;
                    WriteLn('    sub x0, x1, x0');
                    EmitPopX1;
                    WriteLn('    add x0, x1, x0');
                    dim_idx := dim_idx + 1
                  End;
                  Expect(TOK_RBRACKET);
                  { Multiply by element size }
                  If sym_var_param_flags[var_arg_idx] > 0 Then
                  Begin
                    lbl1 := sym_label[sym_var_param_flags[var_arg_idx] - 1];
                    EmitPushX0;
                    EmitMovX0(lbl1);
                    EmitPopX1;
                    WriteLn('    mul x0, x1, x0');
                  End
                  Else If sym_var_param_flags[var_arg_idx] = -1 Then
                    WriteLn('    lsl x0, x0, #8')
                  Else
                    WriteLn('    lsl x0, x0, #3');
                  { Get base address And subtract element offset }
                  If sym_level[var_arg_idx] < scope_level Then
                  Begin
                    EmitFollowChain(sym_level[var_arg_idx], scope_level);
                    EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                  End
                  Else
                    EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                  { Address = base - element_offset }
                  WriteLn('    sub x0, x1, x0');
                End
                Else
                  { Simple variable - emit address }
                  EmitVarAddr(var_arg_idx, scope_level)
              End
              Else
              Begin
                { Value param - evaluate expression }
                ParseExpression
              End;
              EmitPushX0;
              arg_count := arg_count + 1
            Until tok_type <> TOK_COMMA
          End;
          Expect(TOK_RPAREN)
        End;
        { Pop args from stack into registers In reverse order }
        For i := arg_count - 1 DownTo 0 Do
        Begin
          Write('    ldr x'); Write(i); WriteLn(', [sp], #16');
        End;
        { Set up static link For callee }
        EmitStaticLink(sym_level[idx], scope_level);
        { Check If calling imported Unit Procedure or external C function }
        If sym_unit_idx[idx] >= 0 Then
          EmitBLUnitProc(sym_unit_idx[idx], idx)
        Else If sym_is_external[idx] = 1 Then
          EmitBLExternal(idx)
        Else
          EmitBL(sym_label[idx])
      End
      Else If (sym_kind[idx] = SYM_VAR) Or (sym_kind[idx] = SYM_PARAM) Then
      Begin
        If (sym_type[idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
        Begin
          { Array element assignment: arr[i] := expr Or arr[i,j,...] := expr }
          NextToken;  { consume '[' }
          ParseExpression;  { first index In x0 }
          { Subtract low bound for first dimension }
          dim_lo := arr_info[idx * 8];
          EmitPushX0;
          EmitMovX0(dim_lo);
          EmitPopX1;
          WriteLn('    sub x0, x1, x0');
          { Handle multi-dimensional arrays }
          dim_count := arr_dims[idx];
          If dim_count < 1 Then dim_count := 1;
          dim_idx := 1;
          While (dim_idx < dim_count) And (tok_type = TOK_COMMA) Do
          Begin
            NextToken;
            dim_size := arr_info[idx * 8 + dim_idx * 2 + 1];
            EmitPushX0;
            EmitMovX0(dim_size);
            EmitPopX1;
            WriteLn('    mul x0, x1, x0');
            EmitPushX0;
            ParseExpression;
            dim_lo := arr_info[idx * 8 + dim_idx * 2];
            EmitPushX0;
            EmitMovX0(dim_lo);
            EmitPopX1;
            WriteLn('    sub x0, x1, x0');
            EmitPopX1;
            WriteLn('    add x0, x1, x0');
            dim_idx := dim_idx + 1
          End;
          Expect(TOK_RBRACKET);
          { Multiply by element size }
          If sym_var_param_flags[idx] > 0 Then
          Begin
            { Array Of records }
            lbl1 := sym_label[sym_var_param_flags[idx] - 1];
            EmitPushX0;
            EmitMovX0(lbl1);
            EmitPopX1;
            WriteLn('    mul x0, x1, x0');
          End
          Else If sym_var_param_flags[idx] = -1 Then
          Begin
            { Array Of strings - multiply by 256 }
            WriteLn('    lsl x0, x0, #8');
          End
          Else
          Begin
            WriteLn('    lsl x0, x0, #3');
          End;
          { Get base address }
          If sym_level[idx] < scope_level Then
          Begin
            EmitFollowChain(sym_level[idx], scope_level);
            EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
          End
          Else
            EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
          { x1 = base, x0 = offset, compute element address In x1 }
          WriteLn('    sub x1, x1, x0');
          { Check For field access }
          If (sym_var_param_flags[idx] > 0) And (tok_type = TOK_DOT) Then
          Begin
            NextToken;
            arg_count := FindField(sym_var_param_flags[idx] - 1);
            If arg_count < 0 Then
              Error(11);
            NextToken;
            { Add field offset }
            If field_offset[arg_count] > 0 Then
              EmitAddrOffset(1, 1, field_offset[arg_count]);
            EmitPushX1;  { save field address }
            Expect(TOK_ASSIGN);
            ParseExpression;
            If field_type[arg_count] = TYPE_REAL Then
            Begin
              If expr_type <> TYPE_REAL Then
                EmitScvtfD0X0;
              EmitPopX1;
              WriteLn('    str d0, [x1]');
            End
            Else
            Begin
              EmitPopX1;
              WriteLn('    str x0, [x1]');
            End
          End
          Else If sym_var_param_flags[idx] = -1 Then
          Begin
            { Array Of strings - use rt_str_copy }
            EmitPushX1;  { save dest address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { source String address In x0 }
            If expr_type = TYPE_CHAR Then
            Begin
              { Single character - convert to 1-char string on heap }
              WriteLn('    mov x3, x0');  { save char value }
              WriteLn('    mov x0, x21');  { x0 = heap pointer (string addr) }
              WriteLn('    mov x4, #1');
              WriteLn('    strb w4, [x0]');  { store length = 1 }
              WriteLn('    strb w3, [x0, #1]');  { store char }
              WriteLn('    add x21, x21, #256');  { advance heap }
              expr_type := TYPE_STRING
            End;
            { x0 = source, need To call rt_str_copy(dest, source) }
            WriteLn('    mov x1, x0');  { x1 = source }
            EmitPopX0;  { x0 = dest }
            EmitBL(rt_str_copy)
          End
          Else
          Begin
            { Basic Array assignment }
            EmitPushX1;  { save element address }
            Expect(TOK_ASSIGN);
            ParseExpression;
            EmitPopX1;
            WriteLn('    str x0, [x1]');
          End
        End
        Else If (sym_type[idx] = TYPE_RECORD) And (tok_type = TOK_DOT) Then
        Begin
          { Record field assignment: rec.field := value Or rec.field.subfield := value }
          NextToken;  { consume '.' }
          If tok_type <> TOK_IDENT Then
            Error(11);
          { Find the field }
          arg_count := FindField(sym_const_val[idx]);  { reuse arg_count For field_idx }
          If arg_count < 0 Then
            Error(15);  { undefined field }
          { Accumulate total offset For nested fields }
          lbl1 := field_offset[arg_count];  { total offset from base }
          NextToken;
          { Handle nested Record fields }
          While (field_type[arg_count] = TYPE_RECORD) And (tok_type = TOK_DOT) Do
          Begin
            NextToken;  { consume '.' }
            If tok_type <> TOK_IDENT Then
              Error(11);
            lbl2 := arg_count;  { save current field index }
            arg_count := FindField(field_rec_type[lbl2]);
            If arg_count < 0 Then
              Error(15);
            lbl1 := lbl1 + field_offset[arg_count];  { accumulate offset }
            NextToken
          End;
          Expect(TOK_ASSIGN);
          ParseExpression;  { value In x0 Or d0 }

          { Compute field address: base + total_offset }
          If field_type[arg_count] = TYPE_REAL Then
          Begin
            { Value is In d0, need To store To field }
            If expr_type <> TYPE_REAL Then
              EmitScvtfD0X0;
            EmitPushD0;  { save value }
            { Compute address }
            If sym_level[idx] < scope_level Then
            Begin
              EmitFollowChain(sym_level[idx], scope_level);
              EmitAddrOffset(0, 8, sym_offset[idx] + lbl1)
            End
            Else
              EmitAddrOffset(0, 29, sym_offset[idx] + lbl1);
            EmitPopD0;
            { str d0, [x0] }
            WriteLn('    str d0, [x0]');
          End
          Else
          Begin
            { Value is In x0 }
            EmitPushX0;  { save value }
            { Compute address }
            If sym_level[idx] < scope_level Then
            Begin
              EmitFollowChain(sym_level[idx], scope_level);
              EmitAddrOffset(1, 8, sym_offset[idx] + lbl1)
            End
            Else
              EmitAddrOffset(1, 29, sym_offset[idx] + lbl1);
            EmitPopX0;
            { str x0, [x1] }
            WriteLn('    str x0, [x1]');
          End
        End
        Else If (sym_type[idx] = TYPE_POINTER) And (tok_type = TOK_CARET) Then
        Begin
          { Pointer dereference assignment: p^ := value Or pp^^ := value Or p^.field := value }
          { Count consecutive ^ tokens }
          lbl1 := 0;  { deref count }
          While tok_type = TOK_CARET Do
          Begin
            lbl1 := lbl1 + 1;
            NextToken
          End;
          { Load pointer value }
          If sym_level[idx] < scope_level Then
            EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          Else
            EmitLdurX0(sym_offset[idx]);
          { Dereference (count-1) times To get target address }
          For i := 1 To lbl1 - 1 Do
          Begin
            WriteLn('    ldr x0, [x0]');
          End;
          { lbl2 = remaining pointer depth after all derefs }
          lbl2 := ptr_depth[idx] - lbl1;

          If (lbl2 = 0) And (ptr_ultimate_type[idx] = TYPE_RECORD) And (tok_type = TOK_DOT) Then
          Begin
            { Pointer To Record field assignment: p^.field := value Or p^.field.subfield := value }
            NextToken;  { consume '.' }
            { Find field In the Record Type }
            arg_count := FindField(ptr_ultimate_rec[idx]);
            If arg_count < 0 Then
              Error(11);  { unknown field }
            NextToken;  { consume field name }
            { Add field offset To pointer: x0 = x0 + offset }
            If field_offset[arg_count] > 0 Then
              EmitAddrOffset(0, 0, field_offset[arg_count]);
            { Handle nested Record fields }
            While (field_type[arg_count] = TYPE_RECORD) And (tok_type = TOK_DOT) Do
            Begin
              NextToken;  { consume '.' }
              If tok_type <> TOK_IDENT Then
                Error(11);
              var_arg_idx := arg_count;  { save current field index }
              arg_count := FindField(field_rec_type[var_arg_idx]);
              If arg_count < 0 Then
                Error(15);
              If field_offset[arg_count] > 0 Then
                EmitAddrOffset(0, 0, field_offset[arg_count]);
              NextToken
            End;
            EmitPushX0;  { save field address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value To store }
            If field_type[arg_count] = TYPE_REAL Then
            Begin
              If expr_type <> TYPE_REAL Then
                EmitScvtfD0X0;
              EmitPopX1;  { get address into x1 }
              { str d0, [x1] }
              WriteLn('    str d0, [x1]');
            End
            Else
            Begin
              EmitPopX1;  { get address into x1 }
              { str x0, [x1] }
              WriteLn('    str x0, [x1]');
            End
          End
          Else If (lbl2 = 0) And (ptr_ultimate_type[idx] = TYPE_ARRAY) And (tok_type = TOK_LBRACKET) Then
          Begin
            { Pointer To Array element assignment: pa^[i] := value }
            NextToken;  { consume '[' }
            EmitPushX0;  { save Array base address }
            ParseExpression;  { index In x0 }
            Expect(TOK_RBRACKET);
            { x0 = index, stack has base address }
            { Subtract low bound }
            arg_count := sym_label[idx];  { ptr_arr index }
            EmitPushX0;
            EmitMovX0(ptr_arr_lo[arg_count]);
            EmitPopX1;
            { x0 = x1 - x0 = index - low_bound }
            WriteLn('    sub x0, x1, x0');
            { Multiply by 8 using lsl #3 }
            WriteLn('    lsl x0, x0, #3');
            { Add To base: x0 = base + offset }
            EmitPopX1;  { x1 = base address }
            WriteLn('    add x8, x1, x0');
            { x8 now has element address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value To store }
            { Store value }
            If ptr_arr_elem[arg_count] = TYPE_REAL Then
            Begin
              If expr_type <> TYPE_REAL Then
                EmitScvtfD0X0;
              WriteLn('    str d0, [x8]');
            End
            Else
            Begin
              WriteLn('    str x0, [x8]');
            End
          End
          Else
          Begin
            { Simple pointer dereference assignment: p^ := value Or pp^^ := value }
            EmitPushX0;  { save address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value To store }
            { Determine target Type }
            If (lbl2 = 0) And (ptr_ultimate_type[idx] = TYPE_REAL) Then
            Begin
              { Value In d0, need To store through pointer }
              If expr_type <> TYPE_REAL Then
                EmitScvtfD0X0;
              EmitPopX1;  { get address into x1 }
              { str d0, [x1] }
              WriteLn('    str d0, [x1]');
            End
            Else
            Begin
              { Value In x0 }
              EmitPopX1;  { get address into x1 }
              { str x0, [x1] }
              WriteLn('    str x0, [x1]');
            End
          End
        End
        Else If sym_type[idx] = TYPE_STRING Then
        Begin
          { Check For String indexed assignment s[i] := Char }
          If tok_type = TOK_LBRACKET Then
          Begin
            NextToken;  { consume '[' }
            { Get String base address }
            EmitVarAddr(idx, scope_level);
            EmitPushX0;  { save base address }
            ParseExpression;  { index In x0 }
            Expect(TOK_RBRACKET);
            { Add index To base: address = base + index }
            EmitPopX1;  { x1 = base address }
            WriteLn('    add x8, x1, x0');
            { x8 now has the address To store To }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value To store In x0 }
            { strb w0, [x8] }
            WriteLn('    strb w0, [x8]');
          End
          Else
          Begin
          { String assignment }
          Expect(TOK_ASSIGN);
          { Parse String expression (literal, variable, copy, concat, Or + expressions) }
          ParseExpression;
          If expr_type = TYPE_CHAR Then
          Begin
            { Single character - convert to 1-char string on heap }
            WriteLn('    mov x3, x0');  { save char value }
            WriteLn('    mov x0, x21');  { x0 = heap pointer (string addr) }
            WriteLn('    mov x4, #1');
            WriteLn('    strb w4, [x0]');  { store length = 1 }
            WriteLn('    strb w3, [x0, #1]');  { store char }
            WriteLn('    add x21, x21, #256');  { advance heap }
            expr_type := TYPE_STRING
          End;
          If expr_type <> TYPE_STRING Then
            Error(12);  { expected String }
          { x0 = source String address, copy To dest }
          WriteLn('    mov x1, x0');
          { Get dest address into x0 }
          EmitVarAddr(idx, scope_level);
          { Call rt_str_copy(x0=dest, x1=source) }
          EmitBL(rt_str_copy)
          End  { End Of Else For String whole assignment }
        End
        Else If sym_type[idx] = TYPE_REAL Then
        Begin
          { Real variable assignment }
          Expect(TOK_ASSIGN);
          ParseExpression;
          { Convert Integer To float If needed }
          If expr_type <> TYPE_REAL Then
            EmitScvtfD0X0;
          { Check If this is a Var param - need To dereference address }
          If sym_is_var_param[idx] = 1 Then
          Begin
            { d0 has the value, need To store To address In Var param }
            EmitPushD0;  { save value }
            { Load the address stored In the Var param }
            If sym_level[idx] < scope_level Then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitLdurX0(sym_offset[idx]);
            { x0 now has the address, pop value To d0 }
            EmitPopD0;
            { Store d0 To [x0] }
            WriteLn('    str d0, [x0]');
          End
          Else
          Begin
            If sym_level[idx] < scope_level Then
              EmitSturD0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitSturD0(sym_offset[idx])
          End
        End
        Else
        Begin
          { Simple Integer assignment }
          Expect(TOK_ASSIGN);
          ParseExpression;
          { Check If this is a Var param - need To dereference address }
          If sym_is_var_param[idx] = 1 Then
          Begin
            { x0 has the value, need To store To address In Var param }
            EmitPushX0;  { save value }
            { Load the address stored In the Var param }
            If sym_level[idx] < scope_level Then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitLdurX0(sym_offset[idx]);
            { x0 now has the address, pop value To x1 }
            EmitPopX1;
            { Store x1 To [x0] }
            WriteLn('    str x1, [x0]');
          End
          Else
          Begin
            If sym_level[idx] < scope_level Then
              EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            Else
              EmitSturX0(sym_offset[idx])
          End
        End
      End
      Else If sym_kind[idx] = SYM_FUNCTION Then
      Begin
        { Function result assignment - store To result variable at -16 }
        Expect(TOK_ASSIGN);
        ParseExpression;
        { For Real functions, store In d0 }
        If sym_type[idx] = TYPE_REAL Then
        Begin
          If expr_type <> TYPE_REAL Then
            EmitScvtfD0X0;
          EmitSturD0(-16)
        End
        Else If sym_type[idx] = TYPE_STRING Then
        Begin
          { String Function - x0 has source String addr, copy To heap For return }
          { x0 = source, x1 = dest (heap), call str_copy }
          WriteLn('    mov x8, x0');
          WriteLn('    mov x0, x21');
          { Save heap addr as result }
          EmitSturX0(-16);
          { x8 = dest, x9 = source For str_copy }
          WriteLn('    mov x9, x8');
          WriteLn('    mov x8, x21');
          EmitBL(rt_str_copy);
          { Advance heap pointer }
          WriteLn('    add x21, x21, #256');
        End
        Else
          EmitSturX0(-16)
      End
      Else
        Error(7)
      End  { End Of If (with_rec_idx < 0) Or (arg_count < 0) }
    End  { End Of Else For non-builtin identifier }
  End  { End Of Else If tok_type = TOK_IDENT }
End;


