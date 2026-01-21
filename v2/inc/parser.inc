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
  var_flags, var_arg_idx: integer;
begin
  if tok_type = TOK_INTEGER then
  begin
    EmitMovX0(tok_int);
    expr_type := TYPE_INTEGER;
    NextToken
  end
  else if tok_type = TOK_FLOAT_LITERAL then
  begin
    { Construct float at runtime: int_part + frac_part/1000000 }
    { Load integer part and convert to float }
    EmitMovX0(tok_float_int);
    EmitScvtfD0X0;
    { Load fractional part and convert to float }
    EmitPushD0;
    EmitMovX0(tok_float_frac);
    EmitScvtfD0X0;
    { Divide by 1000000 }
    EmitPushD0;
    EmitMovX0(1000000);
    EmitScvtfD0X0;
    EmitPopD1;
    EmitFDiv;  { d0 = d1 / d0 = frac / 1000000 }
    { Add integer part }
    EmitPopD1;
    EmitFAdd;  { d0 = d1 + d0 = int + frac }
    expr_type := TYPE_REAL;
    NextToken
  end
  else if tok_type = TOK_TRUE then
  begin
    EmitMovX0(1);
    expr_type := TYPE_INTEGER;
    NextToken
  end
  else if tok_type = TOK_FALSE then
  begin
    EmitMovX0(0);
    expr_type := TYPE_INTEGER;
    NextToken
  end
  else if tok_type = TOK_LPAREN then
  begin
    NextToken;
    ParseExpression;
    Expect(TOK_RPAREN)
    { expr_type is already set by ParseExpression }
  end
  else if tok_type = TOK_NOT then
  begin
    NextToken;
    ParseFactor;
    EmitEorX0(1);
    expr_type := TYPE_INTEGER  { not always returns boolean/int }
  end
  else if tok_type = TOK_NIL then
  begin
    EmitMovX0(0);  { nil = 0 }
    expr_type := TYPE_POINTER;
    NextToken
  end
  else if tok_type = TOK_AT then
  begin
    NextToken;
    if tok_type <> TOK_IDENT then
      Error(6);  { expected identifier }
    idx := SymLookup;
    if idx < 0 then
      Error(3);  { undefined identifier }
    NextToken;
    EmitVarAddr(idx, scope_level);
    expr_type := TYPE_POINTER
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
      EmitBL(rt_readchar);
      expr_type := TYPE_INTEGER
    end
    { ord = 111,114,100 }
    else if TokIs8(111, 114, 100, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { ord() is identity for integers/chars }
      expr_type := TYPE_INTEGER
    end
    { chr = 99,104,114 }
    else if TokIs8(99, 104, 114, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { chr() is identity for integers/chars }
      expr_type := TYPE_INTEGER
    end
    else
    begin
      idx := SymLookup;
      if idx < 0 then
        Error(3);  { undefined identifier }
      NextToken;
      if sym_kind[idx] = SYM_CONST then
      begin
        EmitMovX0(sym_const_val[idx]);
        expr_type := TYPE_INTEGER
      end
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
        else if (sym_type[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
        begin
          { Record field access: rec.field }
          NextToken;  { consume '.' }
          if tok_type <> TOK_IDENT then
            Error(11);
          { Find the field in the record type }
          arg_count := FindField(sym_const_val[idx]);  { reuse arg_count for field_idx }
          if arg_count < 0 then
            Error(15);  { undefined field }
          { Compute address: base + field_offset }
          { Base is at [x29 + sym_offset] for local, field at positive offset from there }
          if sym_level[idx] < scope_level then
          begin
            EmitFollowChain(sym_level[idx], scope_level);
            { add x0, x8, #sym_offset + field_offset }
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(35);
            write(sym_offset[idx] + field_offset[arg_count]);
            EmitNL
          end
          else
          begin
            { add x0, x29, #sym_offset + field_offset }
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
            writechar(35);
            write(sym_offset[idx] + field_offset[arg_count]);
            EmitNL
          end;
          { Load from computed address }
          if field_type[arg_count] = TYPE_REAL then
          begin
            EmitIndent;
            writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
            writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL;
            expr_type := TYPE_REAL
          end
          else
          begin
            EmitIndent;
            writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL;
            expr_type := field_type[arg_count]
          end;
          NextToken
        end
        else
        begin
          { Load variable value - check for var param and type }
          if sym_type[idx] = TYPE_REAL then
          begin
            if sym_level[idx] < scope_level then
              EmitLdurD0Outer(sym_offset[idx], sym_level[idx], scope_level)
            else
              EmitLdurD0(sym_offset[idx]);
            { If var param, d0 contains address - dereference it }
            if sym_is_var_param[idx] = 1 then
            begin
              { fmov x0, d0; ldr d0, [x0] }
              EmitFmovX0D0;
              EmitIndent;
              writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
              writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
              writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
              EmitNL
            end;
            expr_type := TYPE_REAL
          end
          else
          begin
            if sym_level[idx] < scope_level then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            else
              EmitLdurX0(sym_offset[idx]);
            { If var param, x0 now contains address - dereference it }
            if sym_is_var_param[idx] = 1 then
            begin
              EmitIndent;
              writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
              EmitNL
            end;
            expr_type := sym_type[idx]
          end
        end;
        { Check for pointer dereference }
        if (sym_type[idx] = TYPE_POINTER) and (tok_type = TOK_CARET) then
        begin
          NextToken;
          if sym_const_val[idx] = TYPE_REAL then
          begin
            { ldr d0, [x0] }
            EmitIndent;
            writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
            writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL;
            expr_type := TYPE_REAL
          end
          else
          begin
            { ldr x0, [x0] }
            EmitIndent;
            writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL;
            expr_type := sym_const_val[idx]
          end
        end
      end
      else if sym_kind[idx] = SYM_FUNCTION then
      begin
        { Function call - pass args in x0-x7 }
        arg_count := 0;
        var_flags := sym_var_param_flags[idx];
        if tok_type = TOK_LPAREN then
        begin
          NextToken;
          if tok_type <> TOK_RPAREN then
          begin
            { Evaluate all args and push to stack }
            repeat
              if tok_type = TOK_COMMA then NextToken;
              { Check if this is a var parameter }
              if IsVarParam(var_flags, arg_count) = 1 then
              begin
                { Var param - pass address of variable }
                if tok_type <> TOK_IDENT then
                  Error(6);  { var param requires variable }
                var_arg_idx := SymLookup;
                if var_arg_idx < 0 then
                  Error(3);
                NextToken;
                { Check for array element - pass address of element }
                if (sym_type[var_arg_idx] = TYPE_ARRAY) and (tok_type = TOK_LBRACKET) then
                begin
                  NextToken;  { consume '[' }
                  ParseExpression;  { index in x0 }
                  Expect(TOK_RBRACKET);
                  { Compute element address }
                  EmitPushX0;
                  EmitMovX0(sym_const_val[var_arg_idx]);  { low bound }
                  EmitPopX1;
                  { x0 = x1 - x0 = index - low_bound }
                  EmitIndent;
                  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
                  writechar(120); writechar(48);  { x0 }
                  EmitNL;
                  { Multiply by 8 using lsl #3 }
                  EmitIndent;
                  writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(35); writechar(51);  { #3 }
                  EmitNL;
                  { Get base address and subtract element offset }
                  if sym_level[var_arg_idx] < scope_level then
                  begin
                    EmitFollowChain(sym_level[var_arg_idx], scope_level);
                    EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                  end
                  else
                    EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                  { Address = base - element_offset }
                  EmitIndent;
                  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
                  writechar(120); writechar(48);  { x0 }
                  EmitNL
                end
                else
                  { Simple variable - emit address }
                  EmitVarAddr(var_arg_idx, scope_level)
              end
              else
              begin
                { Value param - evaluate expression }
                ParseExpression
              end;
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
        EmitBL(sym_label[idx]);
        expr_type := sym_type[idx]  { function return type }
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
    if expr_type = TYPE_REAL then
      EmitFNeg
    else
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

procedure ParseTerm;
var
  op, left_type: integer;
begin
  ParseUnary;
  while (tok_type = TOK_STAR) or (tok_type = TOK_DIV) or (tok_type = TOK_MOD) or
        (tok_type = TOK_AND) or (tok_type = TOK_SLASH) do
  begin
    op := tok_type;
    left_type := expr_type;
    NextToken;
    { Push left operand appropriately }
    if left_type = TYPE_REAL then
      EmitPushD0
    else
      EmitPushX0;
    ParseUnary;
    { right operand is now in x0 or d0 depending on expr_type }

    if op = TOK_SLASH then
    begin
      { / always produces real - convert both operands to float }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;  { convert right to float }
      if left_type = TYPE_REAL then
        EmitPopD1
      else
      begin
        EmitPopX1;
        EmitScvtfD1X1  { convert left to float }
      end;
      EmitFDiv;
      expr_type := TYPE_REAL
    end
    else if (left_type = TYPE_REAL) or (expr_type = TYPE_REAL) then
    begin
      { Mixed or both real - use float ops }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;  { convert right to float }
      if left_type = TYPE_REAL then
        EmitPopD1
      else
      begin
        EmitPopX1;
        EmitScvtfD1X1  { convert left to float }
      end;
      if op = TOK_STAR then
        EmitFMul
      else if op = TOK_DIV then
      begin
        { div on floats - truncate result to integer }
        EmitFDiv;
        EmitFcvtzsX0D0;
        expr_type := TYPE_INTEGER
      end
      else if op = TOK_MOD then
        Error(13)  { mod not supported for reals }
      else { TOK_AND }
        Error(13);  { and not supported for reals }
      if (op = TOK_STAR) then
        expr_type := TYPE_REAL
    end
    else
    begin
      { Both integers - use integer ops }
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
        EmitAndX0X1;
      expr_type := TYPE_INTEGER
    end
  end
end;

procedure ParseSimpleExpr;
var
  op, left_type: integer;
begin
  ParseTerm;
  while (tok_type = TOK_PLUS) or (tok_type = TOK_MINUS) or (tok_type = TOK_OR) do
  begin
    op := tok_type;
    left_type := expr_type;
    NextToken;
    { Push left operand appropriately }
    if left_type = TYPE_REAL then
      EmitPushD0
    else
      EmitPushX0;
    ParseTerm;

    if (left_type = TYPE_REAL) or (expr_type = TYPE_REAL) then
    begin
      { Mixed or both real - use float ops }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;  { convert right to float }
      if left_type = TYPE_REAL then
        EmitPopD1
      else
      begin
        EmitPopX1;
        EmitScvtfD1X1  { convert left to float }
      end;
      if op = TOK_PLUS then
        EmitFAdd
      else if op = TOK_MINUS then
        EmitFSub
      else { TOK_OR }
        Error(13);  { or not supported for reals }
      expr_type := TYPE_REAL
    end
    else
    begin
      { Both integers }
      EmitPopX1;
      if op = TOK_PLUS then
        EmitAdd
      else if op = TOK_MINUS then
        EmitSub
      else { TOK_OR }
        EmitOrrX0X1;
      expr_type := TYPE_INTEGER
    end
  end
end;

procedure ParseExpression;
var
  op, cond, left_type: integer;
begin
  ParseSimpleExpr;
  if (tok_type = TOK_EQ) or (tok_type = TOK_NEQ) or (tok_type = TOK_LT) or
     (tok_type = TOK_LE) or (tok_type = TOK_GT) or (tok_type = TOK_GE) then
  begin
    op := tok_type;
    left_type := expr_type;
    NextToken;
    { Push left operand appropriately }
    if left_type = TYPE_REAL then
      EmitPushD0
    else
      EmitPushX0;
    ParseSimpleExpr;

    if (left_type = TYPE_REAL) or (expr_type = TYPE_REAL) then
    begin
      { Float comparison }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;  { convert right to float }
      if left_type = TYPE_REAL then
        EmitPopD1
      else
      begin
        EmitPopX1;
        EmitScvtfD1X1  { convert left to float }
      end;
      EmitFCmp
    end
    else
    begin
      { Integer comparison }
      EmitPopX1;
      EmitCmpX0X1
    end;
    if op = TOK_EQ then cond := 0
    else if op = TOK_NEQ then cond := 1
    else if op = TOK_LT then cond := 2
    else if op = TOK_LE then cond := 3
    else if op = TOK_GT then cond := 4
    else cond := 5;
    EmitCset(cond);
    expr_type := TYPE_INTEGER  { comparison result is always integer (boolean) }
  end
end;

procedure ParseStatement;
var
  idx, lbl1, lbl2, lbl3, arg_count, i: integer;
  var_flags, arg_idx, var_arg_idx: integer;
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
  else if tok_type = TOK_READ then
  begin
    { read(var) - reads an integer or real into a variable }
    NextToken;
    Expect(TOK_LPAREN);
    if tok_type <> TOK_IDENT then
      Error(6);
    idx := SymLookup;
    if idx < 0 then
      Error(3);
    NextToken;
    if sym_type[idx] = TYPE_REAL then
    begin
      { Call read_real runtime - result in d0 }
      EmitBL(rt_read_real);
      { Store result in variable }
      if sym_level[idx] = scope_level then
        EmitSturD0(sym_offset[idx])
      else
        EmitSturD0Outer(sym_offset[idx], sym_level[idx], scope_level)
    end
    else
    begin
      { Call read_int runtime }
      EmitBL(rt_read_int);
      { Store result in variable }
      if sym_level[idx] = scope_level then
        EmitSturX0(sym_offset[idx])
      else
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
    end;
    Expect(TOK_RPAREN)
  end
  else if tok_type = TOK_READLN then
  begin
    { readln(var) or readln - reads integer/real and skips to end of line }
    NextToken;
    if tok_type = TOK_LPAREN then
    begin
      NextToken;
      if tok_type <> TOK_RPAREN then
      begin
        if tok_type <> TOK_IDENT then
          Error(6);
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        NextToken;
        if sym_type[idx] = TYPE_REAL then
        begin
          { Call read_real runtime - result in d0 }
          EmitBL(rt_read_real);
          { Store result in variable }
          if sym_level[idx] = scope_level then
            EmitSturD0(sym_offset[idx])
          else
            EmitSturD0Outer(sym_offset[idx], sym_level[idx], scope_level)
        end
        else
        begin
          { Call read_int runtime }
          EmitBL(rt_read_int);
          { Store result in variable }
          if sym_level[idx] = scope_level then
            EmitSturX0(sym_offset[idx])
          else
            EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
        end
      end;
      Expect(TOK_RPAREN)
    end;
    { Skip to end of line }
    EmitBL(rt_skip_line)
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
              { Print string literal character by character }
              idx := 0;
              while idx < tok_len do
              begin
                EmitMovX0(tok_str[idx]);
                EmitBL(rt_print_char);
                idx := idx + 1
              end;
              NextToken
            end
            else if tok_type = TOK_IDENT then
            begin
              { Check if it's a string variable }
              idx := SymLookup;
              if (idx >= 0) and (sym_type[idx] = TYPE_STRING) then
              begin
                { String variable - compute address and call print_string }
                NextToken;
                EmitVarAddr(idx, scope_level);
                EmitBL(rt_print_string)
              end
              else
              begin
                { Not a string - parse as expression and print based on type }
                ParseExpression;
                if expr_type = TYPE_REAL then
                  EmitBL(rt_print_real)
                else
                  EmitBL(rt_print_int)
              end
            end
            else
            begin
              ParseExpression;
              if expr_type = TYPE_REAL then
                EmitBL(rt_print_real)
              else
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
              { Print string literal character by character }
              idx := 0;
              while idx < tok_len do
              begin
                EmitMovX0(tok_str[idx]);
                EmitBL(rt_print_char);
                idx := idx + 1
              end;
              NextToken
            end
            else if tok_type = TOK_IDENT then
            begin
              { Check if it's a string variable }
              idx := SymLookup;
              if (idx >= 0) and (sym_type[idx] = TYPE_STRING) then
              begin
                { String variable - compute address and call print_string }
                NextToken;
                EmitVarAddr(idx, scope_level);
                EmitBL(rt_print_string)
              end
              else
              begin
                { Not a string - parse as expression and print based on type }
                ParseExpression;
                if expr_type = TYPE_REAL then
                  EmitBL(rt_print_real)
                else
                  EmitBL(rt_print_int)
              end
            end
            else
            begin
              ParseExpression;
              if expr_type = TYPE_REAL then
                EmitBL(rt_print_real)
              else
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
        var_flags := sym_var_param_flags[idx];
        if tok_type = TOK_LPAREN then
        begin
          NextToken;
          if tok_type <> TOK_RPAREN then
          begin
            { Evaluate all args and push to stack }
            repeat
              if tok_type = TOK_COMMA then NextToken;
              { Check if this is a var parameter }
              if IsVarParam(var_flags, arg_count) = 1 then
              begin
                { Var param - pass address of variable }
                if tok_type <> TOK_IDENT then
                  Error(6);  { var param requires variable }
                var_arg_idx := SymLookup;
                if var_arg_idx < 0 then
                  Error(3);
                NextToken;
                { Check for array element - pass address of element }
                if (sym_type[var_arg_idx] = TYPE_ARRAY) and (tok_type = TOK_LBRACKET) then
                begin
                  NextToken;  { consume '[' }
                  ParseExpression;  { index in x0 }
                  Expect(TOK_RBRACKET);
                  { Compute element address }
                  EmitPushX0;
                  EmitMovX0(sym_const_val[var_arg_idx]);  { low bound }
                  EmitPopX1;
                  { x0 = x1 - x0 = index - low_bound }
                  EmitIndent;
                  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
                  writechar(120); writechar(48);  { x0 }
                  EmitNL;
                  { Multiply by 8 using lsl #3 }
                  EmitIndent;
                  writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(35); writechar(51);  { #3 }
                  EmitNL;
                  { Get base address and subtract element offset }
                  if sym_level[var_arg_idx] < scope_level then
                  begin
                    EmitFollowChain(sym_level[var_arg_idx], scope_level);
                    EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                  end
                  else
                    EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                  { Address = base - element_offset }
                  EmitIndent;
                  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
                  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
                  writechar(120); writechar(48);  { x0 }
                  EmitNL
                end
                else
                  { Simple variable - emit address }
                  EmitVarAddr(var_arg_idx, scope_level)
              end
              else
              begin
                { Value param - evaluate expression }
                ParseExpression
              end;
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
        else if (sym_type[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
        begin
          { Record field assignment: rec.field := value }
          NextToken;  { consume '.' }
          if tok_type <> TOK_IDENT then
            Error(11);
          { Find the field }
          arg_count := FindField(sym_const_val[idx]);  { reuse arg_count for field_idx }
          if arg_count < 0 then
            Error(15);  { undefined field }
          NextToken;
          Expect(TOK_ASSIGN);
          ParseExpression;  { value in x0 or d0 }

          { Compute field address: base + field_offset }
          if field_type[arg_count] = TYPE_REAL then
          begin
            { Value is in d0, need to store to field }
            if expr_type <> TYPE_REAL then
              EmitScvtfD0X0;
            EmitPushD0;  { save value }
            { Compute address }
            if sym_level[idx] < scope_level then
            begin
              EmitFollowChain(sym_level[idx], scope_level);
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
              writechar(35);
              write(sym_offset[idx] + field_offset[arg_count]);
              EmitNL
            end
            else
            begin
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
              writechar(35);
              write(sym_offset[idx] + field_offset[arg_count]);
              EmitNL
            end;
            EmitPopD0;
            { str d0, [x0] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL
          end
          else
          begin
            { Value is in x0 }
            EmitPushX0;  { save value }
            { Compute address }
            if sym_level[idx] < scope_level then
            begin
              EmitFollowChain(sym_level[idx], scope_level);
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
              writechar(35);
              write(sym_offset[idx] + field_offset[arg_count]);
              EmitNL
            end
            else
            begin
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
              writechar(35);
              write(sym_offset[idx] + field_offset[arg_count]);
              EmitNL
            end;
            EmitPopX0;
            { str x0, [x1] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
        end
        else if (sym_type[idx] = TYPE_POINTER) and (tok_type = TOK_CARET) then
        begin
          { Pointer dereference assignment: p^ := value }
          NextToken;  { consume ^ }
          Expect(TOK_ASSIGN);
          { Load pointer value (address) and save it }
          if sym_level[idx] < scope_level then
            EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          else
            EmitLdurX0(sym_offset[idx]);
          EmitPushX0;  { save address }
          ParseExpression;  { value to store }
          if sym_const_val[idx] = TYPE_REAL then
          begin
            { Value in d0, need to store through pointer }
            if expr_type <> TYPE_REAL then
              EmitScvtfD0X0;
            EmitPopX1;  { get address into x1 }
            { str d0, [x1] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
          else
          begin
            { Value in x0 }
            EmitPopX1;  { get address into x1 }
            { str x0, [x1] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
        end
        else if sym_type[idx] = TYPE_STRING then
        begin
          { String assignment }
          Expect(TOK_ASSIGN);
          if tok_type <> TOK_STRING then
            Error(12);  { expected string literal }
          { Compute base address of string variable into x8 }
          if sym_level[idx] < scope_level then
          begin
            EmitFollowChain(sym_level[idx], scope_level);
            { add x8, x8, #offset }
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(35);
            write(sym_offset[idx]);
            EmitNL
          end
          else
          begin
            { add x8, x29, #offset }
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
            writechar(35);
            write(sym_offset[idx]);
            EmitNL
          end;
          { Store length at [x8] }
          EmitMovX0(tok_len);
          EmitIndent;
          writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
          writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
          writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
          EmitNL;
          { Store each character at [x8+1], [x8+2], etc }
          for i := 0 to tok_len - 1 do
          begin
            EmitMovX0(tok_str[i]);
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
            writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
            writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
            writechar(35);
            write(i + 1);
            writechar(93);  { ] }
            EmitNL
          end;
          NextToken
        end
        else if sym_type[idx] = TYPE_REAL then
        begin
          { Real variable assignment }
          Expect(TOK_ASSIGN);
          ParseExpression;
          { Convert integer to float if needed }
          if expr_type <> TYPE_REAL then
            EmitScvtfD0X0;
          { Check if this is a var param - need to dereference address }
          if sym_is_var_param[idx] = 1 then
          begin
            { d0 has the value, need to store to address in var param }
            EmitPushD0;  { save value }
            { Load the address stored in the var param }
            if sym_level[idx] < scope_level then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            else
              EmitLdurX0(sym_offset[idx]);
            { x0 now has the address, pop value to d0 }
            EmitPopD0;
            { Store d0 to [x0] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL
          end
          else
          begin
            if sym_level[idx] < scope_level then
              EmitSturD0Outer(sym_offset[idx], sym_level[idx], scope_level)
            else
              EmitSturD0(sym_offset[idx])
          end
        end
        else
        begin
          { Simple integer assignment }
          Expect(TOK_ASSIGN);
          ParseExpression;
          { Check if this is a var param - need to dereference address }
          if sym_is_var_param[idx] = 1 then
          begin
            { x0 has the value, need to store to address in var param }
            EmitPushX0;  { save value }
            { Load the address stored in the var param }
            if sym_level[idx] < scope_level then
              EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            else
              EmitLdurX0(sym_offset[idx]);
            { x0 now has the address, pop value to x1 }
            EmitPopX1;
            { Store x1 to [x0] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
            EmitNL
          end
          else
          begin
            if sym_level[idx] < scope_level then
              EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
            else
              EmitSturX0(sym_offset[idx])
          end
        end
      end
      else if sym_kind[idx] = SYM_FUNCTION then
      begin
        { Function result assignment - store to result variable at -16 }
        Expect(TOK_ASSIGN);
        ParseExpression;
        { For real functions, store in d0 }
        if sym_type[idx] = TYPE_REAL then
        begin
          if expr_type <> TYPE_REAL then
            EmitScvtfD0X0;
          EmitSturD0(-16)
        end
        else
          EmitSturX0(-16)
      end
      else
        Error(7)
    end  { end of else for non-builtin identifier }
  end  { end of else if tok_type = TOK_IDENT }
end;

