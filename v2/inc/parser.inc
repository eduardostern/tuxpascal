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
  idx, arg_count, i, lbl1: integer;
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
  else if tok_type = TOK_STRING then
  begin
    { String literal in expression }
    if tok_len = 1 then
    begin
      { Single character - treat as char/integer }
      EmitMovX0(tok_str[0]);
      expr_type := TYPE_CHAR
    end
    else
    begin
      { Multi-char string - allocate temp from heap and store string }
      { mov x8, x21 ; store string base to x8 }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
      writechar(120); writechar(50); writechar(49);  { x21 }
      EmitNL;
      { Store length byte }
      EmitMovX0(tok_len);
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
      writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
      writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
      EmitNL;
      { Store each character }
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
      { mov x0, x21 ; return string address }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(50); writechar(49);  { x21 }
      EmitNL;
      { add x21, x21, #256 ; advance heap pointer }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
      EmitNL;
      expr_type := TYPE_STRING
    end;
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
    if (sym_type[idx] = TYPE_ARRAY) and (tok_type = TOK_LBRACKET) then
    begin
      { Address of array element: @arr[index] }
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
      { Get base address }
      if sym_level[idx] < scope_level then
      begin
        EmitFollowChain(sym_level[idx], scope_level);
        EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
      end
      else
        EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
      { Address = base - element_offset }
      EmitIndent;
      writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(120); writechar(48);  { x0 }
      EmitNL
    end
    else
      EmitVarAddr(idx, scope_level);
    expr_type := TYPE_POINTER;
    ptr_base_type := sym_type[idx]
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
    { abs = 97,98,115 }
    else if TokIs8(97, 98, 115, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { If x < 0, negate it }
      { cmp x0, #0; b.ge skip; neg x0, x0; skip: }
      EmitIndent;
      writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(48);  { #0 }
      EmitNL;
      EmitIndent;
      writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
      writechar(76); write(label_count);
      EmitNL;
      EmitIndent;
      writechar(110); writechar(101); writechar(103); writechar(32);  { neg }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_INTEGER
    end
    { odd = 111,100,100 }
    else if TokIs8(111, 100, 100, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x AND 1 }
      EmitIndent;
      writechar(97); writechar(110); writechar(100); writechar(32);  { and }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      expr_type := TYPE_BOOLEAN
    end
    { sqr = 115,113,114 }
    else if TokIs8(115, 113, 114, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x * x }
      EmitIndent;
      writechar(109); writechar(117); writechar(108); writechar(32);  { mul }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      expr_type := TYPE_INTEGER
    end
    { succ = 115,117,99,99 }
    else if TokIs8(115, 117, 99, 99, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x + 1 }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      expr_type := TYPE_INTEGER
    end
    { pred = 112,114,101,100 }
    else if TokIs8(112, 114, 101, 100, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x - 1 }
      EmitIndent;
      writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      expr_type := TYPE_INTEGER
    end
    { upcase = 117,112,99,97,115,101 }
    else if (tok_len = 6) and (tok_str[0] = 117) and (tok_str[1] = 112) and
            (tok_str[2] = 99) and (tok_str[3] = 97) and (tok_str[4] = 115) and
            (tok_str[5] = 101) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { If char is 'a'-'z' (97-122), subtract 32 }
      { cmp x0, #97; b.lt skip; cmp x0, #122; b.gt skip; sub x0, x0, #32; skip: }
      EmitIndent;
      writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(57); writechar(55);  { #97 }
      EmitNL;
      EmitIndent;
      writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
      writechar(76); write(label_count);
      EmitNL;
      EmitIndent;
      writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(49); writechar(50); writechar(50);  { #122 }
      EmitNL;
      EmitIndent;
      writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
      writechar(76); write(label_count);
      EmitNL;
      EmitIndent;
      writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(51); writechar(50);  { #32 }
      EmitNL;
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_CHAR
    end
    { lowercase = 108,111,119,101,114,99,97,115,101 }
    else if (tok_len = 9) and (tok_str[0] = 108) and (tok_str[1] = 111) and
            (tok_str[2] = 119) and (tok_str[3] = 101) and (tok_str[4] = 114) and
            (tok_str[5] = 99) and (tok_str[6] = 97) and (tok_str[7] = 115) and
            (tok_str[8] = 101) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { If char is 'A'-'Z' (65-90), add 32 }
      { cmp x0, #65; b.lt skip; cmp x0, #90; b.gt skip; add x0, x0, #32; skip: }
      EmitIndent;
      writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(54); writechar(53);  { #65 }
      EmitNL;
      EmitIndent;
      writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
      writechar(76); write(label_count);
      EmitNL;
      EmitIndent;
      writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(57); writechar(48);  { #90 }
      EmitNL;
      EmitIndent;
      writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
      writechar(76); write(label_count);
      EmitNL;
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(51); writechar(50);  { #32 }
      EmitNL;
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_CHAR
    end
    { length = 108,101,110,103,116,104 }
    else if (tok_len = 6) and (tok_str[0] = 108) and (tok_str[1] = 101) and
            (tok_str[2] = 110) and (tok_str[3] = 103) and (tok_str[4] = 116) and
            (tok_str[5] = 104) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type = TOK_STRING then
      begin
        { String literal - emit constant length }
        EmitMovX0(tok_len);
        NextToken
      end
      else if tok_type = TOK_IDENT then
      begin
        { Must be a string variable }
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        { Load string address }
        EmitVarAddr(idx, scope_level);
        { Load length byte from [x0] }
        EmitIndent;
        writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
        writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
        writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
        EmitNL
      end
      else
        Error(9);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    end
    { copy = 99,111,112,121 }
    else if (tok_len = 4) and (tok_str[0] = 99) and (tok_str[1] = 111) and
            (tok_str[2] = 112) and (tok_str[3] = 121) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: source string }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { source addr in x0 }
        EmitPushX0
      end
      else
        Error(9);
      Expect(TOK_COMMA);
      { Second arg: start index }
      ParseExpression;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Third arg: count }
      ParseExpression;  { count in x0 }
      { x0 = count, x1 = start, x2 = source addr }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
      writechar(120); writechar(48);  { x0 - count }
      EmitNL;
      EmitPopX1;  { start }
      EmitPopX0;  { source addr -> x2 }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
      writechar(120); writechar(48);  { x0 - source }
      EmitNL;
      { Allocate temp string from heap: x0 = x21, x21 += 256 }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(50); writechar(49);  { x21 }
      EmitNL;
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
      EmitNL;
      { Save dest addr }
      EmitPushX0;
      { Inline copy logic: copy from source[start] to dest, count bytes }
      { x0 = dest, x1 = start, x2 = source, x3 = count }
      { Store count as length at dest[0] }
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
      writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
      writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
      EmitNL;
      { Loop: copy count bytes from source[start+i] to dest[1+i] }
      lbl1 := NewLabel;
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
      writechar(35); writechar(48);  { #0 - index }
      EmitNL;
      EmitLabel(lbl1);
      { cmp x4, x3 }
      EmitIndent;
      writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
      writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
      writechar(120); writechar(51);  { x3 }
      EmitNL;
      { b.ge done }
      EmitIndent;
      writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
      writechar(76); write(label_count);
      EmitNL;
      { x5 = start + x4 (source index) }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(120); writechar(52);  { x4 }
      EmitNL;
      { ldrb w6, [x2, x5] }
      EmitIndent;
      writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
      writechar(119); writechar(54); writechar(44); writechar(32);  { w6, }
      writechar(91); writechar(120); writechar(50); writechar(44); writechar(32);  { [x2, }
      writechar(120); writechar(53); writechar(93);  { x5] }
      EmitNL;
      { x5 = x4 + 1 (dest index) }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
      writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      { strb w6, [x0, x5] }
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
      writechar(119); writechar(54); writechar(44); writechar(32);  { w6, }
      writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
      writechar(120); writechar(53); writechar(93);  { x5] }
      EmitNL;
      { x4 = x4 + 1 }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
      writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      EmitBranchLabel(lbl1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      { Restore dest addr to x0 }
      EmitPopX0;
      Expect(TOK_RPAREN);
      expr_type := TYPE_STRING
    end
    { concat = 99,111,110,99,97,116 }
    else if (tok_len = 6) and (tok_str[0] = 99) and (tok_str[1] = 111) and
            (tok_str[2] = 110) and (tok_str[3] = 99) and (tok_str[4] = 97) and
            (tok_str[5] = 116) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      { First string }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { string1 addr in x0 }
        EmitPushX0
      end
      else
        Error(9);
      Expect(TOK_COMMA);
      { Second string }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { string2 addr in x0 }
        { x0 = string2, stack top = string1 }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
        writechar(120); writechar(48);  { x0 }
        EmitNL;
        EmitPopX1;  { string1 }
      end
      else
        Error(9);
      { Allocate temp string from heap: x0 = x21, x21 += 256 }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(50); writechar(49);  { x21 }
      EmitNL;
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
      EmitNL;
      { Call rt_str_concat(x0=dest, x1=str1, x2=str2) }
      EmitBL(rt_str_concat);
      Expect(TOK_RPAREN);
      expr_type := TYPE_STRING
    end
    { trim = 116,114,105,109 }
    else if (tok_len = 4) and (tok_str[0] = 116) and (tok_str[1] = 114) and
            (tok_str[2] = 105) and (tok_str[3] = 109) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      end
      else
        Error(9);
      Expect(TOK_RPAREN);
      EmitBL(rt_str_trim);
      expr_type := TYPE_STRING
    end
    { ltrim = 108,116,114,105,109 }
    else if (tok_len = 5) and (tok_str[0] = 108) and (tok_str[1] = 116) and
            (tok_str[2] = 114) and (tok_str[3] = 105) and (tok_str[4] = 109) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      end
      else
        Error(9);
      Expect(TOK_RPAREN);
      EmitBL(rt_str_ltrim);
      expr_type := TYPE_STRING
    end
    { rtrim = 114,116,114,105,109 }
    else if (tok_len = 5) and (tok_str[0] = 114) and (tok_str[1] = 116) and
            (tok_str[2] = 114) and (tok_str[3] = 105) and (tok_str[4] = 109) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      end
      else
        Error(9);
      Expect(TOK_RPAREN);
      EmitBL(rt_str_rtrim);
      expr_type := TYPE_STRING
    end
    { pos = 112,111,115 }
    else if (tok_len = 3) and (tok_str[0] = 112) and (tok_str[1] = 111) and
            (tok_str[2] = 115) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: substring }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);
        EmitPushX0
      end
      else if tok_type = TOK_STRING then
      begin
        { String literal - allocate temp on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
        writechar(120); writechar(50); writechar(49);  { x21 }
        EmitNL;
        EmitMovX0(tok_len);
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
        writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
        EmitNL;
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
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(50); writechar(49);  { x21 }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        EmitPushX0;
        NextToken
      end
      else
        Error(9);
      Expect(TOK_COMMA);
      { Second arg: string to search in }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);  { string addr in x0 }
        { mov x1, x0; pop x0 }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
        writechar(120); writechar(48);  { x0 }
        EmitNL;
        EmitPopX0  { substr in x0 }
      end
      else
        Error(9);
      { Call rt_str_pos(x0=substr, x1=string) }
      EmitBL(rt_str_pos);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    end
    else
    begin
      { Check for WITH context - try to find identifier as a field }
      if with_rec_idx >= 0 then
      begin
        arg_count := FindField(with_rec_type);  { reuse arg_count for field_idx }
        if arg_count >= 0 then
        begin
          { Found field - generate access code }
          NextToken;
          { Compute address: base + field_offset }
          if sym_level[with_rec_idx] < scope_level then
          begin
            EmitFollowChain(sym_level[with_rec_idx], scope_level);
            { add x0, x8, #sym_offset + field_offset }
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(35);
            write(sym_offset[with_rec_idx] + field_offset[arg_count]);
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
            write(sym_offset[with_rec_idx] + field_offset[arg_count]);
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
          end
        end
        else
        begin
          { Field not found, proceed with normal lookup }
          idx := SymLookup;
          if idx < 0 then
            Error(3);  { undefined identifier }
          { Continue with the normal identifier handling below }
        end
      end
      else
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);  { undefined identifier }
      end;
      { Skip if we already handled WITH field }
      if (with_rec_idx < 0) or (arg_count < 0) then
      begin
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
          { Array element access: arr[index] or arr[index].field }
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
          { Check if array of records }
          if sym_var_param_flags[idx] > 0 then
          begin
            { Array of records - multiply by record size }
            lbl1 := sym_label[sym_var_param_flags[idx] - 1];  { record size }
            EmitPushX0;
            EmitMovX0(lbl1);
            EmitPopX1;
            { mul x0, x1, x0 }
            EmitIndent;
            writechar(109); writechar(117); writechar(108); writechar(32);  { mul }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(120); writechar(48);  { x0 }
            EmitNL
          end
          else
          begin
            { Basic type - multiply by 8 using lsl #3 }
            EmitIndent;
            writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(35); writechar(51);  { #3 }
            EmitNL
          end;
          { Get base address: frame + offset (offset is negative) }
          if sym_level[idx] < scope_level then
          begin
            EmitFollowChain(sym_level[idx], scope_level);
            EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
          end
          else
            EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
          { Compute element address: x1 - x0 }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(48);  { x0 }
          EmitNL;
          { Check for field access on array of records }
          if (sym_var_param_flags[idx] > 0) and (tok_type = TOK_DOT) then
          begin
            NextToken;  { consume '.' }
            arg_count := FindField(sym_var_param_flags[idx] - 1);
            if arg_count < 0 then
              Error(11);
            NextToken;  { consume field name }
            { Add field offset: x1 = x1 + offset }
            if field_offset[arg_count] > 0 then
            begin
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(35);
              write(field_offset[arg_count]);
              EmitNL
            end;
            { Load field value }
            if field_type[arg_count] = TYPE_REAL then
            begin
              EmitIndent;
              writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
              writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
              writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
              EmitNL;
              expr_type := TYPE_REAL
            end
            else
            begin
              EmitIndent;
              writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
              EmitNL;
              expr_type := field_type[arg_count]
            end
          end
          else
          begin
            { Load element value }
            EmitIndent;
            writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
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
          else if sym_type[idx] = TYPE_STRING then
          begin
            { For strings, emit the address (strings are pass-by-reference) }
            EmitVarAddr(idx, scope_level);
            expr_type := TYPE_STRING;
            { Check for string indexing s[i] }
            if tok_type = TOK_LBRACKET then
            begin
              NextToken;  { consume '[' }
              EmitPushX0;  { save string base address }
              ParseExpression;  { index in x0 }
              Expect(TOK_RBRACKET);
              { x0 = index, stack has base address }
              EmitPopX1;  { x1 = base address }
              { Add index to base: x0 = x1 + x0 }
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(120); writechar(48);  { x0 }
              EmitNL;
              { Load byte at [x0]: ldrb w0, [x0] }
              EmitIndent;
              writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
              writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
              writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
              EmitNL;
              expr_type := TYPE_CHAR
            end
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
        { Track pointer base type if this is a pointer }
        if sym_type[idx] = TYPE_POINTER then
          ptr_base_type := sym_const_val[idx];
        { Check for pointer dereference }
        if (sym_type[idx] = TYPE_POINTER) and (tok_type = TOK_CARET) then
        begin
          NextToken;
          if sym_const_val[idx] = TYPE_RECORD then
          begin
            { Pointer to record - x0 has the address, check for field access }
            if tok_type = TOK_DOT then
            begin
              NextToken;  { consume '.' }
              { Find field in the record type }
              arg_count := FindField(sym_label[idx]);  { sym_label has record type index }
              if arg_count < 0 then
                Error(11);  { unknown field }
              NextToken;  { consume field name }
              { Add field offset to pointer: add x0, x0, #offset }
              if field_offset[arg_count] > 0 then
              begin
                EmitIndent;
                writechar(97); writechar(100); writechar(100); writechar(32);  { add }
                writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
                writechar(35);
                write(field_offset[arg_count]);
                EmitNL
              end;
              { Load field value }
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
              end
            end
            else
            begin
              { Just p^ without field access - not meaningful for records }
              expr_type := TYPE_RECORD
            end
          end
          else if sym_const_val[idx] = TYPE_REAL then
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
      end  { end of if (with_rec_idx < 0) or (arg_count < 0) }
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
  op, left_type, left_ptr_base: integer;
begin
  ParseTerm;
  while (tok_type = TOK_PLUS) or (tok_type = TOK_MINUS) or (tok_type = TOK_OR) do
  begin
    op := tok_type;
    left_type := expr_type;
    left_ptr_base := ptr_base_type;
    NextToken;
    { Push left operand appropriately }
    if left_type = TYPE_REAL then
      EmitPushD0
    else
      EmitPushX0;
    ParseTerm;

    if (left_type = TYPE_POINTER) and (op = TOK_PLUS) then
    begin
      { pointer + integer: scale integer by 8 and SUBTRACT (arrays grow downward) }
      EmitPopX1;  { pointer in x1 }
      { x0 has integer offset, multiply by 8 using lsl #3 }
      EmitIndent;
      writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(51);  { #3 }
      EmitNL;
      EmitSub;  { x0 = x1 - x0 (subtract because arrays grow downward) }
      expr_type := TYPE_POINTER;
      ptr_base_type := left_ptr_base
    end
    else if (left_type = TYPE_POINTER) and (op = TOK_MINUS) then
    begin
      if expr_type = TYPE_POINTER then
      begin
        { pointer - pointer: returns integer count (negated for downward growth) }
        EmitPopX1;  { left pointer in x1 }
        EmitSub;  { x0 = x1 - x0 }
        { Negate and divide by 8 for correct count }
        EmitNeg;  { x0 = -(x1 - x0) = x0 - x1 }
        { Divide by 8 (element size) using asr #3 }
        EmitIndent;
        writechar(97); writechar(115); writechar(114); writechar(32);  { asr }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(35); writechar(51);  { #3 }
        EmitNL;
        expr_type := TYPE_INTEGER
      end
      else
      begin
        { pointer - integer: scale integer by 8 and ADD (arrays grow downward) }
        EmitPopX1;  { pointer in x1 }
        { x0 has integer offset, multiply by 8 using lsl #3 }
        EmitIndent;
        writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(35); writechar(51);  { #3 }
        EmitNL;
        EmitAdd;  { x0 = x1 + x0 (add because arrays grow downward) }
        expr_type := TYPE_POINTER;
        ptr_base_type := left_ptr_base
      end
    end
    else if (left_type = TYPE_STRING) and (expr_type = TYPE_STRING) and (op = TOK_PLUS) then
    begin
      { String concatenation: str1 + str2 }
      { x0 = string2 addr, stack top = string1 addr }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
      writechar(120); writechar(48);  { x0 - string2 }
      EmitNL;
      EmitPopX1;  { string1 addr }
      { Allocate temp string from heap }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(50); writechar(49);  { x21 - heap ptr }
      EmitNL;
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
      EmitNL;
      { Call rt_str_concat(x0=dest, x1=str1, x2=str2) }
      EmitBL(rt_str_concat);
      expr_type := TYPE_STRING
    end
    else if (((left_type = TYPE_STRING) and (expr_type = TYPE_CHAR)) or
             ((left_type = TYPE_CHAR) and (expr_type = TYPE_STRING))) and (op = TOK_PLUS) then
    begin
      { String + char or char + string concatenation }
      { Need to convert char to a single-character string, then concatenate }
      if left_type = TYPE_STRING then
      begin
        { string + char: x0 = char, stack top = string addr }
        { Create single-char string from x0 on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
        writechar(120); writechar(48);  { x0 - save char }
        EmitNL;
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
        writechar(120); writechar(50); writechar(49);  { x21 - heap ptr for char string }
        EmitNL;
        EmitMovX0(1);  { length = 1 }
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
        writechar(91); writechar(120); writechar(50); writechar(93);  { [x2] }
        EmitNL;
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
        writechar(91); writechar(120); writechar(50); writechar(44); writechar(32);  { [x2, }
        writechar(35); writechar(49); writechar(93);  { #1] }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        EmitPopX1;  { string1 addr }
        { x2 = char string, x1 = string1, allocate result on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(50); writechar(49);  { x21 - heap ptr for result }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        EmitBL(rt_str_concat)
      end
      else
      begin
        { char + string: x0 = string addr, stack top = char }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
        writechar(120); writechar(48);  { x0 - save string2 addr }
        EmitNL;
        EmitPopX0;  { x0 = char }
        { Create single-char string from x0 on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
        writechar(120); writechar(50); writechar(49);  { x21 - heap ptr for char string }
        EmitNL;
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
        writechar(120); writechar(48);  { x0 - save char }
        EmitNL;
        EmitMovX0(1);  { length = 1 }
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
        writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
        EmitNL;
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
        writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
        writechar(35); writechar(49); writechar(93);  { #1] }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        { x1 = char string, x2 = string2, allocate result on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(50); writechar(49);  { x21 - heap ptr for result }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        EmitBL(rt_str_concat)
      end;
      expr_type := TYPE_STRING
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

    if (left_type = TYPE_STRING) and (expr_type = TYPE_STRING) then
    begin
      { String comparison }
      { x0 = string2 addr, stack top = string1 addr }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitPopX0;  { string1 addr in x0 }
      if (op = TOK_EQ) or (op = TOK_NEQ) then
      begin
        { Call rt_str_compare(x0=str1, x1=str2) - returns 1 if equal, 0 if not }
        EmitBL(rt_str_compare);
        if op = TOK_NEQ then
          EmitEorX0(1)
      end
      else
      begin
        { Relational operators: call rt_str_cmp which returns -1/0/1 }
        EmitBL(rt_str_cmp);
        { x0 = -1 if s1<s2, 0 if s1=s2, 1 if s1>s2 }
        if op = TOK_LT then
        begin
          { cmp x0, #0; cset x0, lt }
          EmitIndent;
          writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(35); writechar(48);  { #0 }
          EmitNL;
          EmitIndent;
          writechar(99); writechar(115); writechar(101); writechar(116); writechar(32);  { cset }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(108); writechar(116);  { lt }
          EmitNL
        end
        else if op = TOK_GT then
        begin
          { cmp x0, #0; cset x0, gt }
          EmitIndent;
          writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(35); writechar(48);  { #0 }
          EmitNL;
          EmitIndent;
          writechar(99); writechar(115); writechar(101); writechar(116); writechar(32);  { cset }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(103); writechar(116);  { gt }
          EmitNL
        end
        else if op = TOK_LE then
        begin
          { cmp x0, #0; cset x0, le }
          EmitIndent;
          writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(35); writechar(48);  { #0 }
          EmitNL;
          EmitIndent;
          writechar(99); writechar(115); writechar(101); writechar(116); writechar(32);  { cset }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(108); writechar(101);  { le }
          EmitNL
        end
        else if op = TOK_GE then
        begin
          { cmp x0, #0; cset x0, ge }
          EmitIndent;
          writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(35); writechar(48);  { #0 }
          EmitNL;
          EmitIndent;
          writechar(99); writechar(115); writechar(101); writechar(116); writechar(32);  { cset }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(103); writechar(101);  { ge }
          EmitNL
        end
      end;
      expr_type := TYPE_INTEGER
    end
    else if (left_type = TYPE_REAL) or (expr_type = TYPE_REAL) then
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
      EmitFCmp;
      if op = TOK_EQ then cond := 0
      else if op = TOK_NEQ then cond := 1
      else if op = TOK_LT then cond := 2
      else if op = TOK_LE then cond := 3
      else if op = TOK_GT then cond := 4
      else cond := 5;
      EmitCset(cond);
      expr_type := TYPE_INTEGER
    end
    else
    begin
      { Integer comparison }
      EmitPopX1;
      EmitCmpX0X1;
      if op = TOK_EQ then cond := 0
      else if op = TOK_NEQ then cond := 1
      else if op = TOK_LT then cond := 2
      else if op = TOK_LE then cond := 3
      else if op = TOK_GT then cond := 4
      else cond := 5;
      EmitCset(cond);
      expr_type := TYPE_INTEGER
    end
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
  else if tok_type = TOK_CASE then
  begin
    { CASE selector OF const: stmt; ... [ELSE stmt] END }
    NextToken;
    ParseExpression;  { selector in x0 }
    EmitPushX0;       { save selector on stack }
    Expect(TOK_OF);
    lbl1 := NewLabel;  { end of case label }

    while (tok_type <> TOK_END) and (tok_type <> TOK_ELSE) do
    begin
      { Parse constant(s) for this case branch }
      lbl2 := NewLabel;  { next case branch label }
      lbl3 := NewLabel;  { match found, execute statement }
      repeat
        if tok_type = TOK_COMMA then NextToken;
        { Load selector from stack (peek, don't pop): ldr x1, [sp] }
        EmitIndent;
        writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
        writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
        writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
        EmitNL;
        { Parse constant value }
        if tok_type = TOK_INTEGER then
        begin
          EmitMovX0(tok_int);
          NextToken
        end
        else if tok_type = TOK_MINUS then
        begin
          NextToken;
          if tok_type = TOK_INTEGER then
          begin
            EmitMovX0(0 - tok_int);
            NextToken
          end
          else
            Error(10)
        end
        else
          Error(10);  { expected constant }
        { Compare: cmp x1, x0 }
        EmitCmpX0X1;
        { If equal, jump to match label }
        EmitIndent;
        writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
        writechar(76); write(lbl3);
        EmitNL
      until tok_type = TOK_COLON;
      { No match in this group, jump to next branch }
      EmitBranchLabel(lbl2);
      { Match found - execute statement }
      EmitLabel(lbl3);
      NextToken;  { consume ':' }
      { Pop selector from stack (discard) }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL;
      ParseStatement;
      { Jump to end of case }
      EmitBranchLabel(lbl1);
      { Repush selector for next comparisons: sub sp, sp, #16; str x0, [sp] }
      { Actually we need to restore selector - this gets tricky }
      { Simpler approach: jump around the next branch's test }
      EmitLabel(lbl2);
      { After ParseStatement the selector was popped, repush it }
      { For simplicity, we push a placeholder and let the next iteration reload }
      { Actually, we need the selector back - let's use a different approach }
      { The cleanest way: keep selector on stack until we match or hit else/end }
      { Re-push selector: we saved it in x0 before? No. Let's rethink. }
      { Better: save selector to a dedicated spot before the loop }
      { For now: don't pop until match, just peek. Pop only when match or done. }
      { Skip the repush - we've jumped to lbl1 if matched, lbl2 if not }
      { At lbl2, we still have selector on stack }
      if tok_type = TOK_SEMICOLON then NextToken
    end;

    if tok_type = TOK_ELSE then
    begin
      NextToken;
      { Pop selector (discard) before else statement }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL;
      ParseStatement;
      { Note: no need to jump to lbl1, we fall through }
    end
    else
    begin
      { No else - just pop selector }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL
    end;
    EmitLabel(lbl1);
    Expect(TOK_END)
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
        else if sym_type[idx] = TYPE_STRING then
        begin
          { Load string variable address into x0 }
          EmitVarAddr(idx, scope_level);
          { Call read_string runtime }
          EmitBL(rt_read_string)
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
  else if tok_type = TOK_WITH then
  begin
    { WITH record_var DO statement }
    NextToken;
    if tok_type <> TOK_IDENT then
      Error(6);  { expected identifier }
    idx := SymLookup;
    if idx < 0 then
      Error(3);  { undefined identifier }
    if sym_type[idx] <> TYPE_RECORD then
      Error(9);  { expected record type }
    { Save current with context }
    lbl1 := with_rec_idx;  { reuse lbl1 to save old with_rec_idx }
    lbl2 := with_rec_type;  { reuse lbl2 to save old with_rec_type }
    with_rec_idx := idx;
    with_rec_type := sym_const_val[idx];  { the type definition index }
    NextToken;
    Expect(TOK_DO);
    ParseStatement;
    { Restore with context }
    with_rec_idx := lbl1;
    with_rec_type := lbl2
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
    { inc = 105,110,99 }
    else if TokIs8(105, 110, 99, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(3);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      NextToken;
      { Load address into x8 }
      EmitVarAddr(idx, scope_level);
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      { Load current value }
      EmitIndent;
      writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
      EmitNL;
      { Add 1 }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      { Store back }
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(32);  { str }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
      EmitNL;
      Expect(TOK_RPAREN)
    end
    { dec = 100,101,99 }
    else if TokIs8(100, 101, 99, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(3);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      NextToken;
      { Load address into x8 }
      EmitVarAddr(idx, scope_level);
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      { Load current value }
      EmitIndent;
      writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
      EmitNL;
      { Subtract 1 }
      EmitIndent;
      writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(35); writechar(49);  { #1 }
      EmitNL;
      { Store back }
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(32);  { str }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
      EmitNL;
      Expect(TOK_RPAREN)
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
    else if TokIs8(110, 101, 119, 0, 0, 0, 0, 0) = 1 then
    begin
      { new(p) - allocate memory for pointer variable }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);  { expected identifier }
      idx := SymLookup;
      if idx < 0 then
        Error(3);  { undefined identifier }
      if sym_type[idx] <> TYPE_POINTER then
        Error(14);  { expected pointer type }
      NextToken;
      Expect(TOK_RPAREN);
      { Determine allocation size based on base type }
      if sym_const_val[idx] = TYPE_RECORD then
        lbl1 := sym_label[sym_label[idx]]  { record size from type definition }
      else
        lbl1 := 8;  { basic types are 8 bytes }
      { Align to 8 bytes }
      lbl1 := ((lbl1 + 7) div 8) * 8;
      { Allocate: get current heap pointer (x21), bump by size }
      { mov x0, x21 }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(120); writechar(50); writechar(49);  { x21 }
      EmitNL;
      { add x21, x21, #size }
      EmitIndent;
      writechar(97); writechar(100); writechar(100); writechar(32);  { add }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
      writechar(35);
      write(lbl1);
      EmitNL;
      { Store address in pointer variable }
      if sym_level[idx] < scope_level then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      else
        EmitSturX0(sym_offset[idx])
    end
    else if (tok_len = 7) and (tok_str[0] = 100) and (tok_str[1] = 105) and
            (tok_str[2] = 115) and (tok_str[3] = 112) and (tok_str[4] = 111) and
            (tok_str[5] = 115) and (tok_str[6] = 101) then
    begin
      { dispose(p) - free memory (no-op for bump allocator) }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);  { expected identifier }
      idx := SymLookup;
      if idx < 0 then
        Error(3);  { undefined identifier }
      if sym_type[idx] <> TYPE_POINTER then
        Error(14);  { expected pointer type }
      NextToken;
      Expect(TOK_RPAREN)
      { No-op for bump allocator - could set pointer to nil for safety }
    end
    { delete = 100,101,108,101,116,101 }
    else if (tok_len = 6) and (tok_str[0] = 100) and (tok_str[1] = 101) and
            (tok_str[2] = 108) and (tok_str[3] = 101) and (tok_str[4] = 116) and
            (tok_str[5] = 101) then
    begin
      { delete(s, start, count) - remove chars from string }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: string variable }
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if sym_type[idx] <> TYPE_STRING then
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
      { x0=count, stack has string addr and start }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitPopX1;  { start }
      EmitPopX0;  { string addr }
      EmitBL(rt_str_delete);
      Expect(TOK_RPAREN)
    end
    { insert = 105,110,115,101,114,116 }
    else if (tok_len = 6) and (tok_str[0] = 105) and (tok_str[1] = 110) and
            (tok_str[2] = 115) and (tok_str[3] = 101) and (tok_str[4] = 114) and
            (tok_str[5] = 116) then
    begin
      { insert(source, dest, pos) - insert source into dest at pos }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: source string }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level);
        EmitPushX0
      end
      else if tok_type = TOK_STRING then
      begin
        { String literal - allocate temp on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
        writechar(120); writechar(50); writechar(49);  { x21 }
        EmitNL;
        EmitMovX0(tok_len);
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
        writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
        EmitNL;
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
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(50); writechar(49);  { x21 }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        EmitPushX0;
        NextToken
      end
      else
        Error(9);
      Expect(TOK_COMMA);
      { Second arg: dest string variable }
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if sym_type[idx] <> TYPE_STRING then
        Error(9);
      NextToken;
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      Expect(TOK_COMMA);
      { Third arg: position }
      ParseExpression;
      { x0=pos, stack has source and dest }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitPopX1;  { dest }
      EmitPopX0;  { source }
      EmitBL(rt_str_insert);
      Expect(TOK_RPAREN)
    end
    { str = 115,116,114 - but only if not a local variable }
    else if (tok_len = 3) and (tok_str[0] = 115) and (tok_str[1] = 116) and
            (tok_str[2] = 114) and (SymLookup < 0) then
    begin
      { str(n, s) - convert integer n to string s }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: integer expression }
      ParseExpression;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Second arg: string variable }
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if sym_type[idx] <> TYPE_STRING then
        Error(9);
      NextToken;
      EmitVarAddr(idx, scope_level);
      { x0 = string addr, pop integer into x1 then swap }
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitPopX0;  { x0 = integer value }
      { Now: x0 = integer, x1 = string addr }
      EmitBL(rt_int_to_str);
      Expect(TOK_RPAREN)
    end
    { val = 118,97,108 - but only if not a local variable }
    else if (tok_len = 3) and (tok_str[0] = 118) and (tok_str[1] = 97) and
            (tok_str[2] = 108) and (SymLookup < 0) then
    begin
      { val(s, v, code) - convert string s to integer v, error in code }
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: string }
      if tok_type = TOK_IDENT then
      begin
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_type[idx] <> TYPE_STRING then
          Error(9);
        NextToken;
        EmitVarAddr(idx, scope_level)
      end
      else if tok_type = TOK_STRING then
      begin
        { String literal - allocate temp on heap }
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
        writechar(120); writechar(50); writechar(49);  { x21 }
        EmitNL;
        EmitMovX0(tok_len);
        EmitIndent;
        writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
        writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
        writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
        EmitNL;
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
        EmitIndent;
        writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
        writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
        writechar(120); writechar(50); writechar(49);  { x21 }
        EmitNL;
        EmitIndent;
        writechar(97); writechar(100); writechar(100); writechar(32);  { add }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
        writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
        EmitNL;
        NextToken
      end
      else
        Error(9);
      { Call rt_str_to_int: x0=string addr -> x0=value, x1=error }
      EmitBL(rt_str_to_int);
      { Save both results: push error first (x1), then value (x0) }
      EmitPushX1;
      EmitPushX0;
      Expect(TOK_COMMA);
      { Second arg: integer variable to receive value }
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      NextToken;
      { Get address of v into x0, move to x1, pop value, store }
      EmitVarAddr(idx, scope_level);
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitPopX0;  { value }
      { str x0, [x1] }
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(32);  { str }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
      EmitNL;
      Expect(TOK_COMMA);
      { Third arg: integer variable to receive error code }
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      NextToken;
      { Get address of code into x0, move to x1, pop error, store }
      EmitVarAddr(idx, scope_level);
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
      writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
      writechar(120); writechar(48);  { x0 }
      EmitNL;
      EmitPopX0;  { error }
      { str x0, [x1] }
      EmitIndent;
      writechar(115); writechar(116); writechar(114); writechar(32);  { str }
      writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
      writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
      EmitNL;
      Expect(TOK_RPAREN)
    end
    { clrscr = 99,108,114,115,99,114 }
    else if TokIs8(99, 108, 114, 115, 99, 114, 0, 0) = 1 then
    begin
      NextToken;
      EmitBL(rt_clrscr)
    end
    { gotoxy = 103,111,116,111,120,121 }
    else if TokIs8(103, 111, 116, 111, 120, 121, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;    { x }
      EmitPushX0;
      Expect(TOK_COMMA);
      ParseExpression;    { y }
      EmitPopX1;          { x1=x, x0=y }
      EmitBL(rt_gotoxy);
      Expect(TOK_RPAREN)
    end
    { clreol = 99,108,114,101,111,108 }
    else if TokIs8(99, 108, 114, 101, 111, 108, 0, 0) = 1 then
    begin
      NextToken;
      EmitBL(rt_clreol)
    end
    { textcolor = 116,101,120,116,99,111,108,111,114 (9 chars) }
    else if (tok_len = 9) and (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 101) and
            (ToLower(tok_str[2]) = 120) and (ToLower(tok_str[3]) = 116) and (ToLower(tok_str[4]) = 99) and
            (ToLower(tok_str[5]) = 111) and (ToLower(tok_str[6]) = 108) and (ToLower(tok_str[7]) = 111) and
            (ToLower(tok_str[8]) = 114) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;    { color code 0-7 }
      EmitBL(rt_textcolor);
      Expect(TOK_RPAREN)
    end
    { textbackground = 116,101,120,116,98,97,99,107,103,114,111,117,110,100 (14 chars) }
    else if (tok_len = 14) and (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 101) and
            (ToLower(tok_str[2]) = 120) and (ToLower(tok_str[3]) = 116) and (ToLower(tok_str[4]) = 98) and
            (ToLower(tok_str[5]) = 97) and (ToLower(tok_str[6]) = 99) and (ToLower(tok_str[7]) = 107) and
            (ToLower(tok_str[8]) = 103) and (ToLower(tok_str[9]) = 114) and (ToLower(tok_str[10]) = 111) and
            (ToLower(tok_str[11]) = 117) and (ToLower(tok_str[12]) = 110) and (ToLower(tok_str[13]) = 100) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;    { color code 0-7 }
      EmitBL(rt_textbackground);
      Expect(TOK_RPAREN)
    end
    { normvideo = 110,111,114,109,118,105,100,101,111 (9 chars) }
    else if (tok_len = 9) and (ToLower(tok_str[0]) = 110) and (ToLower(tok_str[1]) = 111) and
            (ToLower(tok_str[2]) = 114) and (ToLower(tok_str[3]) = 109) and (ToLower(tok_str[4]) = 118) and
            (ToLower(tok_str[5]) = 105) and (ToLower(tok_str[6]) = 100) and (ToLower(tok_str[7]) = 101) and
            (ToLower(tok_str[8]) = 111) then
    begin
      NextToken;
      EmitBL(rt_normvideo)
    end
    { highvideo = 104,105,103,104,118,105,100,101,111 (9 chars) }
    else if (tok_len = 9) and (ToLower(tok_str[0]) = 104) and (ToLower(tok_str[1]) = 105) and
            (ToLower(tok_str[2]) = 103) and (ToLower(tok_str[3]) = 104) and (ToLower(tok_str[4]) = 118) and
            (ToLower(tok_str[5]) = 105) and (ToLower(tok_str[6]) = 100) and (ToLower(tok_str[7]) = 101) and
            (ToLower(tok_str[8]) = 111) then
    begin
      NextToken;
      EmitBL(rt_highvideo)
    end
    { lowvideo = 108,111,119,118,105,100,101,111 (8 chars) }
    else if TokIs8(108, 111, 119, 118, 105, 100, 101, 111) = 1 then
    begin
      NextToken;
      EmitBL(rt_lowvideo)
    end
    { hidecursor = 104,105,100,101,99,117,114,115,111,114 (10 chars) }
    else if (tok_len = 10) and (ToLower(tok_str[0]) = 104) and (ToLower(tok_str[1]) = 105) and
            (ToLower(tok_str[2]) = 100) and (ToLower(tok_str[3]) = 101) and (ToLower(tok_str[4]) = 99) and
            (ToLower(tok_str[5]) = 117) and (ToLower(tok_str[6]) = 114) and (ToLower(tok_str[7]) = 115) and
            (ToLower(tok_str[8]) = 111) and (ToLower(tok_str[9]) = 114) then
    begin
      NextToken;
      EmitBL(rt_hidecursor)
    end
    { showcursor = 115,104,111,119,99,117,114,115,111,114 (10 chars) }
    else if (tok_len = 10) and (ToLower(tok_str[0]) = 115) and (ToLower(tok_str[1]) = 104) and
            (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 119) and (ToLower(tok_str[4]) = 99) and
            (ToLower(tok_str[5]) = 117) and (ToLower(tok_str[6]) = 114) and (ToLower(tok_str[7]) = 115) and
            (ToLower(tok_str[8]) = 111) and (ToLower(tok_str[9]) = 114) then
    begin
      NextToken;
      EmitBL(rt_showcursor)
    end
    { sleep = 115,108,101,101,112 (5 chars) - Sleep(ms) }
    else if TokIs8(115, 108, 101, 101, 112, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { ms in x0 }
      EmitBL(rt_sleep);
      Expect(TOK_RPAREN)
    end
    else
    begin
      { Check for WITH context - try to find identifier as a field }
      if with_rec_idx >= 0 then
      begin
        arg_count := FindField(with_rec_type);  { reuse arg_count for field_idx }
        if arg_count >= 0 then
        begin
          { Found field - handle assignment }
          NextToken;
          Expect(TOK_ASSIGN);
          ParseExpression;  { value in x0 or d0 }
          { Compute address: base + field_offset }
          if field_type[arg_count] = TYPE_REAL then
          begin
            { Value is in d0, need to store to field }
            if expr_type <> TYPE_REAL then
              EmitScvtfD0X0;
            EmitPushD0;  { save value }
          end
          else
            EmitPushX0;  { save value }
          { Compute address }
          if sym_level[with_rec_idx] < scope_level then
          begin
            EmitFollowChain(sym_level[with_rec_idx], scope_level);
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(35);
            write(sym_offset[with_rec_idx] + field_offset[arg_count]);
            EmitNL
          end
          else
          begin
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
            writechar(35);
            write(sym_offset[with_rec_idx] + field_offset[arg_count]);
            EmitNL
          end;
          { Store value to field }
          if field_type[arg_count] = TYPE_REAL then
          begin
            EmitPopD0;
            { str d0, [x1] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
          else
          begin
            EmitPopX0;
            { str x0, [x1] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
        end
        else
        begin
          { Field not found, proceed with normal lookup }
          idx := SymLookup;
          if idx < 0 then
            Error(3)
        end
      end
      else
      begin
        { Not a built-in, look up in symbol table }
        idx := SymLookup;
        if idx < 0 then
          Error(3)
      end;
      { Skip if we already handled WITH field assignment }
      if (with_rec_idx < 0) or (arg_count < 0) then
      begin
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
          { Array element assignment: arr[index] := expr or arr[index].field := expr }
          NextToken;  { consume '[' }
          ParseExpression;  { index in x0 }
          Expect(TOK_RBRACKET);
          { Subtract low bound }
          EmitPushX0;
          EmitMovX0(sym_const_val[idx]);
          EmitPopX1;
          { x0 = x1 - x0 = index - low_bound }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(48);  { x0 }
          EmitNL;
          { Multiply by element size }
          if sym_var_param_flags[idx] > 0 then
          begin
            { Array of records }
            lbl1 := sym_label[sym_var_param_flags[idx] - 1];
            EmitPushX0;
            EmitMovX0(lbl1);
            EmitPopX1;
            EmitIndent;
            writechar(109); writechar(117); writechar(108); writechar(32);  { mul }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(120); writechar(48);  { x0 }
            EmitNL
          end
          else
          begin
            EmitIndent;
            writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(35); writechar(51);  { #3 }
            EmitNL
          end;
          { Get base address }
          if sym_level[idx] < scope_level then
          begin
            EmitFollowChain(sym_level[idx], scope_level);
            EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
          end
          else
            EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
          { x1 = base, x0 = offset, compute element address in x1 }
          EmitIndent;
          writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
          writechar(120); writechar(48);  { x0 }
          EmitNL;
          { Check for field access }
          if (sym_var_param_flags[idx] > 0) and (tok_type = TOK_DOT) then
          begin
            NextToken;
            arg_count := FindField(sym_var_param_flags[idx] - 1);
            if arg_count < 0 then
              Error(11);
            NextToken;
            { Add field offset }
            if field_offset[arg_count] > 0 then
            begin
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
              writechar(35);
              write(field_offset[arg_count]);
              EmitNL
            end;
            EmitPushX1;  { save field address }
            Expect(TOK_ASSIGN);
            ParseExpression;
            if field_type[arg_count] = TYPE_REAL then
            begin
              if expr_type <> TYPE_REAL then
                EmitScvtfD0X0;
              EmitPopX1;
              EmitIndent;
              writechar(115); writechar(116); writechar(114); writechar(32);  { str }
              writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
              writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
              EmitNL
            end
            else
            begin
              EmitPopX1;
              EmitIndent;
              writechar(115); writechar(116); writechar(114); writechar(32);  { str }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
              EmitNL
            end
          end
          else
          begin
            { Basic array assignment }
            EmitPushX1;  { save element address }
            Expect(TOK_ASSIGN);
            ParseExpression;
            EmitPopX1;
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(32);  { str }
            writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
            writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
            EmitNL
          end
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
          { Pointer dereference assignment: p^ := value or p^.field := value }
          NextToken;  { consume ^ }
          { Load pointer value (address) }
          if sym_level[idx] < scope_level then
            EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          else
            EmitLdurX0(sym_offset[idx]);

          if (sym_const_val[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
          begin
            { Pointer to record field assignment: p^.field := value }
            NextToken;  { consume '.' }
            { Find field in the record type }
            arg_count := FindField(sym_label[idx]);  { sym_label has record type index }
            if arg_count < 0 then
              Error(11);  { unknown field }
            NextToken;  { consume field name }
            { Add field offset to pointer: x0 = x0 + offset }
            if field_offset[arg_count] > 0 then
            begin
              EmitIndent;
              writechar(97); writechar(100); writechar(100); writechar(32);  { add }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(35);
              write(field_offset[arg_count]);
              EmitNL
            end;
            EmitPushX0;  { save field address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value to store }
            if field_type[arg_count] = TYPE_REAL then
            begin
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
              EmitPopX1;  { get address into x1 }
              { str x0, [x1] }
              EmitIndent;
              writechar(115); writechar(116); writechar(114); writechar(32);  { str }
              writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
              writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
              EmitNL
            end
          end
          else
          begin
            { Simple pointer dereference assignment: p^ := value }
            EmitPushX0;  { save address }
            Expect(TOK_ASSIGN);
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
        end
        else if sym_type[idx] = TYPE_STRING then
        begin
          { Check for string indexed assignment s[i] := char }
          if tok_type = TOK_LBRACKET then
          begin
            NextToken;  { consume '[' }
            { Get string base address }
            EmitVarAddr(idx, scope_level);
            EmitPushX0;  { save base address }
            ParseExpression;  { index in x0 }
            Expect(TOK_RBRACKET);
            { Add index to base: address = base + index }
            EmitPopX1;  { x1 = base address }
            EmitIndent;
            writechar(97); writechar(100); writechar(100); writechar(32);  { add }
            writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(120); writechar(48);  { x0 }
            EmitNL;
            { x8 now has the address to store to }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value to store in x0 }
            { strb w0, [x8] }
            EmitIndent;
            writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
            writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
            writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
            EmitNL
          end
          else
          begin
          { String assignment }
          Expect(TOK_ASSIGN);
          if tok_type = TOK_STRING then
          begin
            { Assign from string literal }
            { Compute base address of string variable into x8 }
            if sym_level[idx] < scope_level then
            begin
              EmitFollowChain(sym_level[idx], scope_level);
              { add/sub x8, x8, #offset }
              if sym_offset[idx] < 0 then
              begin
                EmitIndent;
                writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
                writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
                writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
                writechar(35);
                write(0 - sym_offset[idx]);
                EmitNL
              end
              else
              begin
                EmitIndent;
                writechar(97); writechar(100); writechar(100); writechar(32);  { add }
                writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
                writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
                writechar(35);
                write(sym_offset[idx]);
                EmitNL
              end
            end
            else
            begin
              { add/sub x8, x29, #offset }
              if sym_offset[idx] < 0 then
              begin
                EmitIndent;
                writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
                writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
                writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
                writechar(35);
                write(0 - sym_offset[idx]);
                EmitNL
              end
              else
              begin
                EmitIndent;
                writechar(97); writechar(100); writechar(100); writechar(32);  { add }
                writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
                writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
                writechar(35);
                write(sym_offset[idx]);
                EmitNL
              end
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
          else
          begin
            { Parse string expression (variable, copy, concat, or + expressions) }
            ParseExpression;
            if expr_type <> TYPE_STRING then
              Error(12);  { expected string }
            { x0 = source string address, copy to dest }
            EmitIndent;
            writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
            writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
            writechar(120); writechar(48);  { x0 }
            EmitNL;
            { Get dest address into x0 }
            EmitVarAddr(idx, scope_level);
            { Call rt_str_copy(x0=dest, x1=source) }
            EmitBL(rt_str_copy)
          end
          end  { end of else for string whole assignment }
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
        else if sym_type[idx] = TYPE_STRING then
        begin
          { String function - x0 has source string addr, copy to heap for return }
          { x0 = source, x1 = dest (heap), call str_copy }
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
          writechar(120); writechar(48);  { x0 - save source }
          EmitNL;
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
          writechar(120); writechar(50); writechar(49);  { x21 - heap ptr }
          EmitNL;
          { Save heap addr as result }
          EmitSturX0(-16);
          { x8 = dest, x9 = source for str_copy }
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
          writechar(120); writechar(56);  { x8 - source }
          EmitNL;
          EmitIndent;
          writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
          writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
          writechar(120); writechar(50); writechar(49);  { x21 - dest }
          EmitNL;
          EmitBL(rt_str_copy);
          { Advance heap pointer }
          EmitIndent;
          writechar(97); writechar(100); writechar(100); writechar(32);  { add }
          writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
          writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
          writechar(35); writechar(50); writechar(53); writechar(54);  { #256 }
          EmitNL
        end
        else
          EmitSturX0(-16)
      end
      else
        Error(7)
      end  { end of if (with_rec_idx < 0) or (arg_count < 0) }
    end  { end of else for non-builtin identifier }
  end  { end of else if tok_type = TOK_IDENT }
end;

