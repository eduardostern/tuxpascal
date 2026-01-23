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
  idx, arg_count, i, lbl1, lbl2: integer;
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
  else if tok_type = TOK_LBRACKET then
  begin
    { Set constructor: [1, 3, 5] or [1..5] or ['a'..'z'] }
    NextToken;
    EmitMovX0(0);  { start with empty set }
    if tok_type <> TOK_RBRACKET then
    begin
      repeat
        if tok_type = TOK_COMMA then NextToken;
        { Save set so far }
        EmitPushX0;
        { Parse first value }
        ParseExpression;
        if tok_type = TOK_DOTDOT then
        begin
          { Range: lo..hi - create mask for all bits from lo to hi }
          EmitPushX0;  { save lo }
          NextToken;
          ParseExpression;  { hi in x0 }
          { x0 = hi, need to set bits from lo to hi inclusive }
          { Algorithm: create mask with bits set from lo to hi }
          { For simplicity, use a loop: for i := lo to hi do set |= (1 << i) }
          writeln('    mov x2, x0');
          EmitPopX1;  { x1 = lo }
          EmitPopX0;  { x0 = set so far }
          { Loop: while x1 <= x2 do x0 |= (1 << x1); x1++ }
          lbl1 := NewLabel;
          lbl2 := NewLabel;
          EmitLabel(lbl1);
          { cmp x1, x2 }
          writeln('    cmp x1, x2');
          { b.gt done }
          write('    b.gt L'); writeln(lbl2);
          { x3 = 1 << x1 }
          writeln('    mov x3, #1');
          writeln('    lsl x3, x3, x1');
          { x0 |= x3 }
          writeln('    orr x0, x0, x3');
          { x1++ }
          writeln('    add x1, x1, #1');
          EmitBranchLabel(lbl1);
          EmitLabel(lbl2)
        end
        else
        begin
          { Single element: set the bit }
          { x0 has the element value, stack has set so far }
          { mask = 1 << x0, then OR with saved set }
          writeln('    mov x1, #1');
          writeln('    lsl x1, x1, x0');
          EmitPopX0;  { saved set }
          EmitOrrX0X1  { set | (1 << element) }
        end
      until tok_type = TOK_RBRACKET
    end;
    NextToken;
    expr_type := TYPE_SET
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
      writeln('    mov x8, x21');
      { Store length byte }
      EmitMovX0(tok_len);
      writeln('    strb w0, [x8]');
      { Store each character }
      for i := 0 to tok_len - 1 do
      begin
        EmitMovX0(tok_str[i]);
        write('    strb w0, [x8, #'); write(i + 1); writeln(']');
      end;
      { mov x0, x21 ; return string address }
      writeln('    mov x0, x21');
      { add x21, x21, #256 ; advance heap pointer }
      writeln('    add x21, x21, #256');
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
      writeln('    sub x0, x1, x0');
      { Multiply by 8 (element size) using lsl #3 }
      writeln('    lsl x0, x0, #3');
      { Get base address }
      if sym_level[idx] < scope_level then
      begin
        EmitFollowChain(sym_level[idx], scope_level);
        EmitSubLargeOffset(1, 8, 0 - sym_offset[idx])
      end
      else
        EmitSubLargeOffset(1, 29, 0 - sym_offset[idx]);
      { Address = base - element_offset }
      writeln('    sub x0, x1, x0');
    end
    else
      EmitVarAddr(idx, scope_level);
    expr_type := TYPE_POINTER;
    ptr_base_type := sym_type[idx]
  end
  else if tok_type = TOK_IDENT then
  begin
    { Check for built-in functions: readchar, keypressed, ord, chr }
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
    { getinputfd = 103,101,116,105,110,112,117,116,102,100 (10 chars) }
    else if (tok_len = 10) and (ToLower(tok_str[0]) = 103) and (ToLower(tok_str[1]) = 101) and
            (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 105) and (ToLower(tok_str[4]) = 110) and
            (ToLower(tok_str[5]) = 112) and (ToLower(tok_str[6]) = 117) and (ToLower(tok_str[7]) = 116) and
            (ToLower(tok_str[8]) = 102) and (ToLower(tok_str[9]) = 100) then
    begin
      { getinputfd - returns current input file descriptor (x19) }
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        Expect(TOK_RPAREN)
      end;
      { mov x0, x19 }
      writeln('    mov x0, x19');
      expr_type := TYPE_INTEGER
    end
    { getoutputfd = 103,101,116,111,117,116,112,117,116,102,100 (11 chars) }
    else if (tok_len = 11) and (ToLower(tok_str[0]) = 103) and (ToLower(tok_str[1]) = 101) and
            (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 111) and (ToLower(tok_str[4]) = 117) and
            (ToLower(tok_str[5]) = 116) and (ToLower(tok_str[6]) = 112) and (ToLower(tok_str[7]) = 117) and
            (ToLower(tok_str[8]) = 116) and (ToLower(tok_str[9]) = 102) and (ToLower(tok_str[10]) = 100) then
    begin
      { getoutputfd - returns current output file descriptor (x20) }
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        Expect(TOK_RPAREN)
      end;
      { mov x0, x20 }
      writeln('    mov x0, x20');
      expr_type := TYPE_INTEGER
    end
    { readfd = 114,101,97,100,102,100 (6 chars) }
    else if (tok_len = 6) and (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) and
            (ToLower(tok_str[2]) = 97) and (ToLower(tok_str[3]) = 100) and (ToLower(tok_str[4]) = 102) and
            (ToLower(tok_str[5]) = 100) then
    begin
      { readfd(fd) - read one char from fd, returns char or -1 for EOF }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = fd, save it then do read syscall }
      { sub sp, sp, #16; str x0, [sp] - allocate buffer and save fd }
      writeln('    sub sp, sp, #16');
      { mov x1, sp - buffer on stack }
      writeln('    mov x1, sp');
      { mov x2, #1 - read 1 byte }
      writeln('    mov x2, #1');
      { read syscall: x16 = 0x2000003 }
      writeln('    movz x16, #3');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Check if read returned >= 1 }
      { cmp x0, #1; b.ge got_char; mov x0, #-1; b done; got_char: ldrb w0, [sp]; done: }
      writeln('    cmp x0, #1');
      write('    b.ge L'); writeln(label_count);
      EmitMovX0(-1);
      EmitBranchLabel(label_count + 1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      { ldrb w0, [sp] }
      writeln('    ldrb w0, [sp]');
      EmitLabel(label_count);
      label_count := label_count + 1;
      { add sp, sp, #16 - restore stack }
      writeln('    add sp, sp, #16');
      expr_type := TYPE_INTEGER
    end
    { openfile = 111,112,101,110,102,105,108,101 (8 chars) }
    else if TokIs8(111, 112, 101, 110, 102, 105, 108, 101) = 1 then
    begin
      { openfile(filename) - open file for reading, returns fd or -1 on error }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = Pascal string address, need to skip length byte for C string }
      { add x0, x0, #1 }
      writeln('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_RDONLY(0), x2=mode(0) }
      writeln('    mov x1, #0');
      writeln('    mov x2, #0');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      writeln('    movz x16, #5');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { On macOS, carry flag set on error - convert errno to -1 }
      { b.cc skip; mov x0, #-1; skip: }
      write('    b.cc L'); writeln(label_count);
      EmitMovX0(-1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_INTEGER
    end
    { createfile = 99,114,101,97,116,101,102,105,108,101 (10 chars) }
    else if (tok_len = 10) and (ToLower(tok_str[0]) = 99) and (ToLower(tok_str[1]) = 114) and
            (ToLower(tok_str[2]) = 101) and (ToLower(tok_str[3]) = 97) and (ToLower(tok_str[4]) = 116) and
            (ToLower(tok_str[5]) = 101) and (ToLower(tok_str[6]) = 102) and (ToLower(tok_str[7]) = 105) and
            (ToLower(tok_str[8]) = 108) and (ToLower(tok_str[9]) = 101) then
    begin
      { createfile(filename) - create/open file for writing, returns fd or -1 }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = Pascal string address, need to skip length byte for C string }
      { add x0, x0, #1 }
      writeln('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_WRONLY|O_CREAT|O_TRUNC(1537), x2=mode(420=0644) }
      { mov x1, #1537 }
      writeln('    mov x1, #1537');
      { mov x2, #420 }
      writeln('    mov x2, #420');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      writeln('    movz x16, #5');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { On macOS, carry flag set on error - convert errno to -1 }
      write('    b.cc L'); writeln(label_count);
      EmitMovX0(-1);
      EmitLabel(label_count);
      label_count := label_count + 1;
      expr_type := TYPE_INTEGER
    end
    { keypressed = 107,101,121,112,114,101,115,115,101,100 (10 chars) }
    else if (tok_len = 10) and (ToLower(tok_str[0]) = 107) and (ToLower(tok_str[1]) = 101) and
            (ToLower(tok_str[2]) = 121) and (ToLower(tok_str[3]) = 112) and (ToLower(tok_str[4]) = 114) and
            (ToLower(tok_str[5]) = 101) and (ToLower(tok_str[6]) = 115) and (ToLower(tok_str[7]) = 115) and
            (ToLower(tok_str[8]) = 101) and (ToLower(tok_str[9]) = 100) then
    begin
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        Expect(TOK_RPAREN)
      end;
      EmitBL(rt_keypressed);
      expr_type := TYPE_BOOLEAN
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
      writeln('    cmp x0, #0');
      write('    b.ge L'); writeln(label_count);
      writeln('    neg x0, x0');
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
      writeln('    and x0, x0, #1');
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
      writeln('    mul x0, x0, x0');
      expr_type := TYPE_INTEGER
    end
    { sqrt = 115,113,114,116 }
    else if TokIs8(115, 113, 114, 116, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Convert to real if integer }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { fsqrt d0, d0 }
      writeln('    fsqrt d0, d0');
      expr_type := TYPE_REAL
    end
    { round = 114,111,117,110,100 }
    else if (tok_len = 5) and (tok_str[0] = 114) and (tok_str[1] = 111) and
            (tok_str[2] = 117) and (tok_str[3] = 110) and (tok_str[4] = 100) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Convert to real if integer }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { fcvtas x0, d0 - round to nearest with ties to away }
      writeln('    fcvtas x0, d0');
      expr_type := TYPE_INTEGER
    end
    { trunc = 116,114,117,110,99 }
    else if (tok_len = 5) and (tok_str[0] = 116) and (tok_str[1] = 114) and
            (tok_str[2] = 117) and (tok_str[3] = 110) and (tok_str[4] = 99) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Convert to real if integer }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { fcvtzs x0, d0 - truncate toward zero }
      writeln('    fcvtzs x0, d0');
      expr_type := TYPE_INTEGER
    end
    { sin = 115,105,110 }
    else if TokIs8(115, 105, 110, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_sin);
      expr_type := TYPE_REAL
    end
    { cos = 99,111,115 }
    else if TokIs8(99, 111, 115, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_cos);
      expr_type := TYPE_REAL
    end
    { tan = 116,97,110 }
    else if TokIs8(116, 97, 110, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_tan);
      expr_type := TYPE_REAL
    end
    { exp = 101,120,112 }
    else if TokIs8(101, 120, 112, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_exp);
      expr_type := TYPE_REAL
    end
    { ln = 108,110 }
    else if (tok_len = 2) and (tok_str[0] = 108) and (tok_str[1] = 110) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_ln);
      expr_type := TYPE_REAL
    end
    { random = 114,97,110,100,111,109 }
    else if (tok_len = 6) and (tok_str[0] = 114) and (tok_str[1] = 97) and
            (tok_str[2] = 110) and (tok_str[3] = 100) and (tok_str[4] = 111) and
            (tok_str[5] = 109) then
    begin
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        { random(n) - returns 0..n-1 }
        NextToken;
        if tok_type <> TOK_RPAREN then
        begin
          ParseExpression;
          EmitPushX0;
          EmitBL(rt_random);
          EmitPopX1;
          { x0 mod x1 using udiv and msub }
          writeln('    udiv x2, x0, x1');
          writeln('    msub x0, x2, x1, x0');
        end
        else
        begin
          { random() with no arg - return real 0.0..1.0 }
          EmitBL(rt_random);
          { Convert to real and divide by 2^64 }
          EmitScvtfD0X0;
          { Load 2^63 as divisor (since signed, use 2^63) }
          writeln('    movz x1, #0x8000, lsl #48');
          writeln('    scvtf d1, x1');
          writeln('    fdiv d0, d0, d1');
          { Make positive by using fabs }
          writeln('    fabs d0, d0');
          expr_type := TYPE_REAL
        end;
        Expect(TOK_RPAREN)
      end
      else
      begin
        { random with no parens - return real 0.0..1.0 }
        EmitBL(rt_random);
        EmitScvtfD0X0;
        writeln('    movz x1, #0x8000, lsl #48');
        writeln('    scvtf d1, x1');
        writeln('    fdiv d0, d0, d1');
        writeln('    fabs d0, d0');
        expr_type := TYPE_REAL
      end;
      if expr_type <> TYPE_REAL then
        expr_type := TYPE_INTEGER
    end
    { pi = 112,105 - returns 3.14159265358979 }
    else if (tok_len = 2) and (tok_str[0] = 112) and (tok_str[1] = 105) then
    begin
      NextToken;
      { Load pi = 3.14159265358979323846 into d0 }
      { IEEE 754 double: 0x400921FB54442D18 }
      writeln('    movz x0, #0x2D18');
      writeln('    movk x0, #0x5444, lsl #16');
      writeln('    movk x0, #0x21FB, lsl #32');
      writeln('    movk x0, #0x4009, lsl #48');
      EmitFmovD0X0;
      expr_type := TYPE_REAL
    end
    { frac = 102,114,97,99 - fractional part }
    else if TokIs8(102, 114, 97, 99, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { frac(x) = x - trunc(x) }
      { Save x to stack }
      EmitPushD0;
      { Truncate to integer }
      EmitFcvtzsX0D0;
      { Convert back to float }
      EmitScvtfD0X0;
      { Pop original, now d1 = trunc(x) }
      writeln('    fmov d1, d0');
      EmitPopD0;
      { d0 = x - trunc(x) }
      writeln('    fsub d0, d0, d1');
      expr_type := TYPE_REAL
    end
    { int = 105,110,116 - integer part as real (different from trunc) }
    else if TokIs8(105, 110, 116, 0, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { int(x) = trunc(x) as float }
      { frintz d0, d0 - round toward zero (truncate) }
      writeln('    frintz d0, d0');
      expr_type := TYPE_REAL
    end
    { arctan = 97,114,99,116,97,110 }
    else if (tok_len = 6) and (tok_str[0] = 97) and (tok_str[1] = 114) and
            (tok_str[2] = 99) and (tok_str[3] = 116) and (tok_str[4] = 97) and
            (tok_str[5] = 110) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_arctan);
      expr_type := TYPE_REAL
    end
    { arcsin = 97,114,99,115,105,110 }
    else if (tok_len = 6) and (tok_str[0] = 97) and (tok_str[1] = 114) and
            (tok_str[2] = 99) and (tok_str[3] = 115) and (tok_str[4] = 105) and
            (tok_str[5] = 110) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_arcsin);
      expr_type := TYPE_REAL
    end
    { arccos = 97,114,99,99,111,115 }
    else if (tok_len = 6) and (tok_str[0] = 97) and (tok_str[1] = 114) and
            (tok_str[2] = 99) and (tok_str[3] = 99) and (tok_str[4] = 111) and
            (tok_str[5] = 115) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitBL(rt_arccos);
      expr_type := TYPE_REAL
    end
    { log10 = 108,111,103,49,48 }
    else if (tok_len = 5) and (tok_str[0] = 108) and (tok_str[1] = 111) and
            (tok_str[2] = 103) and (tok_str[3] = 49) and (tok_str[4] = 48) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { log10(x) = ln(x) / ln(10) }
      EmitBL(rt_ln);
      EmitPushD0;
      { Load ln(10) = 2.302585... into d0 }
      writeln('    movz x0, #0x634A');
      writeln('    movk x0, #0x9BFE, lsl #16');
      writeln('    movk x0, #0x66D3, lsl #32');
      writeln('    movk x0, #0x4002, lsl #48');
      EmitFmovD0X0;
      EmitPopD1;
      { d0 = ln(x) / ln(10) }
      writeln('    fdiv d0, d1, d0');
      expr_type := TYPE_REAL
    end
    { log2 = 108,111,103,50 }
    else if TokIs8(108, 111, 103, 50, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { log2(x) = ln(x) / ln(2) }
      EmitBL(rt_ln);
      EmitPushD0;
      { Load ln(2) = 0.693147... into d0 }
      writeln('    movz x0, #0x39EF');
      writeln('    movk x0, #0xFEFA, lsl #16');
      writeln('    movk x0, #0x2E42, lsl #32');
      writeln('    movk x0, #0x3FE6, lsl #48');
      EmitFmovD0X0;
      EmitPopD1;
      { d0 = ln(x) / ln(2) }
      writeln('    fdiv d0, d1, d0');
      expr_type := TYPE_REAL
    end
    { power = 112,111,119,101,114 }
    else if (tok_len = 5) and (tok_str[0] = 112) and (tok_str[1] = 111) and
            (tok_str[2] = 119) and (tok_str[3] = 101) and (tok_str[4] = 114) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { base }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      EmitPushD0;
      Expect(TOK_COMMA);
      ParseExpression;  { exponent }
      if expr_type <> TYPE_REAL then
        EmitScvtfD0X0;
      { d0 = exp, stack has base }
      { power(b, e) = exp(e * ln(b)) }
      { Move exp to d1, get base to d0, save exp on stack }
      EmitPushD0;       { stack = [exp, base] }
      EmitPopD1;        { d1 = exp, stack = [base] }
      EmitPopD0;        { d0 = base, stack = [] }
      { Push d1 (exp) to stack - str d1, [sp, #-16]! }
      writeln('    str d1, [sp, #-16]!');
      { d0 = base, stack = [exp] }
      EmitBL(rt_ln);    { d0 = ln(base), stack = [exp] }
      EmitPopD1;        { d1 = exp, stack = [] }
      { fmul d0, d0, d1 - d0 = ln(base) * exp }
      writeln('    fmul d0, d0, d1');
      EmitBL(rt_exp);   { d0 = exp(ln(base) * exp) = base^exp }
      Expect(TOK_RPAREN);
      expr_type := TYPE_REAL
    end
    { succ = 115,117,99,99 }
    else if TokIs8(115, 117, 99, 99, 0, 0, 0, 0) = 1 then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { Return x + 1 }
      writeln('    add x0, x0, #1');
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
      writeln('    sub x0, x0, #1');
      expr_type := TYPE_INTEGER
    end
    { sizeof = 115,105,122,101,111,102 }
    else if (tok_len = 6) and (tok_str[0] = 115) and (tok_str[1] = 105) and
            (tok_str[2] = 122) and (tok_str[3] = 101) and (tok_str[4] = 111) and
            (tok_str[5] = 102) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      { Check for type name or variable }
      if tok_type = TOK_INTEGER_TYPE then
      begin
        EmitMovX0(8);
        NextToken
      end
      else if tok_type = TOK_CHAR_TYPE then
      begin
        EmitMovX0(1);
        NextToken
      end
      else if tok_type = TOK_BOOLEAN_TYPE then
      begin
        EmitMovX0(1);
        NextToken
      end
      else if tok_type = TOK_REAL_TYPE then
      begin
        EmitMovX0(8);
        NextToken
      end
      else if tok_type = TOK_STRING_TYPE then
      begin
        EmitMovX0(256);
        NextToken
      end
      else if tok_type = TOK_IDENT then
      begin
        { Look up identifier - could be variable or type name }
        idx := SymLookup;
        if idx < 0 then
          Error(3);
        if sym_kind[idx] = SYM_TYPEDEF then
        begin
          { Type definition - get size from sym_label (record size) }
          EmitMovX0(sym_label[idx])
        end
        else if sym_type[idx] = TYPE_INTEGER then
          EmitMovX0(8)
        else if sym_type[idx] = TYPE_CHAR then
          EmitMovX0(1)  { Logical size, not storage size }
        else if sym_type[idx] = TYPE_BOOLEAN then
          EmitMovX0(1)  { Logical size, not storage size }
        else if sym_type[idx] = TYPE_REAL then
          EmitMovX0(8)
        else if sym_type[idx] = TYPE_STRING then
          EmitMovX0(256)
        else if sym_type[idx] = TYPE_POINTER then
          EmitMovX0(8)
        else if sym_type[idx] = TYPE_ARRAY then
        begin
          { Array: sym_label already contains total size in bytes }
          EmitMovX0(sym_label[idx])
        end
        else if sym_type[idx] = TYPE_RECORD then
        begin
          { Record: sym_const_val has typedef index, get size from typedef's sym_label }
          EmitMovX0(sym_label[sym_const_val[idx]])
        end
        else
          EmitMovX0(8);  { Default to 8 bytes }
        NextToken
      end
      else
        Error(9);
      Expect(TOK_RPAREN);
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
      writeln('    cmp x0, #97');
      write('    b.lt L'); writeln(label_count);
      writeln('    cmp x0, #122');
      write('    b.gt L'); writeln(label_count);
      writeln('    sub x0, x0, #32');
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
      writeln('    cmp x0, #65');
      write('    b.lt L'); writeln(label_count);
      writeln('    cmp x0, #90');
      write('    b.gt L'); writeln(label_count);
      writeln('    add x0, x0, #32');
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
        writeln('    ldrb w0, [x0]');
      end
      else
        Error(9);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    end
    { eof = 101,111,102 }
    else if (tok_len = 3) and (tok_str[0] = 101) and (tok_str[1] = 111) and
            (tok_str[2] = 102) then
    begin
      { eof(f) - check if at end of file }
      { Uses lseek to compare current position with file size }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get file fd and save for later }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      writeln('    ldr x0, [x0]');
      { Save fd to x23 (callee-saved) }
      writeln('    mov x23, x0');
      { lseek(fd, 0, SEEK_CUR=1) to get current position }
      writeln('    mov x1, #0');
      writeln('    mov x2, #1');
      { lseek syscall: 0x20000C7 }
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = current position, save to x24 }
      writeln('    mov x24, x0');
      { lseek(fd, 0, SEEK_END=2) to get file size }
      writeln('    mov x0, x23');
      writeln('    mov x1, #0');
      writeln('    mov x2, #2');
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = file size, x24 = current position }
      { Restore file position: lseek(fd, current_pos, SEEK_SET=0) }
      writeln('    mov x25, x0');
      writeln('    mov x0, x23');
      writeln('    mov x1, x24');
      writeln('    mov x2, #0');
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Compare: x24 = current_pos, x25 = file_size }
      { cmp x24, x25 }
      writeln('    cmp x24, x25');
      { cset x0, ge - x0 = 1 if current_pos >= file_size }
      writeln('    cset x0, ge');
      expr_type := TYPE_BOOLEAN
    end
    { filepos = 102,105,108,101,112,111,115 }
    else if (tok_len = 7) and (tok_str[0] = 102) and (tok_str[1] = 105) and
            (tok_str[2] = 108) and (tok_str[3] = 101) and (tok_str[4] = 112) and
            (tok_str[5] = 111) and (tok_str[6] = 115) then
    begin
      { filepos(f) - get current position in file }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get file fd }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      writeln('    ldr x0, [x0]');
      { lseek(fd, 0, SEEK_CUR=1) to get current position }
      writeln('    mov x1, #0');
      writeln('    mov x2, #1');
      { lseek syscall: 0x20000C7 }
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = current position }
      expr_type := TYPE_INTEGER
    end
    { filesize = 102,105,108,101,115,105,122,101 }
    else if (tok_len = 8) and (tok_str[0] = 102) and (tok_str[1] = 105) and
            (tok_str[2] = 108) and (tok_str[3] = 101) and (tok_str[4] = 115) and
            (tok_str[5] = 105) and (tok_str[6] = 122) and (tok_str[7] = 101) then
    begin
      { filesize(f) - get file size }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get file fd and save for later }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      writeln('    ldr x0, [x0]');
      { Save fd to x23 }
      writeln('    mov x23, x0');
      { lseek(fd, 0, SEEK_CUR=1) to get current position (to restore later) }
      writeln('    mov x1, #0');
      writeln('    mov x2, #1');
      { lseek syscall: 0x20000C7 }
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = current position, save to x24 }
      writeln('    mov x24, x0');
      { lseek(fd, 0, SEEK_END=2) to get file size }
      writeln('    mov x0, x23');
      writeln('    mov x1, #0');
      writeln('    mov x2, #2');
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { x0 = file size, save to x25 }
      writeln('    mov x25, x0');
      { Restore file position: lseek(fd, current_pos, SEEK_SET=0) }
      writeln('    mov x0, x23');
      writeln('    mov x1, x24');
      writeln('    mov x2, #0');
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Move file size to x0 as return value }
      writeln('    mov x0, x25');
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
      writeln('    mov x3, x0');
      EmitPopX1;  { start }
      EmitPopX0;  { source addr -> x2 }
      writeln('    mov x2, x0');
      { Allocate temp string from heap: x0 = x21, x21 += 256 }
      writeln('    mov x0, x21');
      writeln('    add x21, x21, #256');
      { Save dest addr }
      EmitPushX0;
      { Inline copy logic: copy from source[start] to dest, count bytes }
      { x0 = dest, x1 = start, x2 = source, x3 = count }
      { Store count as length at dest[0] }
      writeln('    strb w3, [x0]');
      { Loop: copy count bytes from source[start+i] to dest[1+i] }
      lbl1 := NewLabel;
      writeln('    mov x4, #0');
      EmitLabel(lbl1);
      { cmp x4, x3 }
      writeln('    cmp x4, x3');
      { b.ge done }
      write('    b.ge L'); writeln(label_count);
      { x5 = start + x4 (source index) }
      writeln('    add x5, x1, x4');
      { ldrb w6, [x2, x5] }
      writeln('    ldrb w6, [x2, x5]');
      { x5 = x4 + 1 (dest index) }
      writeln('    add x5, x4, #1');
      { strb w6, [x0, x5] }
      writeln('    strb w6, [x0, x5]');
      { x4 = x4 + 1 }
      writeln('    add x4, x4, #1');
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
        writeln('    mov x2, x0');
        EmitPopX1;  { string1 }
      end
      else
        Error(9);
      { Allocate temp string from heap: x0 = x21, x21 += 256 }
      writeln('    mov x0, x21');
      writeln('    add x21, x21, #256');
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
        writeln('    mov x8, x21');
        EmitMovX0(tok_len);
        writeln('    strb w0, [x8]');
        for i := 0 to tok_len - 1 do
        begin
          EmitMovX0(tok_str[i]);
          write('    strb w0, [x8, #'); write(i + 1); writeln(']');
        end;
        writeln('    mov x0, x21');
        writeln('    add x21, x21, #256');
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
        writeln('    mov x1, x0');
        EmitPopX0  { substr in x0 }
      end
      else
        Error(9);
      { Call rt_str_pos(x0=substr, x1=string) }
      EmitBL(rt_str_pos);
      Expect(TOK_RPAREN);
      expr_type := TYPE_INTEGER
    end
    { paramcount = 112,97,114,97,109,99,111,117,110,116 }
    else if (tok_len = 10) and (tok_str[0] = 112) and (tok_str[1] = 97) and
            (tok_str[2] = 114) and (tok_str[3] = 97) and (tok_str[4] = 109) and
            (tok_str[5] = 99) and (tok_str[6] = 111) and (tok_str[7] = 117) and
            (tok_str[8] = 110) and (tok_str[9] = 116) then
    begin
      { paramcount - returns argc - 1 (number of command-line parameters) }
      NextToken;
      { x25 holds argc, return argc - 1 }
      writeln('    sub x0, x25, #1');
      expr_type := TYPE_INTEGER
    end
    { paramstr = 112,97,114,97,109,115,116,114 }
    else if (tok_len = 8) and (tok_str[0] = 112) and (tok_str[1] = 97) and
            (tok_str[2] = 114) and (tok_str[3] = 97) and (tok_str[4] = 109) and
            (tok_str[5] = 115) and (tok_str[6] = 116) and (tok_str[7] = 114) then
    begin
      { paramstr(n) - returns argv[n] as Pascal string }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { n in x0 }
      Expect(TOK_RPAREN);
      EmitBL(rt_paramstr);
      expr_type := TYPE_STRING
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
            writeln('    add x0, x8, #');
          end
          else
          begin
            { add x0, x29, #sym_offset + field_offset }
            writeln('    add x0, x29, #');
          end;
          { Load from computed address }
          if field_type[arg_count] = TYPE_REAL then
          begin
            writeln('    ldr d0, [x0]');
            expr_type := TYPE_REAL
          end
          else
          begin
            writeln('    ldr x0, [x0]');
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
          writeln('    sub x0, x1, x0');
          { Check if array of records }
          if sym_var_param_flags[idx] > 0 then
          begin
            { Array of records - multiply by record size }
            lbl1 := sym_label[sym_var_param_flags[idx] - 1];  { record size }
            EmitPushX0;
            EmitMovX0(lbl1);
            EmitPopX1;
            { mul x0, x1, x0 }
            writeln('    mul x0, x1, x0');
          end
          else
          begin
            { Basic type - multiply by 8 using lsl #3 }
            writeln('    lsl x0, x0, #3');
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
          writeln('    sub x1, x1, x0');
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
              writeln('    add x1, x1, #');
            end;
            { Load field value }
            if field_type[arg_count] = TYPE_REAL then
            begin
              writeln('    ldr d0, [x1]');
              expr_type := TYPE_REAL
            end
            else
            begin
              writeln('    ldr x0, [x1]');
              expr_type := field_type[arg_count]
            end
          end
          else
          begin
            { Load element value }
            writeln('    ldr x0, [x1]');
          end
        end
        else if (sym_type[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
        begin
          { Record field access: rec.field or rec.field.subfield... }
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
            writeln('    add x0, x8, #');
          end
          else
          begin
            { add x0, x29, #sym_offset + field_offset }
            writeln('    add x0, x29, #');
          end;
          NextToken;
          { Handle nested record fields: x0 has address, check for more dots }
          while (field_type[arg_count] = TYPE_RECORD) and (tok_type = TOK_DOT) do
          begin
            NextToken;  { consume '.' }
            if tok_type <> TOK_IDENT then
              Error(11);
            { Find sub-field in the nested record type }
            lbl1 := arg_count;  { save current field index }
            arg_count := FindField(field_rec_type[lbl1]);
            if arg_count < 0 then
              Error(15);
            { Add sub-field offset to x0 }
            if field_offset[arg_count] > 0 then
            begin
              writeln('    add x0, x0, #');
            end;
            NextToken
          end;
          { Load from final computed address }
          if field_type[arg_count] = TYPE_REAL then
          begin
            writeln('    ldr d0, [x0]');
            expr_type := TYPE_REAL
          end
          else if field_type[arg_count] = TYPE_RECORD then
          begin
            { Accessing a nested record as a whole - keep address in x0 }
            expr_type := TYPE_RECORD
          end
          else
          begin
            writeln('    ldr x0, [x0]');
            expr_type := field_type[arg_count]
          end
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
              writeln('    ldr d0, [x0]');
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
              writeln('    add x0, x1, x0');
              { Load byte at [x0]: ldrb w0, [x0] }
              writeln('    ldrb w0, [x0]');
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
              writeln('    ldr x0, [x0]');
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
          { Count consecutive ^ tokens }
          lbl1 := 0;  { deref count }
          while tok_type = TOK_CARET do
          begin
            lbl1 := lbl1 + 1;
            NextToken
          end;
          { lbl2 = remaining depth after derefs }
          lbl2 := ptr_depth[idx] - lbl1;
          { Emit dereference operations }
          { For pointer-to-array, don't emit final deref - array base is the pointer value }
          { For pointer-to-record with field access, don't emit final deref - record base is the pointer value }
          if (lbl2 = 0) and (ptr_ultimate_type[idx] = TYPE_ARRAY) then
            arg_count := lbl1 - 1
          else if (lbl2 = 0) and (ptr_ultimate_type[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
            arg_count := lbl1 - 1
          else
            arg_count := lbl1;
          for i := 1 to arg_count do
          begin
            writeln('    ldr x0, [x0]');
          end;
          { Determine result type }
          if lbl2 > 0 then
          begin
            { Still have pointer levels remaining }
            expr_type := TYPE_POINTER;
            ptr_base_type := TYPE_POINTER
          end
          else if ptr_ultimate_type[idx] = TYPE_RECORD then
          begin
            { Reached record - check for field access }
            if tok_type = TOK_DOT then
            begin
              NextToken;  { consume '.' }
              { Find field in the record type }
              arg_count := FindField(ptr_ultimate_rec[idx]);
              if arg_count < 0 then
                Error(11);  { unknown field }
              NextToken;  { consume field name }
              { Add field offset to pointer: add x0, x0, #offset }
              if field_offset[arg_count] > 0 then
              begin
                writeln('    add x0, x0, #');
              end;
              { Handle nested record fields }
              while (field_type[arg_count] = TYPE_RECORD) and (tok_type = TOK_DOT) do
              begin
                NextToken;  { consume '.' }
                if tok_type <> TOK_IDENT then
                  Error(11);
                var_arg_idx := arg_count;  { save current field index }
                arg_count := FindField(field_rec_type[var_arg_idx]);
                if arg_count < 0 then
                  Error(15);
                if field_offset[arg_count] > 0 then
                begin
                  writeln('    add x0, x0, #');
                end;
                NextToken
              end;
              { Load field value }
              if field_type[arg_count] = TYPE_REAL then
              begin
                writeln('    ldr d0, [x0]');
                expr_type := TYPE_REAL
              end
              else if field_type[arg_count] = TYPE_RECORD then
                expr_type := TYPE_RECORD
              else
              begin
                writeln('    ldr x0, [x0]');
                expr_type := field_type[arg_count]
              end
            end
            else
            begin
              { Just p^ without field access - address in x0 }
              expr_type := TYPE_RECORD
            end
          end
          else if ptr_ultimate_type[idx] = TYPE_ARRAY then
          begin
            { Pointer to array - check for indexing pa^[i] }
            if tok_type = TOK_LBRACKET then
            begin
              NextToken;  { consume '[' }
              EmitPushX0;  { save array base address }
              ParseExpression;  { index in x0 }
              Expect(TOK_RBRACKET);
              { x0 = index, stack has base address }
              { Subtract low bound }
              arg_count := sym_label[idx];  { ptr_arr index }
              EmitPushX0;
              EmitMovX0(ptr_arr_lo[arg_count]);
              EmitPopX1;
              { x0 = x1 - x0 = index - low_bound }
              writeln('    sub x0, x1, x0');
              { Multiply by 8 using lsl #3 }
              writeln('    lsl x0, x0, #3');
              { Add to base: x1 = base + offset }
              EmitPopX1;  { x1 = base address }
              writeln('    add x0, x1, x0');
              { Load element value }
              if ptr_arr_elem[arg_count] = TYPE_REAL then
              begin
                writeln('    ldr d0, [x0]');
                expr_type := TYPE_REAL
              end
              else
              begin
                writeln('    ldr x0, [x0]');
                expr_type := ptr_arr_elem[arg_count]
              end
            end
            else
              expr_type := TYPE_ARRAY  { Just pa^ without indexing }
          end
          else if ptr_ultimate_type[idx] = TYPE_REAL then
          begin
            expr_type := TYPE_REAL
          end
          else
          begin
            expr_type := ptr_ultimate_type[idx]
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
                  writeln('    sub x0, x1, x0');
                  { Multiply by 8 using lsl #3 }
                  writeln('    lsl x0, x0, #3');
                  { Get base address and subtract element offset }
                  if sym_level[var_arg_idx] < scope_level then
                  begin
                    EmitFollowChain(sym_level[var_arg_idx], scope_level);
                    EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                  end
                  else
                    EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                  { Address = base - element_offset }
                  writeln('    sub x0, x1, x0');
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
          write('    ldr x'); write(i); writeln(', [sp], #16');
        end;
        { Set up static link for callee }
        EmitStaticLink(sym_level[idx], scope_level);
        { Check if calling imported unit procedure }
        if sym_unit_idx[idx] >= 0 then
          EmitBLUnitProc(sym_unit_idx[idx], idx)
        else
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
  op, left_type, and_skip_label, had_and: integer;
begin
  ParseUnary;
  had_and := 0;
  and_skip_label := 0;
  while (tok_type = TOK_STAR) or (tok_type = TOK_DIV) or (tok_type = TOK_MOD) or
        (tok_type = TOK_AND) or (tok_type = TOK_SLASH) do
  begin
    op := tok_type;
    left_type := expr_type;
    NextToken;

    if op = TOK_AND then
    begin
      { Short-circuit AND: if current value is 0, skip rest }
      if had_and = 0 then
      begin
        and_skip_label := NewLabel;
        had_and := 1
      end;
      { cbz x0, .Lskip - branch if zero }
      EmitBranchLabelZ(and_skip_label);
      ParseUnary;
      expr_type := TYPE_INTEGER
    end
    else
    begin
      { Regular operators - push left, eval right, pop, compute }
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
          Error(13);  { mod not supported for reals }
        if (op = TOK_STAR) then
          expr_type := TYPE_REAL
      end
      else if (left_type = TYPE_SET) or (expr_type = TYPE_SET) then
      begin
        { Set intersection: x0 = x1 AND x0 }
        EmitPopX1;
        writeln('    and x0, x1, x0');
        expr_type := TYPE_SET
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
        end;
        expr_type := TYPE_INTEGER
      end
    end
  end;

  { Emit skip label for short-circuit AND if we had any }
  if had_and = 1 then
    EmitLabel(and_skip_label)
end;

procedure ParseSimpleExpr;
var
  op, left_type, left_ptr_base, or_true_label, or_end_label, had_or: integer;
begin
  ParseTerm;
  had_or := 0;
  or_true_label := 0;
  or_end_label := 0;
  while (tok_type = TOK_PLUS) or (tok_type = TOK_MINUS) or (tok_type = TOK_OR) do
  begin
    op := tok_type;
    left_type := expr_type;
    left_ptr_base := ptr_base_type;
    NextToken;

    if op = TOK_OR then
    begin
      { Short-circuit OR: if current value is non-zero, skip rest with result 1 }
      if had_or = 0 then
      begin
        or_true_label := NewLabel;
        or_end_label := NewLabel;
        had_or := 1
      end;
      { cbnz x0, .Ltrue - branch if not zero }
      EmitBranchLabelNZ(or_true_label);
      ParseTerm;
      expr_type := TYPE_INTEGER
    end
    else
    begin
      { Regular operators (+, -) - push left, eval right, pop, compute }
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
      writeln('    lsl x0, x0, #3');
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
        writeln('    asr x0, x0, #3');
        expr_type := TYPE_INTEGER
      end
      else
      begin
        { pointer - integer: scale integer by 8 and ADD (arrays grow downward) }
        EmitPopX1;  { pointer in x1 }
        { x0 has integer offset, multiply by 8 using lsl #3 }
        writeln('    lsl x0, x0, #3');
        EmitAdd;  { x0 = x1 + x0 (add because arrays grow downward) }
        expr_type := TYPE_POINTER;
        ptr_base_type := left_ptr_base
      end
    end
    else if (left_type = TYPE_STRING) and (expr_type = TYPE_STRING) and (op = TOK_PLUS) then
    begin
      { String concatenation: str1 + str2 }
      { x0 = string2 addr, stack top = string1 addr }
      writeln('    mov x2, x0');
      EmitPopX1;  { string1 addr }
      { Allocate temp string from heap }
      writeln('    mov x0, x21');
      writeln('    add x21, x21, #256');
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
        writeln('    mov x3, x0');
        writeln('    mov x2, x21');
        EmitMovX0(1);  { length = 1 }
        writeln('    strb w0, [x2]');
        writeln('    strb w3, [x2, #1]');
        writeln('    add x21, x21, #256');
        EmitPopX1;  { string1 addr }
        { x2 = char string, x1 = string1, allocate result on heap }
        writeln('    mov x0, x21');
        writeln('    add x21, x21, #256');
        EmitBL(rt_str_concat)
      end
      else
      begin
        { char + string: x0 = string addr, stack top = char }
        writeln('    mov x2, x0');
        EmitPopX0;  { x0 = char }
        { Create single-char string from x0 on heap }
        writeln('    mov x1, x21');
        writeln('    mov x3, x0');
        EmitMovX0(1);  { length = 1 }
        writeln('    strb w0, [x1]');
        writeln('    strb w3, [x1, #1]');
        writeln('    add x21, x21, #256');
        { x1 = char string, x2 = string2, allocate result on heap }
        writeln('    mov x0, x21');
        writeln('    add x21, x21, #256');
        EmitBL(rt_str_concat)
      end;
      expr_type := TYPE_STRING
    end
    else if (left_type = TYPE_SET) or (expr_type = TYPE_SET) then
    begin
      { Set operations: + is union, - is difference }
      EmitPopX1;  { left set in x1 }
      if op = TOK_PLUS then
      begin
        { Union: x0 = x1 OR x0 }
        writeln('    orr x0, x1, x0');
      end
      else if op = TOK_MINUS then
      begin
        { Difference: x0 = x1 AND NOT x0 }
        { First: mvn x0, x0 (NOT x0) }
        writeln('    mvn x0, x0');
        { Then: and x0, x1, x0 }
        writeln('    and x0, x1, x0');
      end;
      expr_type := TYPE_SET
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
      { Both integers (only + and - reach here) }
      EmitPopX1;
      if op = TOK_PLUS then
        EmitAdd
      else
        EmitSub;
      expr_type := TYPE_INTEGER
    end
    end  { end of else for non-OR operators }
  end;

  { Emit labels for short-circuit OR if we had any }
  if had_or = 1 then
  begin
    { b .Lend - skip the mov x0, #1 }
    EmitBranchLabel(or_end_label);
    EmitLabel(or_true_label);
    EmitMovX0(1);
    EmitLabel(or_end_label)
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
      writeln('    mov x1, x0');
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
          writeln('    cmp x0, #0');
          writeln('    cset x0, lt');
        end
        else if op = TOK_GT then
        begin
          { cmp x0, #0; cset x0, gt }
          writeln('    cmp x0, #0');
          writeln('    cset x0, gt');
        end
        else if op = TOK_LE then
        begin
          { cmp x0, #0; cset x0, le }
          writeln('    cmp x0, #0');
          writeln('    cset x0, le');
        end
        else if op = TOK_GE then
        begin
          { cmp x0, #0; cset x0, ge }
          writeln('    cmp x0, #0');
          writeln('    cset x0, ge');
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
  else if tok_type = TOK_IN then
  begin
    { Set membership: value in set }
    { x0 = value, need to check if bit is set in the set }
    EmitPushX0;  { push value }
    NextToken;
    ParseSimpleExpr;  { set in x0 }
    { x0 = set, stack = value }
    { Result: (set >> value) & 1 }
    EmitPopX1;  { value in x1 }
    { lsr x0, x0, x1 }
    writeln('    lsr x0, x0, x1');
    { and x0, x0, #1 }
    EmitAndImm(1);
    expr_type := TYPE_BOOLEAN
  end
end;

procedure ParseStatement;
var
  idx, lbl1, lbl2, lbl3, arg_count, i: integer;
  var_flags, arg_idx, var_arg_idx: integer;
  old_break, old_continue: integer;
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
    lbl1 := NewLabel;  { loop start / continue target }
    lbl2 := NewLabel;  { loop end / break target }
    { Save old break/continue labels }
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
    { Restore old break/continue labels }
    break_label := old_break;
    continue_label := old_continue
  end
  else if tok_type = TOK_REPEAT then
  begin
    lbl1 := NewLabel;  { loop start / continue target }
    lbl2 := NewLabel;  { loop end / break target }
    { Save old break/continue labels }
    old_break := break_label;
    old_continue := continue_label;
    break_label := lbl2;
    continue_label := lbl1;
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
    EmitBranchLabelZ(lbl1);
    EmitLabel(lbl2);
    { Restore old break/continue labels }
    break_label := old_break;
    continue_label := old_continue
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

    lbl1 := NewLabel;  { condition check }
    lbl2 := NewLabel;  { loop end / break target }
    lbl3 := NewLabel;  { increment / continue target }

    { Save old break/continue labels }
    old_break := break_label;
    old_continue := continue_label;
    break_label := lbl2;
    continue_label := lbl3;

    if tok_type = TOK_TO then
    begin
      NextToken;
      ParseExpression;  { end value into x0 }
      EmitPushX0;       { save end value on stack }
      Expect(TOK_DO);
      EmitLabel(lbl1);
      EmitLdurX0(sym_offset[idx]);  { load loop var }
      { ldur x1, [sp] - load end value from stack }
      writeln('    ldur x1, [sp]');
      EmitCmpX0X1;
      EmitCset(2);  { lt: exit when end < i, meaning i > end }
      EmitBranchLabelNZ(lbl2);
      ParseStatement;
      { continue target - increment }
      EmitLabel(lbl3);
      EmitLdurX0(sym_offset[idx]);
      writeln('    add x0, x0, #1');
      EmitSturX0(sym_offset[idx]);
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { Restore old break/continue labels }
      break_label := old_break;
      continue_label := old_continue;
      { Pop end value from stack }
      writeln('    add sp, sp, #16');
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
      writeln('    ldur x1, [sp]');
      EmitCmpX0X1;
      EmitCset(4);  { gt: exit when end > i, meaning i < end }
      EmitBranchLabelNZ(lbl2);
      ParseStatement;
      { continue target - decrement }
      EmitLabel(lbl3);
      EmitLdurX0(sym_offset[idx]);
      writeln('    sub x0, x0, #1');
      EmitSturX0(sym_offset[idx]);
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { Restore old break/continue labels }
      break_label := old_break;
      continue_label := old_continue;
      { Pop end value from stack }
      writeln('    add sp, sp, #16');
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
        writeln('    ldr x1, [sp]');
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
        write('    b.eq L'); writeln(lbl3);
      until tok_type = TOK_COLON;
      { No match in this group, jump to next branch }
      EmitBranchLabel(lbl2);
      { Match found - execute statement }
      EmitLabel(lbl3);
      NextToken;  { consume ':' }
      { Pop selector from stack (discard) }
      writeln('    add sp, sp, #16');
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
      writeln('    add sp, sp, #16');
      ParseStatement;
      { Note: no need to jump to lbl1, we fall through }
    end
    else
    begin
      { No else - just pop selector }
      writeln('    add sp, sp, #16');
    end;
    EmitLabel(lbl1);
    Expect(TOK_END)
  end
  else if tok_type = TOK_READ then
  begin
    { read([f,] var) - reads an integer or real into a variable }
    NextToken;
    Expect(TOK_LPAREN);
    lbl1 := 0;  { flag: 1 if reading from file }
    if tok_type <> TOK_IDENT then
      Error(6);
    idx := SymLookup;
    if idx < 0 then
      Error(3);
    { Check if first arg is a file variable }
    if (sym_type[idx] = TYPE_FILE) or (sym_type[idx] = TYPE_TEXT) then
    begin
      { File variable - save x19 and load file's fd }
      lbl1 := 1;
      NextToken;
      { Push x19 to save it }
      writeln('    str x19, [sp, #-16]!');
      { Load file's fd into x19 }
      EmitVarAddr(idx, scope_level);
      writeln('    ldr x19, [x0]');
      Expect(TOK_COMMA);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3)
    end;
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
    { Restore x19 if we saved it }
    if lbl1 = 1 then
    begin
      writeln('    ldr x19, [sp], #16');
    end;
    Expect(TOK_RPAREN)
  end
  else if tok_type = TOK_READLN then
  begin
    { readln([f,] var) or readln([f]) - reads integer/real/string and skips to end of line }
    NextToken;
    lbl1 := 0;  { flag: 1 if reading from file }
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
        { Check if first arg is a file variable }
        if (sym_type[idx] = TYPE_FILE) or (sym_type[idx] = TYPE_TEXT) then
        begin
          { File variable - save x19 and load file's fd }
          lbl1 := 1;
          NextToken;
          { Push x19 to save it }
          writeln('    str x19, [sp, #-16]!');
          { Load file's fd into x19 }
          EmitVarAddr(idx, scope_level);
          writeln('    ldr x19, [x0]');
          { Check for comma (more args) or just skip to rparen }
          if tok_type = TOK_COMMA then
          begin
            NextToken;
            if tok_type <> TOK_IDENT then
              Error(6);
            idx := SymLookup;
            if idx < 0 then
              Error(3)
          end
          else
          begin
            { Just file arg, no variable to read into - skip to rparen }
            Expect(TOK_RPAREN);
            { Skip to end of line }
            EmitBL(rt_skip_line);
            { Restore x19 }
            writeln('    ldr x19, [sp], #16');
          end
        end;
        { Only process variable if we have one }
        if tok_type = TOK_IDENT then
        begin
          idx := SymLookup;
          if idx < 0 then
            Error(3);
          lbl2 := sym_type[idx];  { save type for skip_line decision }
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
            { Call read_string runtime - already consumes newline }
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
          end;
          Expect(TOK_RPAREN);
          { Skip to end of line only for integer/real (read_string already consumed newline) }
          if lbl2 <> TYPE_STRING then
            EmitBL(rt_skip_line);
          { Restore x19 if we saved it }
          if lbl1 = 1 then
          begin
            writeln('    ldr x19, [sp], #16');
          end
        end
      end
      else
      begin
        { readln() with no args }
        Expect(TOK_RPAREN);
        EmitBL(rt_skip_line)
      end
    end
    else
    begin
      { readln with no parens }
      EmitBL(rt_skip_line)
    end
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
    { break = 98,114,101,97,107 }
    if (tok_len = 5) and (tok_str[0] = 98) and (tok_str[1] = 114) and
       (tok_str[2] = 101) and (tok_str[3] = 97) and (tok_str[4] = 107) then
    begin
      NextToken;
      if break_label = 0 then
        Error(15)  { break not inside a loop }
      else
        EmitBranchLabel(break_label)
    end
    { continue = 99,111,110,116,105,110,117,101 }
    else if (tok_len = 8) and (tok_str[0] = 99) and (tok_str[1] = 111) and
            (tok_str[2] = 110) and (tok_str[3] = 116) and (tok_str[4] = 105) and
            (tok_str[5] = 110) and (tok_str[6] = 117) and (tok_str[7] = 101) then
    begin
      NextToken;
      if continue_label = 0 then
        Error(15)  { continue not inside a loop }
      else
        EmitBranchLabel(continue_label)
    end
    { exit = 101,120,105,116 }
    else if (tok_len = 4) and (tok_str[0] = 101) and (tok_str[1] = 120) and
            (tok_str[2] = 105) and (tok_str[3] = 116) then
    begin
      NextToken;
      if exit_label = 0 then
      begin
        { In main program - just call halt with exit code 0 }
        EmitMovX0(0);
        { mov x16, #1 }
        writeln('    mov x16, #1');
        { svc #0x80 }
        writeln('    svc #0x80');
      end
      else
        EmitBranchLabel(exit_label)
    end
    { write = 119,114,105,116,101 }
    { writeln = 119,114,105,116,101,108,110 }
    { readchar = 114,101,97,100,99,104,97,114 }
    { writechar = 119,114,105,116,101,99,104,97 - actually too long, use 8 }
    { halt = 104,97,108,116 }
    else if TokIs8(119, 114, 105, 116, 101, 108, 110, 0) = 1 then
    begin
      { writeln }
      NextToken;
      lbl1 := 0;  { flag: 1 if writing to file }
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        if tok_type <> TOK_RPAREN then
        begin
          { Check if first arg is a file variable }
          if tok_type = TOK_IDENT then
          begin
            idx := SymLookup;
            if (idx >= 0) and ((sym_type[idx] = TYPE_FILE) or (sym_type[idx] = TYPE_TEXT)) then
            begin
              { File variable - save x20 and load file's fd }
              lbl1 := 1;
              NextToken;
              { Push x20 to save it }
              writeln('    str x20, [sp, #-16]!');
              { Load file's fd into x20 }
              EmitVarAddr(idx, scope_level);
              writeln('    ldr x20, [x0]');
              if tok_type = TOK_COMMA then
                NextToken
            end
          end;
          if (lbl1 = 0) or (tok_type <> TOK_RPAREN) then
          begin
            while tok_type <> TOK_RPAREN do
            begin
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
                  else if expr_type = TYPE_STRING then
                    EmitBL(rt_print_string)
                  else
                    EmitBL(rt_print_int)
                end
              end
              else
              begin
                ParseExpression;
                if expr_type = TYPE_REAL then
                  EmitBL(rt_print_real)
                else if expr_type = TYPE_STRING then
                  EmitBL(rt_print_string)
                else
                  EmitBL(rt_print_int)
              end;
              if tok_type = TOK_COMMA then NextToken
            end
          end
        end;
        Expect(TOK_RPAREN)
      end;
      EmitBL(rt_newline);
      { Restore x20 if we saved it }
      if lbl1 = 1 then
      begin
        writeln('    ldr x20, [sp], #16');
      end
    end
    else if TokIs8(119, 114, 105, 116, 101, 0, 0, 0) = 1 then
    begin
      { write }
      NextToken;
      lbl1 := 0;  { flag: 1 if writing to file }
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        if tok_type <> TOK_RPAREN then
        begin
          { Check if first arg is a file variable }
          if tok_type = TOK_IDENT then
          begin
            idx := SymLookup;
            if (idx >= 0) and ((sym_type[idx] = TYPE_FILE) or (sym_type[idx] = TYPE_TEXT)) then
            begin
              { File variable - save x20 and load file's fd }
              lbl1 := 1;
              NextToken;
              { Push x20 to save it }
              writeln('    str x20, [sp, #-16]!');
              { Load file's fd into x20 }
              EmitVarAddr(idx, scope_level);
              writeln('    ldr x20, [x0]');
              if tok_type = TOK_COMMA then
                NextToken
            end
          end;
          if (lbl1 = 0) or (tok_type <> TOK_RPAREN) then
          begin
            while tok_type <> TOK_RPAREN do
            begin
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
                  else if expr_type = TYPE_STRING then
                    EmitBL(rt_print_string)
                  else
                    EmitBL(rt_print_int)
                end
              end
              else
              begin
                ParseExpression;
                if expr_type = TYPE_REAL then
                  EmitBL(rt_print_real)
                else if expr_type = TYPE_STRING then
                  EmitBL(rt_print_string)
                else
                  EmitBL(rt_print_int)
              end;
              if tok_type = TOK_COMMA then NextToken
            end
          end
        end;
        Expect(TOK_RPAREN)
      end;
      { Restore x20 if we saved it }
      if lbl1 = 1 then
      begin
        writeln('    ldr x20, [sp], #16');
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
    { randomize = 114,97,110,100,111,109,105,122,101 }
    else if (tok_len = 9) and (tok_str[0] = 114) and (tok_str[1] = 97) and
            (tok_str[2] = 110) and (tok_str[3] = 100) and (tok_str[4] = 111) and
            (tok_str[5] = 109) and (tok_str[6] = 105) and (tok_str[7] = 122) and
            (tok_str[8] = 101) then
    begin
      { randomize - seed the PRNG using stack pointer XOR with a constant }
      NextToken;
      if tok_type = TOK_LPAREN then
      begin
        NextToken;
        if tok_type <> TOK_RPAREN then
        begin
          { randomize(seed) - use provided seed }
          ParseExpression;
          { mov x27, x0 }
          writeln('    mov x27, x0');
        end
        else
        begin
          { randomize() - use sp as seed }
          writeln('    mov x27, sp');
        end;
        Expect(TOK_RPAREN)
      end
      else
      begin
        { randomize with no parens - use sp as seed }
        writeln('    mov x27, sp');
      end
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
      writeln('    mov x8, x0');
      { Load current value }
      writeln('    ldr x0, [x8]');
      { Add 1 }
      writeln('    add x0, x0, #1');
      { Store back }
      writeln('    str x0, [x8]');
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
      writeln('    mov x8, x0');
      { Load current value }
      writeln('    ldr x0, [x8]');
      { Subtract 1 }
      writeln('    sub x0, x0, #1');
      { Store back }
      writeln('    str x0, [x8]');
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
    { writefilechar(file, char) - 119,114,105,116,101,102,105,108,101,99,104,97,114 }
    else if (tok_len = 13) and (tok_str[0] = 119) and (tok_str[1] = 114) and
            (tok_str[2] = 105) and (tok_str[3] = 116) and (tok_str[4] = 101) and
            (tok_str[5] = 102) and (tok_str[6] = 105) and (tok_str[7] = 108) and
            (tok_str[8] = 101) and (tok_str[9] = 99) and (tok_str[10] = 104) and
            (tok_str[11] = 97) and (tok_str[12] = 114) then
    begin
      NextToken;
      Expect(TOK_LPAREN);
      { First arg: file variable }
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      { Load file fd into x0 }
      EmitVarAddr(idx, scope_level);
      writeln('    ldr x0, [x0]');
      { Push fd to stack }
      EmitPushX0;
      NextToken;
      Expect(TOK_COMMA);
      { Second arg: character expression }
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 has char, pop fd into x1, swap }
      writeln('    mov x1, x0');
      writeln('    ldr x0, [sp], #16');
      { Call write char to fd runtime: x0=fd, x1=char }
      EmitBL(rt_write_char_fd)
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
      else if sym_const_val[idx] = TYPE_ARRAY then
      begin
        { Pointer to array: calculate size from bounds }
        arg_count := sym_label[idx];  { ptr_arr index }
        lbl1 := (ptr_arr_hi[arg_count] - ptr_arr_lo[arg_count] + 1) * 8
      end
      else
        lbl1 := 8;  { basic types are 8 bytes }
      { Align to 8 bytes }
      lbl1 := ((lbl1 + 7) div 8) * 8;
      { Allocate via rt_alloc: put size in x0, call rt_alloc }
      EmitMovX0(lbl1);
      EmitBL(rt_alloc);
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
      { dispose(p) - free memory via free list allocator }
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
      { Load pointer value into x0 }
      if sym_level[idx] < scope_level then
        EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      else
        EmitLdurX0(sym_offset[idx]);
      { Call rt_free to return memory to free list }
      EmitBL(rt_free);
      { Set pointer to nil for safety }
      EmitMovX0(0);
      if sym_level[idx] < scope_level then
        EmitSturX0Outer(sym_offset[idx], sym_level[idx], scope_level)
      else
        EmitSturX0(sym_offset[idx])
    end
    { assign = 97,115,115,105,103,110 }
    else if (tok_len = 6) and (tok_str[0] = 97) and (tok_str[1] = 115) and
            (tok_str[2] = 115) and (tok_str[3] = 105) and (tok_str[4] = 103) and
            (tok_str[5] = 110) then
    begin
      { assign(f, filename) - associate file variable with filename }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_COMMA);
      { Get address of file variable }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;  { save file var address }
      { Parse filename (string expression) }
      ParseExpression;
      { x0 = source string address, stack = file var address }
      { Copy filename to file var offset 16 }
      EmitPopX1;  { x1 = file var address }
      { add x1, x1, #16 - point to filename area }
      writeln('    add x1, x1, #16');
      { swap x0 and x1 for str_copy (dest in x0, src in x1) }
      { x0 = src string, x1 = dest (file var + 16) }
      writeln('    mov x2, x0');
      writeln('    mov x0, x1');
      writeln('    mov x1, x2');
      { Now x0 = dest, x1 = src. Save dest before call. }
      EmitPushX0;
      EmitBL(rt_str_copy);
      EmitPopX0;  { restore dest address }
      { Add null terminator for C string compatibility }
      { ldrb w1, [x0] - load length }
      writeln('    ldrb w1, [x0]');
      { add x1, x1, #1 - position after last char }
      writeln('    add x1, x1, #1');
      { strb wzr, [x0, x1] - store null byte }
      writeln('    strb wzr, [x0, x1]');
      { Initialize fd to -1 (not open) }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      EmitMovX0(-1);
      EmitPopX1;
      { str x0, [x1] }
      writeln('    str x0, [x1]');
      Expect(TOK_RPAREN)
    end
    { assigntokstr = 97,115,115,105,103,110,116,111,107,115,116,114 }
    else if (tok_len = 12) and (tok_str[0] = 97) and (tok_str[1] = 115) and
            (tok_str[2] = 115) and (tok_str[3] = 105) and (tok_str[4] = 103) and
            (tok_str[5] = 110) and (tok_str[6] = 116) and (tok_str[7] = 111) and
            (tok_str[8] = 107) and (tok_str[9] = 115) and (tok_str[10] = 116) and
            (tok_str[11] = 114) then
    begin
      { assigntokstr(f, start, len) - assign filename from tok_str to file }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_COMMA);
      { Parse start index }
      ParseExpression;
      EmitPushX0;  { save start }
      Expect(TOK_COMMA);
      { Parse length }
      ParseExpression;
      { x0 = len, stack has start }
      { mov x2, x0 - x2 = len }
      writeln('    mov x2, x0');
      EmitPopX1;  { x1 = start }
      { Get address of tok_str using VarAddr }
      { We need the tok_str global - find it in symbol table }
      { For now, use direct offset calculation since tok_str is a global array }
      { Get file var base address first }
      EmitVarAddr(idx, scope_level);
      { add x0, x0, #16 - point to filename area }
      writeln('    add x0, x0, #16');
      { Save dest address }
      EmitPushX0;
      { Get tok_str base address - look it up }
      { str x1, [sp, #-16]! - save start index }
      writeln('    str x1, [sp, #-16]!');
      { str x2, [sp, #-16]! - save len }
      writeln('    str x2, [sp, #-16]!');
      { Find tok_str in symbol table and get its address }
      tok_str[0] := 116; tok_str[1] := 111; tok_str[2] := 107; tok_str[3] := 95;
      tok_str[4] := 115; tok_str[5] := 116; tok_str[6] := 114; tok_str[7] := 0;
      tok_len := 7;
      arg_idx := SymLookup;
      if arg_idx < 0 then
        Error(3);  { tok_str not found }
      EmitVarAddr(arg_idx, scope_level);  { x0 = tok_str base }
      { mov x3, x0 - x3 = tok_str base }
      writeln('    mov x3, x0');
      { ldr x2, [sp], #16 - restore len }
      writeln('    ldr x2, [sp], #16');
      { ldr x1, [sp], #16 - restore start }
      writeln('    ldr x1, [sp], #16');
      { ldr x0, [sp], #16 - restore dest }
      EmitPopX0;
      { x3 + x1*8 = source address (tok_str elements are 8 bytes) }
      { lsl x1, x1, #3 }
      writeln('    lsl x1, x1, #3');
      { add x3, x3, x1 - x3 = source address }
      writeln('    add x3, x3, x1');
      { Copy loop: copy x2 characters from [x3] to [x0], skip length byte }
      { add x0, x0, #1 - skip length byte position }
      writeln('    add x0, x0, #1');
      { Save x2 (len) for later length byte write }
      writeln('    mov x4, x2');
      lbl1 := NewLabel;
      lbl2 := NewLabel;
      EmitLabel(lbl1);
      { cbz x2, done }
      write('    cbz x2, L'); writeln(lbl2);
      { ldr x5, [x3], #8 - load 8-byte integer from tok_str }
      writeln('    ldr x5, [x3], #8');
      { strb w5, [x0], #1 - store as byte }
      writeln('    strb w5, [x0], #1');
      { sub x2, x2, #1 }
      writeln('    sub x2, x2, #1');
      EmitBranchLabel(lbl1);
      EmitLabel(lbl2);
      { strb wzr, [x0] - null terminate }
      writeln('    strb wzr, [x0]');
      { Write length byte at start of filename: x0-x4-1 = start address }
      { sub x0, x0, x4 }
      writeln('    sub x0, x0, x4');
      { sub x0, x0, #1 }
      writeln('    sub x0, x0, #1');
      { strb w4, [x0] - store length byte }
      writeln('    strb w4, [x0]');
      { Initialize fd to -1 }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      EmitMovX0(-1);
      EmitPopX1;
      { str x0, [x1] }
      writeln('    str x0, [x1]');
      Expect(TOK_RPAREN)
    end
    { reset = 114,101,115,101,116 }
    else if (tok_len = 5) and (tok_str[0] = 114) and (tok_str[1] = 101) and
            (tok_str[2] = 115) and (tok_str[3] = 101) and (tok_str[4] = 116) then
    begin
      { reset(f) - open file for reading }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get filename address (offset 16) and convert to C string }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;  { save file var base }
      { add x0, x0, #16 - point to filename }
      writeln('    add x0, x0, #16');
      { Convert Pascal string to C string: skip length byte }
      writeln('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_RDONLY(0), x2=mode(0) }
      writeln('    mov x1, #0');
      writeln('    mov x2, #0');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      writeln('    movz x16, #5');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Store fd in file variable (offset 0) }
      EmitPopX1;  { x1 = file var base }
      { str x0, [x1] }
      writeln('    str x0, [x1]');
      { Store mode=1 (read) at offset 8 }
      writeln('    mov x0, #1');
      { str x0, [x1, #8] }
      writeln('    str x0, [x1, #8]');
    end
    { rewrite = 114,101,119,114,105,116,101 }
    else if (tok_len = 7) and (tok_str[0] = 114) and (tok_str[1] = 101) and
            (tok_str[2] = 119) and (tok_str[3] = 114) and (tok_str[4] = 105) and
            (tok_str[5] = 116) and (tok_str[6] = 101) then
    begin
      { rewrite(f) - open/create file for writing }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Get filename address (offset 16) and convert to C string }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;  { save file var base }
      { add x0, x0, #16 - point to filename }
      writeln('    add x0, x0, #16');
      { Convert Pascal string to C string: skip length byte }
      writeln('    add x0, x0, #1');
      { open syscall: x0=path, x1=O_WRONLY|O_CREAT|O_TRUNC(1537), x2=mode(420) }
      { mov x1, #1537 }
      writeln('    mov x1, #1537');
      { mov x2, #420 }
      writeln('    mov x2, #420');
      { movz x16, #5; movk x16, #0x200, lsl #16 = 0x2000005 }
      writeln('    movz x16, #5');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Store fd in file variable (offset 0) }
      EmitPopX1;  { x1 = file var base }
      { str x0, [x1] }
      writeln('    str x0, [x1]');
      { Store mode=2 (write) at offset 8 }
      writeln('    mov x0, #2');
      { str x0, [x1, #8] }
      writeln('    str x0, [x1, #8]');
    end
    { close = 99,108,111,115,101 }
    else if (tok_len = 5) and (tok_str[0] = 99) and (tok_str[1] = 108) and
            (tok_str[2] = 111) and (tok_str[3] = 115) and (tok_str[4] = 101) then
    begin
      { close(f) - close file }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Load fd from file variable }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      writeln('    ldr x0, [x0]');
      { close syscall: x0=fd }
      { movz x16, #6; movk x16, #0x200, lsl #16 = 0x2000006 }
      writeln('    movz x16, #6');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { Set fd to -1 to mark as closed }
      EmitVarAddr(idx, scope_level);
      EmitPushX0;
      EmitMovX0(-1);
      EmitPopX1;
      { str x0, [x1] }
      writeln('    str x0, [x1]');
      { Set mode to 0 (closed) at offset 8 }
      writeln('    mov x0, #0');
      { str x0, [x1, #8] }
      writeln('    str x0, [x1, #8]');
    end
    { setinput = 115,101,116,105,110,112,117,116 }
    else if (tok_len = 8) and (tok_str[0] = 115) and (tok_str[1] = 101) and
            (tok_str[2] = 116) and (tok_str[3] = 105) and (tok_str[4] = 110) and
            (tok_str[5] = 112) and (tok_str[6] = 117) and (tok_str[7] = 116) then
    begin
      { setinput(f) - set input file descriptor from text file variable }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if sym_type[idx] <> TYPE_TEXT then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Load fd from file variable and store in x19 }
      EmitVarAddr(idx, scope_level);
      { ldr x19, [x0] }
      writeln('    ldr x19, [x0]');
    end
    { setoutput = 115,101,116,111,117,116,112,117,116 }
    else if (tok_len = 9) and (tok_str[0] = 115) and (tok_str[1] = 101) and
            (tok_str[2] = 116) and (tok_str[3] = 111) and (tok_str[4] = 117) and
            (tok_str[5] = 116) and (tok_str[6] = 112) and (tok_str[7] = 117) and
            (tok_str[8] = 116) then
    begin
      { setoutput(f) - set output file descriptor from text file variable }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if sym_type[idx] <> TYPE_TEXT then
        Error(9);
      NextToken;
      Expect(TOK_RPAREN);
      { Load fd from file variable and store in x20 }
      EmitVarAddr(idx, scope_level);
      { ldr x20, [x0] }
      writeln('    ldr x20, [x0]');
    end
    { setinputfd = 115,101,116,105,110,112,117,116,102,100 (10 chars) }
    else if (tok_len = 10) and (tok_str[0] = 115) and (tok_str[1] = 101) and
            (tok_str[2] = 116) and (tok_str[3] = 105) and (tok_str[4] = 110) and
            (tok_str[5] = 112) and (tok_str[6] = 117) and (tok_str[7] = 116) and
            (tok_str[8] = 102) and (tok_str[9] = 100) then
    begin
      { setinputfd(fd) - set input file descriptor directly from integer }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { mov x19, x0 }
      writeln('    mov x19, x0');
    end
    { setoutputfd = 115,101,116,111,117,116,112,117,116,102,100 (11 chars) }
    else if (tok_len = 11) and (tok_str[0] = 115) and (tok_str[1] = 101) and
            (tok_str[2] = 116) and (tok_str[3] = 111) and (tok_str[4] = 117) and
            (tok_str[5] = 116) and (tok_str[6] = 112) and (tok_str[7] = 117) and
            (tok_str[8] = 116) and (tok_str[9] = 102) and (tok_str[10] = 100) then
    begin
      { setoutputfd(fd) - set output file descriptor directly from integer }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { mov x20, x0 }
      writeln('    mov x20, x0');
    end
    { closefd = 99,108,111,115,101,102,100 (7 chars) }
    else if (tok_len = 7) and (tok_str[0] = 99) and (tok_str[1] = 108) and
            (tok_str[2] = 111) and (tok_str[3] = 115) and (tok_str[4] = 101) and
            (tok_str[5] = 102) and (tok_str[6] = 100) then
    begin
      { closefd(fd) - close a file descriptor }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;
      Expect(TOK_RPAREN);
      { x0 = fd to close }
      { close syscall: x16 = 0x2000006 }
      writeln('    movz x16, #6');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc
    end
    { writefd = 119,114,105,116,101,102,100 (7 chars) }
    else if (tok_len = 7) and (tok_str[0] = 119) and (tok_str[1] = 114) and
            (tok_str[2] = 105) and (tok_str[3] = 116) and (tok_str[4] = 101) and
            (tok_str[5] = 102) and (tok_str[6] = 100) then
    begin
      { writefd(fd, char) - write one char to fd }
      NextToken;
      Expect(TOK_LPAREN);
      ParseExpression;  { fd }
      EmitPushX0;
      Expect(TOK_COMMA);
      ParseExpression;  { char }
      { x0 = char, stack top = fd }
      { sub sp, sp, #16; strb w0, [sp] - store char in buffer }
      writeln('    sub sp, sp, #16');
      { strb w0, [sp] }
      writeln('    strb w0, [sp]');
      { ldr x0, [sp, #16] - load fd from saved position }
      writeln('    ldr x0, [sp, #16]');
      { mov x1, sp - buffer address }
      writeln('    mov x1, sp');
      { mov x2, #1 - write 1 byte }
      writeln('    mov x2, #1');
      { write syscall: x16 = 0x2000004 }
      writeln('    movz x16, #4');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      { add sp, sp, #32 - restore stack (16 for buffer + 16 for saved fd) }
      writeln('    add sp, sp, #32');
      Expect(TOK_RPAREN)
    end
    { seek = 115,101,101,107 }
    else if (tok_len = 4) and (tok_str[0] = 115) and (tok_str[1] = 101) and
            (tok_str[2] = 101) and (tok_str[3] = 107) then
    begin
      { seek(f, pos) - move to position in file }
      NextToken;
      Expect(TOK_LPAREN);
      if tok_type <> TOK_IDENT then
        Error(6);
      idx := SymLookup;
      if idx < 0 then
        Error(3);
      if (sym_type[idx] <> TYPE_FILE) and (sym_type[idx] <> TYPE_TEXT) then
        Error(9);
      NextToken;
      Expect(TOK_COMMA);
      { Get file fd and save to x23 }
      EmitVarAddr(idx, scope_level);
      { ldr x0, [x0] - load fd }
      writeln('    ldr x0, [x0]');
      { Save fd to x23 }
      writeln('    mov x23, x0');
      { Parse position expression }
      ParseExpression;
      { x0 = position, x23 = fd }
      { mov x1, x0 (position to x1) }
      writeln('    mov x1, x0');
      { mov x0, x23 (fd to x0) }
      writeln('    mov x0, x23');
      { mov x2, #0 (SEEK_SET) }
      writeln('    mov x2, #0');
      { lseek syscall: 0x20000C7 }
      writeln('    movz x16, #0xC7');
      writeln('    movk x16, #0x200, lsl #16');
      EmitSvc;
      Expect(TOK_RPAREN)
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
      writeln('    mov x2, x0');
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
        writeln('    mov x8, x21');
        EmitMovX0(tok_len);
        writeln('    strb w0, [x8]');
        for i := 0 to tok_len - 1 do
        begin
          EmitMovX0(tok_str[i]);
          write('    strb w0, [x8, #'); write(i + 1); writeln(']');
        end;
        writeln('    mov x0, x21');
        writeln('    add x21, x21, #256');
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
      writeln('    mov x2, x0');
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
      writeln('    mov x1, x0');
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
        writeln('    mov x8, x21');
        EmitMovX0(tok_len);
        writeln('    strb w0, [x8]');
        for i := 0 to tok_len - 1 do
        begin
          EmitMovX0(tok_str[i]);
          write('    strb w0, [x8, #'); write(i + 1); writeln(']');
        end;
        writeln('    mov x0, x21');
        writeln('    add x21, x21, #256');
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
      writeln('    mov x1, x0');
      EmitPopX0;  { value }
      { str x0, [x1] }
      writeln('    str x0, [x1]');
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
      writeln('    mov x1, x0');
      EmitPopX0;  { error }
      { str x0, [x1] }
      writeln('    str x0, [x1]');
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
    { initkeyboard = 105,110,105,116,107,101,121,98,111,97,114,100 (12 chars) }
    else if (tok_len = 12) and (ToLower(tok_str[0]) = 105) and (ToLower(tok_str[1]) = 110) and
            (ToLower(tok_str[2]) = 105) and (ToLower(tok_str[3]) = 116) and (ToLower(tok_str[4]) = 107) and
            (ToLower(tok_str[5]) = 101) and (ToLower(tok_str[6]) = 121) and (ToLower(tok_str[7]) = 98) and
            (ToLower(tok_str[8]) = 111) and (ToLower(tok_str[9]) = 97) and (ToLower(tok_str[10]) = 114) and
            (ToLower(tok_str[11]) = 100) then
    begin
      NextToken;
      EmitBL(rt_initkeyboard)
    end
    { donekeyboard = 100,111,110,101,107,101,121,98,111,97,114,100 (12 chars) }
    else if (tok_len = 12) and (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 111) and
            (ToLower(tok_str[2]) = 110) and (ToLower(tok_str[3]) = 101) and (ToLower(tok_str[4]) = 107) and
            (ToLower(tok_str[5]) = 101) and (ToLower(tok_str[6]) = 121) and (ToLower(tok_str[7]) = 98) and
            (ToLower(tok_str[8]) = 111) and (ToLower(tok_str[9]) = 97) and (ToLower(tok_str[10]) = 114) and
            (ToLower(tok_str[11]) = 100) then
    begin
      NextToken;
      EmitBL(rt_donekeyboard)
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
            writeln('    add x1, x8, #');
          end
          else
          begin
            writeln('    add x1, x29, #');
          end;
          { Store value to field }
          if field_type[arg_count] = TYPE_REAL then
          begin
            EmitPopD0;
            { str d0, [x1] }
            writeln('    str d0, [x1]');
          end
          else
          begin
            EmitPopX0;
            { str x0, [x1] }
            writeln('    str x0, [x1]');
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
                  writeln('    sub x0, x1, x0');
                  { Multiply by 8 using lsl #3 }
                  writeln('    lsl x0, x0, #3');
                  { Get base address and subtract element offset }
                  if sym_level[var_arg_idx] < scope_level then
                  begin
                    EmitFollowChain(sym_level[var_arg_idx], scope_level);
                    EmitSubLargeOffset(1, 8, 0 - sym_offset[var_arg_idx])
                  end
                  else
                    EmitSubLargeOffset(1, 29, 0 - sym_offset[var_arg_idx]);
                  { Address = base - element_offset }
                  writeln('    sub x0, x1, x0');
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
          write('    ldr x'); write(i); writeln(', [sp], #16');
        end;
        { Set up static link for callee }
        EmitStaticLink(sym_level[idx], scope_level);
        { Check if calling imported unit procedure }
        if sym_unit_idx[idx] >= 0 then
          EmitBLUnitProc(sym_unit_idx[idx], idx)
        else
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
          writeln('    sub x0, x1, x0');
          { Multiply by element size }
          if sym_var_param_flags[idx] > 0 then
          begin
            { Array of records }
            lbl1 := sym_label[sym_var_param_flags[idx] - 1];
            EmitPushX0;
            EmitMovX0(lbl1);
            EmitPopX1;
            writeln('    mul x0, x1, x0');
          end
          else
          begin
            writeln('    lsl x0, x0, #3');
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
          writeln('    sub x1, x1, x0');
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
              writeln('    add x1, x1, #');
            end;
            EmitPushX1;  { save field address }
            Expect(TOK_ASSIGN);
            ParseExpression;
            if field_type[arg_count] = TYPE_REAL then
            begin
              if expr_type <> TYPE_REAL then
                EmitScvtfD0X0;
              EmitPopX1;
              writeln('    str d0, [x1]');
            end
            else
            begin
              EmitPopX1;
              writeln('    str x0, [x1]');
            end
          end
          else
          begin
            { Basic array assignment }
            EmitPushX1;  { save element address }
            Expect(TOK_ASSIGN);
            ParseExpression;
            EmitPopX1;
            writeln('    str x0, [x1]');
          end
        end
        else if (sym_type[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
        begin
          { Record field assignment: rec.field := value or rec.field.subfield := value }
          NextToken;  { consume '.' }
          if tok_type <> TOK_IDENT then
            Error(11);
          { Find the field }
          arg_count := FindField(sym_const_val[idx]);  { reuse arg_count for field_idx }
          if arg_count < 0 then
            Error(15);  { undefined field }
          { Accumulate total offset for nested fields }
          lbl1 := field_offset[arg_count];  { total offset from base }
          NextToken;
          { Handle nested record fields }
          while (field_type[arg_count] = TYPE_RECORD) and (tok_type = TOK_DOT) do
          begin
            NextToken;  { consume '.' }
            if tok_type <> TOK_IDENT then
              Error(11);
            lbl2 := arg_count;  { save current field index }
            arg_count := FindField(field_rec_type[lbl2]);
            if arg_count < 0 then
              Error(15);
            lbl1 := lbl1 + field_offset[arg_count];  { accumulate offset }
            NextToken
          end;
          Expect(TOK_ASSIGN);
          ParseExpression;  { value in x0 or d0 }

          { Compute field address: base + total_offset }
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
              writeln('    add x0, x8, #');
            end
            else
            begin
              writeln('    add x0, x29, #');
            end;
            EmitPopD0;
            { str d0, [x0] }
            writeln('    str d0, [x0]');
          end
          else
          begin
            { Value is in x0 }
            EmitPushX0;  { save value }
            { Compute address }
            if sym_level[idx] < scope_level then
            begin
              EmitFollowChain(sym_level[idx], scope_level);
              writeln('    add x1, x8, #');
            end
            else
            begin
              writeln('    add x1, x29, #');
            end;
            EmitPopX0;
            { str x0, [x1] }
            writeln('    str x0, [x1]');
          end
        end
        else if (sym_type[idx] = TYPE_POINTER) and (tok_type = TOK_CARET) then
        begin
          { Pointer dereference assignment: p^ := value or pp^^ := value or p^.field := value }
          { Count consecutive ^ tokens }
          lbl1 := 0;  { deref count }
          while tok_type = TOK_CARET do
          begin
            lbl1 := lbl1 + 1;
            NextToken
          end;
          { Load pointer value }
          if sym_level[idx] < scope_level then
            EmitLdurX0Outer(sym_offset[idx], sym_level[idx], scope_level)
          else
            EmitLdurX0(sym_offset[idx]);
          { Dereference (count-1) times to get target address }
          for i := 1 to lbl1 - 1 do
          begin
            writeln('    ldr x0, [x0]');
          end;
          { lbl2 = remaining pointer depth after all derefs }
          lbl2 := ptr_depth[idx] - lbl1;

          if (lbl2 = 0) and (ptr_ultimate_type[idx] = TYPE_RECORD) and (tok_type = TOK_DOT) then
          begin
            { Pointer to record field assignment: p^.field := value or p^.field.subfield := value }
            NextToken;  { consume '.' }
            { Find field in the record type }
            arg_count := FindField(ptr_ultimate_rec[idx]);
            if arg_count < 0 then
              Error(11);  { unknown field }
            NextToken;  { consume field name }
            { Add field offset to pointer: x0 = x0 + offset }
            if field_offset[arg_count] > 0 then
            begin
              writeln('    add x0, x0, #');
            end;
            { Handle nested record fields }
            while (field_type[arg_count] = TYPE_RECORD) and (tok_type = TOK_DOT) do
            begin
              NextToken;  { consume '.' }
              if tok_type <> TOK_IDENT then
                Error(11);
              var_arg_idx := arg_count;  { save current field index }
              arg_count := FindField(field_rec_type[var_arg_idx]);
              if arg_count < 0 then
                Error(15);
              if field_offset[arg_count] > 0 then
              begin
                writeln('    add x0, x0, #');
              end;
              NextToken
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
              writeln('    str d0, [x1]');
            end
            else
            begin
              EmitPopX1;  { get address into x1 }
              { str x0, [x1] }
              writeln('    str x0, [x1]');
            end
          end
          else if (lbl2 = 0) and (ptr_ultimate_type[idx] = TYPE_ARRAY) and (tok_type = TOK_LBRACKET) then
          begin
            { Pointer to array element assignment: pa^[i] := value }
            NextToken;  { consume '[' }
            EmitPushX0;  { save array base address }
            ParseExpression;  { index in x0 }
            Expect(TOK_RBRACKET);
            { x0 = index, stack has base address }
            { Subtract low bound }
            arg_count := sym_label[idx];  { ptr_arr index }
            EmitPushX0;
            EmitMovX0(ptr_arr_lo[arg_count]);
            EmitPopX1;
            { x0 = x1 - x0 = index - low_bound }
            writeln('    sub x0, x1, x0');
            { Multiply by 8 using lsl #3 }
            writeln('    lsl x0, x0, #3');
            { Add to base: x0 = base + offset }
            EmitPopX1;  { x1 = base address }
            writeln('    add x8, x1, x0');
            { x8 now has element address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value to store }
            { Store value }
            if ptr_arr_elem[arg_count] = TYPE_REAL then
            begin
              if expr_type <> TYPE_REAL then
                EmitScvtfD0X0;
              writeln('    str d0, [x8]');
            end
            else
            begin
              writeln('    str x0, [x8]');
            end
          end
          else
          begin
            { Simple pointer dereference assignment: p^ := value or pp^^ := value }
            EmitPushX0;  { save address }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value to store }
            { Determine target type }
            if (lbl2 = 0) and (ptr_ultimate_type[idx] = TYPE_REAL) then
            begin
              { Value in d0, need to store through pointer }
              if expr_type <> TYPE_REAL then
                EmitScvtfD0X0;
              EmitPopX1;  { get address into x1 }
              { str d0, [x1] }
              writeln('    str d0, [x1]');
            end
            else
            begin
              { Value in x0 }
              EmitPopX1;  { get address into x1 }
              { str x0, [x1] }
              writeln('    str x0, [x1]');
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
            writeln('    add x8, x1, x0');
            { x8 now has the address to store to }
            Expect(TOK_ASSIGN);
            ParseExpression;  { value to store in x0 }
            { strb w0, [x8] }
            writeln('    strb w0, [x8]');
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
                writeln('    sub x8, x8, #');
              end
              else
              begin
                writeln('    add x8, x8, #');
              end
            end
            else
            begin
              { add/sub x8, x29, #offset }
              if sym_offset[idx] < 0 then
              begin
                writeln('    sub x8, x29, #');
              end
              else
              begin
                writeln('    add x8, x29, #');
              end
            end;
            { Store length at [x8] }
            EmitMovX0(tok_len);
            writeln('    strb w0, [x8]');
            { Store each character at [x8+1], [x8+2], etc }
            for i := 0 to tok_len - 1 do
            begin
              EmitMovX0(tok_str[i]);
              write('    strb w0, [x8, #'); write(i + 1); writeln(']');
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
            writeln('    mov x1, x0');
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
            writeln('    str d0, [x0]');
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
            writeln('    str x1, [x0]');
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
          writeln('    mov x8, x0');
          writeln('    mov x0, x21');
          { Save heap addr as result }
          EmitSturX0(-16);
          { x8 = dest, x9 = source for str_copy }
          writeln('    mov x9, x8');
          writeln('    mov x8, x21');
          EmitBL(rt_str_copy);
          { Advance heap pointer }
          writeln('    add x21, x21, #256');
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


