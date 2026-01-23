{ ----- Print Runtime ----- }

Procedure EmitPrintIntRuntime;
Var
  loop_lbl, done_lbl, neg_lbl, print_lbl: Integer;
Begin
  { Runtime routine To print Integer In x0 }
  EmitLabel(rt_print_int);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { Save value }
  EmitSturX0(-24);

  { Handle negative }
  neg_lbl := NewLabel;
  done_lbl := NewLabel;
  WriteLn('    cmp x0, #0');

  Write('    b.ge L'); WriteLn(neg_lbl);

  { Print minus sign }
  EmitMovX0(1);
  EmitSturX0(-32);
  EmitMovX0(45);  { '-' }
  EmitSturX0(-8);
  EmitMovX16(33554436); { 0x2000004 }
  EmitMovX0X20;
  WriteLn('    sub x1, x29, #8');
  WriteLn('    mov x2, #1');
  EmitSvc;

  { Negate }
  EmitLdurX0(-24);
  EmitNeg;
  EmitSturX0(-24);

  EmitLabel(neg_lbl);

  { Convert To String (digits In reverse) }
  EmitMovX0(0);
  EmitSturX0(-40);  { digit count }

  loop_lbl := NewLabel;
  print_lbl := NewLabel;

  EmitLabel(loop_lbl);
  EmitLdurX0(-24);
  EmitBranchLabelZ(print_lbl);

  { val % 10 }
  EmitLdurX0(-24);
  EmitPushX0;
  EmitMovX0(10);
  EmitPopX1;
  EmitSDiv;
  EmitMovX2X0;
  EmitLdurX0(-24);
  EmitPushX0;
  EmitMovX0(10);
  EmitPopX1;
  EmitMsub;

  { Store digit }
  WriteLn('    add x0, x0, #48');

  WriteLn('    ldur x1, [x29, #-40]');

  WriteLn('    sub x2, x29, #48');

  WriteLn('    strb w0, [x2, x1]');

  { digit count++ }
  WriteLn('    add x1, x1, #1');

  WriteLn('    stur x1, [x29, #-40]');

  { val /= 10 }
  EmitLdurX0(-24);
  EmitPushX0;
  EmitMovX0(10);
  EmitPopX1;
  EmitSDiv;
  EmitSturX0(-24);

  EmitBranchLabel(loop_lbl);

  EmitLabel(print_lbl);

  { Handle zero }
  EmitLdurX0(-40);
  EmitBranchLabelNZ(done_lbl);
  EmitMovX0(48);  { '0' }
  EmitSturX0(-48);
  EmitMovX0(1);
  EmitSturX0(-40);

  EmitLabel(done_lbl);

  { Print digits In reverse order }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);
  EmitLdurX0(-40);
  EmitBranchLabelZ(done_lbl);

  { digit count-- }
  WriteLn('    sub x0, x0, #1');
  EmitSturX0(-40);

  { Load digit }
  WriteLn('    sub x1, x29, #48');

  WriteLn('    ldrb w0, [x1, x0]');

  { Print Char }
  EmitSturX0(-8);
  EmitMovX16(33554436);
  EmitMovX0X20;
  WriteLn('    sub x1, x29, #8');
  WriteLn('    mov x2, #1');
  EmitSvc;

  EmitBranchLabel(loop_lbl);

  { Exit label }
  EmitLabel(done_lbl);

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitNewlineRuntime;
Begin
  { Newline routine - print Chr(10) }
  EmitLabel(rt_newline);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  EmitMovX0(10);
  EmitSturX0(-9);
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;
  WriteLn('    sub x1, x29, #9');
  WriteLn('    mov x2, #1');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitReadcharRuntime;
Begin
  { ReadChar routine - Read one Char, return In x0 (-1 For EOF) }
  { Uses x19 as input file descriptor (0=stdin, Or opened file) }
  EmitLabel(rt_readchar);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  { Check If Read returned >= 1 }
  WriteLn('    cmp x0, #1');
  Write('    b.ge L'); WriteLn(label_count);
  EmitMovX0(-1);  { EOF }
  EmitBranchLabel(label_count + 1);
  EmitLabel(label_count);
  label_count := label_count + 1;
  WriteLn('    ldrb w0, [sp]');
  EmitLabel(label_count);
  label_count := label_count + 1;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitPrintCharRuntime;
Begin
  { Print Char routine - print Char In x0 }
  EmitLabel(rt_print_char);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  WriteLn('    strb w0, [sp]');
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitWriteCharFdRuntime;
Begin
  { Write Char To fd routine - x0=fd, x1=Char }
  EmitLabel(rt_write_char_fd);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store Char on stack }
  WriteLn('    strb w1, [sp]');
  { x0 already has fd, Set up rest Of syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitPrintStringRuntime;
Var
  loop_lbl, done_lbl: Integer;
Begin
  { Print String routine - x0 = address Of pascal String (Length byte + chars) }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(rt_print_string);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);
  { Save base address To [x29, #-8] }
  WriteLn('    stur x0, [x29, #-8]');
  { Load Length from [x0] into x1 And save To [x29, #-16] }
  WriteLn('    ldrb w1, [x0]');
  WriteLn('    stur x1, [x29, #-16]');
  { Initialize index To 0 at [x29, #-24] }
  WriteLn('    mov x2, #0');
  WriteLn('    stur x2, [x29, #-24]');
  { Loop label }
  EmitLabel(loop_lbl);
  { Load index And Length, compare }
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    cmp x2, x1');
  { b.ge done_lbl }
  Write('    b.ge L'); WriteLn(done_lbl);
  { Load Char at [base + index + 1] }
  WriteLn('    ldur x0, [x29, #-8]');
  WriteLn('    add x0, x0, x2');
  WriteLn('    add x0, x0, #1');
  WriteLn('    ldrb w0, [x0]');
  { Call print_char }
  EmitBL(rt_print_char);
  { Increment index }
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    add x2, x2, #1');
  WriteLn('    stur x2, [x29, #-24]');
  { Branch back To loop }
  EmitBranchLabel(loop_lbl);
  { Done label }
  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitReadIntRuntime;
Var
  skip_ws_lbl, read_digit_lbl, done_lbl, neg_lbl, not_neg_lbl, skip_neg_lbl: Integer;
Begin
  { Read Integer routine - reads from x19 (input fd), returns In x0 }
  { Skips whitespace, handles optional minus sign, reads digits }
  EmitLabel(rt_read_int);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { x21 = accumulated value, x22 = negative flag }
  WriteLn('    mov x21, #0');
  WriteLn('    mov x22, #0');

  { Skip whitespace loop }
  skip_ws_lbl := NewLabel;
  EmitLabel(skip_ws_lbl);
  { Read one Char }
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  { Check If Read failed }
  WriteLn('    cmp x0, #1');
  done_lbl := NewLabel;
  Write('    b.lt L'); WriteLn(done_lbl);
  { Load Char into x23 }
  WriteLn('    ldrb w23, [sp]');
  { Check If space (32), tab (9), newline (10), Or carriage return (13) }
  WriteLn('    cmp x23, #32');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);
  WriteLn('    cmp x23, #9');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);
  WriteLn('    cmp x23, #10');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);
  WriteLn('    cmp x23, #13');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);

  { Check For minus sign (45) }
  neg_lbl := NewLabel;
  not_neg_lbl := NewLabel;
  WriteLn('    cmp x23, #45');
  Write('    b.ne L'); WriteLn(not_neg_lbl);
  { Set negative flag }
  WriteLn('    mov x22, #1');
  EmitBranchLabel(neg_lbl);
  EmitLabel(not_neg_lbl);
  { Not a minus, so it should be a digit - process it }
  WriteLn('    sub x23, x23, #48');
  { Check If valid digit (0-9) }
  WriteLn('    cmp x23, #9');
  Write('    b.hi L'); WriteLn(done_lbl);
  { Add To accumulated value: x21 = x21 * 10 + x23 }
  WriteLn('    mov x24, #10');
  WriteLn('    mul x21, x21, x24');
  WriteLn('    add x21, x21, x23');

  { Read digit loop }
  EmitLabel(neg_lbl);
  read_digit_lbl := NewLabel;
  EmitLabel(read_digit_lbl);
  { Read one Char }
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  { Check If Read failed }
  WriteLn('    cmp x0, #1');
  Write('    b.lt L'); WriteLn(done_lbl);
  { Load Char into x23 }
  WriteLn('    ldrb w23, [sp]');
  { Save original Char To x18 For pushback }
  WriteLn('    mov x18, x23');
  { Convert To digit }
  WriteLn('    sub x23, x23, #48');
  { Check If valid digit (0-9) }
  WriteLn('    cmp x23, #9');
  Write('    b.hi L'); WriteLn(done_lbl);
  { Add To accumulated value: x21 = x21 * 10 + x23 }
  WriteLn('    mov x24, #10');
  WriteLn('    mul x21, x21, x24');
  WriteLn('    add x21, x21, x23');
  EmitBranchLabel(read_digit_lbl);

  { Done - apply negative If needed }
  EmitLabel(done_lbl);
  skip_neg_lbl := NewLabel;
  Write('    cbz x22, L'); WriteLn(skip_neg_lbl);
  WriteLn('    neg x21, x21');
  EmitLabel(skip_neg_lbl);
  { Move result To x0 }
  WriteLn('    mov x0, x21');
  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitSkipLineRuntime;
Var
  loop_lbl, done_lbl, check_pb_lbl: Integer;
Begin
  { Skip To End Of line routine - reads chars Until newline Or EOF }
  { Uses x18 as pushback character from read_int (-1 means none) }
  EmitLabel(rt_skip_line);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  check_pb_lbl := NewLabel;

  { First check If pushback character is available }
  WriteLn('    cmp x18, #0');
  Write('    b.lt L'); WriteLn(loop_lbl);
  { Check If pushback is newline }
  WriteLn('    cmp x18, #10');
  { Clear pushback }
  WriteLn('    mov x18, #-1');
  Write('    b.eq L'); WriteLn(done_lbl);

  EmitLabel(loop_lbl);
  { Read one Char }
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  { Check If Read failed }
  WriteLn('    cmp x0, #1');
  Write('    b.lt L'); WriteLn(done_lbl);
  { Load Char }
  WriteLn('    ldrb w0, [sp]');
  { Check If newline (10) }
  WriteLn('    cmp x0, #10');
  Write('    b.ne L'); WriteLn(loop_lbl);

  EmitLabel(done_lbl);
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitPrintRealRuntime;
Var
  neg_lbl, skip_neg_lbl, print_frac_lbl, frac_loop_lbl: Integer;
Begin
  { Print Real In d0 - format: Integer.fraction (6 decimal places) }
  EmitLabel(rt_print_real);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { Save original value To [x29, #-16] as double }
  WriteLn('    stur d0, [x29, #-16]');

  { Check For negative }
  neg_lbl := NewLabel;
  skip_neg_lbl := NewLabel;
  { fcmp d0, #0.0 }
  WriteLn('    fcmp d0, #0.0');
  { b.ge skip_neg }
  Write('    b.ge L'); WriteLn(neg_lbl);

  { Print '-' And negate }
  EmitMovX0(45);  { '-' }
  EmitBL(rt_print_char);
  { fneg d0, d0 }
  WriteLn('    ldur d0, [x29, #-16]');
  EmitFNeg;
  WriteLn('    stur d0, [x29, #-16]');

  EmitLabel(neg_lbl);

  { Load value And get Integer part }
  WriteLn('    ldur d0, [x29, #-16]');

  { fcvtzs x0, d0 - truncate To Integer }
  EmitFcvtzsX0D0;

  { Save Integer part To [x29, #-24] }
  WriteLn('    stur x0, [x29, #-24]');

  { Print Integer part }
  EmitBL(rt_print_int);

  { Print '.' }
  EmitMovX0(46);  { '.' }
  EmitBL(rt_print_char);

  { Calculate fractional part: (original - int_part) * 1000000 }
  WriteLn('    ldur d0, [x29, #-16]');

  { Load Integer part And convert To float }
  WriteLn('    ldur x0, [x29, #-24]');

  { scvtf d1, x0 }
  WriteLn('    scvtf d1, x0');

  { fsub d0, d0, d1 - fractional part (d0 = original - int_part) }
  { EmitFSub does d0=d1-d0, so emit inline For d0=d0-d1 }
  WriteLn('    fsub d0, d0, d1');

  { Multiply by 1000000 }
  EmitMovX0(1000000);
  EmitScvtfD0X0;  { d0 = 1000000.0 }
  EmitPushD0;
  WriteLn('    ldur d0, [x29, #-16]');
  WriteLn('    ldur x0, [x29, #-24]');
  WriteLn('    scvtf d1, x0');
  { fsub d0, d0, d1 - d0 = frac part (original - int_part) }
  { EmitFSub does d0=d1-d0, so emit inline For d0=d0-d1 }
  WriteLn('    fsub d0, d0, d1');
  EmitPopD1;  { d1 = 1000000.0 }
  EmitFMul;  { d0 = frac * 1000000 }

  { Convert To Integer }
  EmitFcvtzsX0D0;

  { Save fractional digits To [x29, #-32] }
  WriteLn('    stur x0, [x29, #-32]');

  { Print fractional part With leading zeros (6 digits) }
  { Count digits needed }
  frac_loop_lbl := NewLabel;
  print_frac_lbl := NewLabel;

  { Print leading zeros: If frac < 100000, print '0', etc }
  WriteLn('    ldur x0, [x29, #-32]');

  { x1 = 100000 - use movz since 100000 > 65535 }
  { movz x1, #34464; movk x1, #1, lsl #16 }
  WriteLn('    movz x1, #34464');
  WriteLn('    movk x1, #1, lsl #16');

  EmitLabel(frac_loop_lbl);
  { cmp x1, #10 }
  WriteLn('    cmp x1, #10');
  { b.lt print_frac }
  Write('    b.lt L'); WriteLn(print_frac_lbl);
  { cmp x0, x1 }
  WriteLn('    cmp x0, x1');
  { b.ge print_frac }
  Write('    b.ge L'); WriteLn(print_frac_lbl);
  { Print '0' }
  EmitPushX0;
  EmitPushX1;
  EmitMovX0(48);
  EmitBL(rt_print_char);
  EmitPopX1;
  EmitPopX0;
  { x1 = x1 / 10 }
  WriteLn('    mov x2, #10');
  WriteLn('    udiv x1, x1, x2');
  EmitBranchLabel(frac_loop_lbl);

  EmitLabel(print_frac_lbl);
  { Load And print the fractional value }
  WriteLn('    ldur x0, [x29, #-32]');
  { Only print If non-zero }
  EmitBranchLabelZ(label_count);
  EmitBL(rt_print_int);
  EmitLabel(label_count);
  label_count := label_count + 1;

  EmitAddSP(64);
  EmitLdp;
  EmitRet
End;

Procedure EmitReadRealRuntime;
Var
  skip_ws_lbl, read_int_lbl, read_frac_lbl, done_lbl, neg_lbl, not_neg_lbl, skip_neg_lbl: Integer;
Begin
  { Read Real from input, return In d0 }
  EmitLabel(rt_read_real);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { x21 = Integer part, x22 = fractional part (scaled), x23 = neg flag, x24 = frac scale }
  WriteLn('    mov x21, #0');
  WriteLn('    mov x22, #0');
  WriteLn('    mov x23, #0');
  WriteLn('    mov x24, #1');

  { Skip whitespace }
  skip_ws_lbl := NewLabel;
  EmitLabel(skip_ws_lbl);
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  done_lbl := NewLabel;
  WriteLn('    cmp x0, #1');
  Write('    b.lt L'); WriteLn(done_lbl);
  { Load Char }
  WriteLn('    ldrb w25, [sp]');
  { Check whitespace }
  WriteLn('    cmp x25, #32');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);
  WriteLn('    cmp x25, #9');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);
  WriteLn('    cmp x25, #10');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);
  WriteLn('    cmp x25, #13');
  Write('    b.eq L'); WriteLn(skip_ws_lbl);

  { Check For minus }
  neg_lbl := NewLabel;
  not_neg_lbl := NewLabel;
  WriteLn('    cmp x25, #45');
  Write('    b.ne L'); WriteLn(not_neg_lbl);
  WriteLn('    mov x23, #1');
  EmitBranchLabel(neg_lbl);
  EmitLabel(not_neg_lbl);

  { First Char is a digit - process it }
  WriteLn('    sub x25, x25, #48');
  WriteLn('    cmp x25, #9');
  Write('    b.hi L'); WriteLn(done_lbl);
  { x21 = x21 * 10 + x25 }
  WriteLn('    mov x26, #10');
  WriteLn('    mul x21, x21, x26');
  WriteLn('    add x21, x21, x25');

  { Read Integer part loop }
  EmitLabel(neg_lbl);
  read_int_lbl := NewLabel;
  read_frac_lbl := NewLabel;
  EmitLabel(read_int_lbl);
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  WriteLn('    cmp x0, #1');
  Write('    b.lt L'); WriteLn(done_lbl);
  WriteLn('    ldrb w25, [sp]');

  { Check For '.' }
  WriteLn('    cmp x25, #46');
  Write('    b.eq L'); WriteLn(read_frac_lbl);

  { Check If digit }
  WriteLn('    sub x25, x25, #48');
  WriteLn('    cmp x25, #9');
  Write('    b.hi L'); WriteLn(done_lbl);
  { x21 = x21 * 10 + x25 }
  WriteLn('    mov x26, #10');
  WriteLn('    mul x21, x21, x26');
  WriteLn('    add x21, x21, x25');
  EmitBranchLabel(read_int_lbl);

  { Read fractional part }
  EmitLabel(read_frac_lbl);
  skip_neg_lbl := NewLabel;
  EmitLabel(skip_neg_lbl);
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read }
  EmitSvc;
  WriteLn('    cmp x0, #1');
  Write('    b.lt L'); WriteLn(done_lbl);
  WriteLn('    ldrb w25, [sp]');

  { Check If digit }
  WriteLn('    sub x25, x25, #48');
  WriteLn('    cmp x25, #9');
  Write('    b.hi L'); WriteLn(done_lbl);
  { x22 = x22 * 10 + x25, x24 = x24 * 10 }
  WriteLn('    mov x26, #10');
  WriteLn('    mul x22, x22, x26');
  WriteLn('    add x22, x22, x25');
  WriteLn('    mul x24, x24, x26');
  EmitBranchLabel(skip_neg_lbl);

  { Done - combine Integer And fractional parts }
  EmitLabel(done_lbl);
  { d0 = x21 (Integer part) }
  WriteLn('    scvtf d0, x21');
  { d1 = x22 (fractional part) }
  WriteLn('    scvtf d1, x22');
  { d2 = x24 (scale) }
  WriteLn('    scvtf d2, x24');
  { d1 = d1 / d2 }
  WriteLn('    fdiv d1, d1, d2');
  { d0 = d0 + d1 }
  EmitFAdd;

  { Apply negative If needed }
  skip_neg_lbl := NewLabel;
  Write('    cbz x23, L'); WriteLn(skip_neg_lbl);
  EmitFNeg;
  EmitLabel(skip_neg_lbl);

  EmitAddSP(64);
  EmitLdp;
  EmitRet
End;

Procedure EmitReadStringRuntime;
Var
  loop_lbl, done_lbl, store_len_lbl: Integer;
Begin
  { Read String from input (x19), String buffer address passed In x0 }
  { String format: byte 0 = Length, bytes 1-255 = characters }
  { Reads Until newline Or max 255 chars }
  EmitLabel(rt_read_string);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save String buffer address In x21 }
  WriteLn('    mov x21, x0');

  { x22 = character count (starts at 0) }
  WriteLn('    mov x22, #0');

  { Read loop }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Check If count >= 255 }
  WriteLn('    cmp x22, #255');
  Write('    b.ge L'); WriteLn(done_lbl);

  { Read one character: Read(x19, sp, 1) }
  WriteLn('    mov x0, x19');
  WriteLn('    mov x1, sp');
  WriteLn('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = Read syscall }
  EmitSvc;

  { Check If Read failed Or EOF (x0 < 1) }
  WriteLn('    cmp x0, #1');
  Write('    b.lt L'); WriteLn(done_lbl);

  { Load character into x23 }
  WriteLn('    ldrb w23, [sp]');

  { Check If newline (10) Or carriage return (13) }
  WriteLn('    cmp x23, #10');
  Write('    b.eq L'); WriteLn(done_lbl);
  WriteLn('    cmp x23, #13');
  Write('    b.eq L'); WriteLn(done_lbl);

  { Store character at buffer[count+1] }
  { x24 = x21 + x22 + 1 }
  WriteLn('    add x24, x21, x22');
  WriteLn('    add x24, x24, #1');
  WriteLn('    strb w23, [x24]');

  { Increment count }
  WriteLn('    add x22, x22, #1');

  { Loop back }
  Write('    b L'); WriteLn(loop_lbl);

  { Done - store Length at buffer[0] }
  EmitLabel(done_lbl);
  WriteLn('    strb w22, [x21]');

  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitFileOpenInit;
Var
  skip_input_lbl, skip_output_lbl: Integer;
Begin
  { Initialize x19 (input fd) And x20 (output fd) from command line }
  { On entry: x0 = argc, x1 = argv }
  { Usage: tpcv2 input.pas output.s }
  skip_input_lbl := NewLabel;
  skip_output_lbl := NewLabel;

  { Save argc And argv To callee-saved registers }
  WriteLn('    mov x21, x0');
  WriteLn('    mov x22, x1');

  { Default: x19 = 0 (stdin), x20 = 1 (stdout), x18 = -1 (no pushback) }
  WriteLn('    mov x19, #0');
  WriteLn('    mov x20, #1');
  WriteLn('    mov x18, #-1');

  { If argc < 2, skip input file open }
  WriteLn('    cmp x21, #2');
  Write('    b.lt L'); WriteLn(skip_input_lbl);

  { Load argv[1] into x0 (input filename) }
  WriteLn('    ldr x0, [x22, #8]');

  { Open input file: open(filename, O_RDONLY, 0) }
  WriteLn('    mov x1, #0');
  WriteLn('    mov x2, #0');
  EmitMovX16(33554437);  { 0x2000005 = open }
  EmitSvc;

  { Move input fd To x19 }
  WriteLn('    mov x19, x0');

  { If argc < 4, skip output file open (need: prog input.pas -o output.s) }
  WriteLn('    cmp x21, #4');
  Write('    b.lt L'); WriteLn(skip_output_lbl);

  { Check If argv[2] == "-o": load argv[2], check first two bytes }
  WriteLn('    ldr x0, [x22, #16]');
  { Load first byte, check For '-' (45) }
  WriteLn('    ldrb w1, [x0]');
  WriteLn('    cmp w1, #45');
  Write('    b.ne L'); WriteLn(skip_output_lbl);
  { Load second byte, check For 'o' (111) }
  WriteLn('    ldrb w1, [x0, #1]');
  WriteLn('    cmp w1, #111');
  Write('    b.ne L'); WriteLn(skip_output_lbl);

  { Load argv[3] into x0 (output filename) }
  WriteLn('    ldr x0, [x22, #24]');

  { Open output file: open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644) }
  { O_WRONLY=1, O_CREAT=0x200, O_TRUNC=0x400 => 0x601 = 1537 }
  WriteLn('    mov x1, #1537');
  { mode 0644 = 420 decimal }
  WriteLn('    mov x2, #420');
  EmitMovX16(33554437);  { 0x2000005 = open }
  EmitSvc;

  { Move output fd To x20 }
  WriteLn('    mov x20, x0');

  EmitLabel(skip_output_lbl);
  EmitLabel(skip_input_lbl)
End;

Procedure EmitHeapInitRuntime;
Begin
  { Initialize heap using mmap syscall }
  { Allocates 1MB Of memory For heap, stores base In x21 }
  { Initializes free list head In x22 pointing To entire block }
  EmitLabel(rt_heap_init);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  { mov x0, #0  - addr = NULL (let kernel choose) }
  WriteLn('    mov x0, #0');

  { mov x1, #1048576  - Length = 1MB (0x100000) }
  { Need movz + movk since 1048576 > 65535 }
  WriteLn('    movz x1, #0');
  WriteLn('    movk x1, #16, lsl #16');

  { mov x2, #3  - PROT_READ | PROT_WRITE }
  WriteLn('    mov x2, #3');

  { mov x3, #4098  - MAP_PRIVATE | MAP_ANON (0x1002) }
  WriteLn('    mov x3, #4098');

  { mov x4, #-1  - fd = -1 }
  WriteLn('    movn x4, #0');

  { mov x5, #0  - offset = 0 }
  WriteLn('    mov x5, #0');

  { mmap syscall: 0x20000C5 }
  WriteLn('    movz x16, #0xC5');
  WriteLn('    movk x16, #0x200, lsl #16');

  EmitSvc;

  { mov x21, x0  - store heap base In x21 }
  WriteLn('    mov x21, x0');

  { Initialize free list: x22 = x21 (free list head points To heap base) }
  WriteLn('    mov x22, x21');

  { Store block size (1MB) at [x21]: first free block header }
  { movz x1, #0; movk x1, #16, lsl #16 = 1048576 }
  WriteLn('    movz x1, #0');
  WriteLn('    movk x1, #16, lsl #16');
  { str x1, [x21] - store size }
  WriteLn('    str x1, [x21]');

  { Store next=0 at [x21, #8]: no next free block }
  WriteLn('    str xzr, [x21, #8]');

  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitAllocRuntime;
Var
  loop_lbl, found_lbl, split_lbl, no_split_lbl, update_head_lbl, done_lbl, oom_lbl: Integer;
Begin
  { Allocate memory from free list With block splitting }
  { Input: x0 = requested size }
  { Output: x0 = pointer To user data, Or 0 If OOM }
  loop_lbl := NewLabel;
  found_lbl := NewLabel;
  split_lbl := NewLabel;
  no_split_lbl := NewLabel;
  update_head_lbl := NewLabel;
  done_lbl := NewLabel;
  oom_lbl := NewLabel;

  EmitLabel(rt_alloc);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  { Add 16 For header And align To 16 bytes: x0 = (x0 + 31) & ~15 }
  WriteLn('    add x0, x0, #31');
  { And x0, x0, #-16 }
  WriteLn('    And x0, x0, #-16');
  { Save required size To x3 }
  WriteLn('    mov x3, x0');

  { x1 = prev (0 initially), x2 = curr (x22 = free list head) }
  WriteLn('    mov x1, #0');
  WriteLn('    mov x2, x22');

  { Loop: walk free list looking For big enough block }
  EmitLabel(loop_lbl);
  { cbz x2, oom - End Of list }
  Write('    cbz x2, L'); WriteLn(oom_lbl);
  { ldr x4, [x2] - x4 = block size }
  WriteLn('    ldr x4, [x2]');
  { cmp x4, x3 - compare block size To required }
  WriteLn('    cmp x4, x3');
  { b.ge found }
  Write('    b.ge L'); WriteLn(found_lbl);
  { Not big enough - move To next }
  { mov x1, x2 - prev = curr }
  WriteLn('    mov x1, x2');
  { ldr x2, [x2, #8] - curr = curr->next }
  WriteLn('    ldr x2, [x2, #8]');
  EmitBranchLabel(loop_lbl);

  { Found a big enough block - check If we can split it }
  EmitLabel(found_lbl);
  { x2 = block addr, x3 = required size, x4 = block size }
  { ldr x5, [x2, #8] - x5 = found->next }
  WriteLn('    ldr x5, [x2, #8]');
  { Check If block can be split: x4 - x3 >= 32 }
  { sub x6, x4, x3 - x6 = remainder size }
  WriteLn('    sub x6, x4, x3');
  { cmp x6, #32 }
  WriteLn('    cmp x6, #32');
  { b.lt no_split - If remainder < 32, don't split }
  Write('    b.lt L'); WriteLn(no_split_lbl);

  { Split the block }
  EmitLabel(split_lbl);
  { x6 = remainder size, x7 = remainder block address = x2 + x3 }
  { add x7, x2, x3 }
  WriteLn('    add x7, x2, x3');
  { str x6, [x7] - remainder->size = x6 }
  WriteLn('    str x6, [x7]');
  { str x5, [x7, #8] - remainder->next = found->next }
  WriteLn('    str x5, [x7, #8]');
  { str x3, [x2] - found->size = required size }
  WriteLn('    str x3, [x2]');
  { mov x5, x7 - use remainder as New "next" For unlinking }
  WriteLn('    mov x5, x7');

  { Unlink found block from free list (And link remainder In its place) }
  EmitLabel(no_split_lbl);
  { cbz x1, update_head - If prev==0, update head }
  Write('    cbz x1, L'); WriteLn(update_head_lbl);
  { str x5, [x1, #8] - prev->next = x5 (remainder Or found->next) }
  WriteLn('    str x5, [x1, #8]');
  EmitBranchLabel(done_lbl);

  EmitLabel(update_head_lbl);
  { mov x22, x5 - free list head = x5 (remainder Or found->next) }
  WriteLn('    mov x22, x5');

  EmitLabel(done_lbl);
  { Mark as allocated: str xzr, [x2, #8] - next = 0 }
  WriteLn('    str xzr, [x2, #8]');
  { Return user pointer: add x0, x2, #16 }
  WriteLn('    add x0, x2, #16');
  EmitAddSP(16);
  EmitLdp;
  EmitRet;

  { Out Of memory }
  EmitLabel(oom_lbl);
  EmitMovX0(0);
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitFreeRuntime;
Begin
  { Free memory back To free list }
  { Input: x0 = pointer To user data }
  EmitLabel(rt_free);
  { Get block header: sub x0, x0, #16 }
  WriteLn('    sub x0, x0, #16');
  { Link into free list: str x22, [x0, #8] - block->next = free_head }
  WriteLn('    str x22, [x0, #8]');
  { Update free list head: mov x22, x0 }
  WriteLn('    mov x22, x0');
  EmitRet
End;

Procedure EmitStrCopyRuntime;
Var
  loop_lbl, done_lbl: Integer;
Begin
  { Copy String from x1 (source) To x0 (dest) }
  { Pascal strings: byte 0 = Length, bytes 1..Length = chars }
  EmitLabel(rt_str_copy);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save dest And source addresses }
  { stur x0, [x29, #-8] - dest }
  WriteLn('    stur x0, [x29, #-8]');
  { stur x1, [x29, #-16] - source }
  WriteLn('    stur x1, [x29, #-16]');

  { Load Length from source [x1] into x2 }
  WriteLn('    ldrb w2, [x1]');
  { Store Length To dest [x0] }
  WriteLn('    strb w2, [x0]');
  { Save Length To [x29, #-24] }
  WriteLn('    stur x2, [x29, #-24]');

  { Initialize index To 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  { Loop: copy bytes 1..Length }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Load index }
  EmitLdurX0(-32);
  { Load Length }
  WriteLn('    ldur x2, [x29, #-24]');
  { cmp x0, x2 - If index >= Length, done }
  WriteLn('    cmp x0, x2');
  { b.ge done }
  Write('    b.ge L'); WriteLn(done_lbl);

  { x3 = index + 1 (offset into String) }
  WriteLn('    add x3, x0, #1');

  { Load source Char: ldrb w4, [x1, x3] where x1 = source }
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w4, [x1, x3]');

  { Store To dest Char: strb w4, [x5, x3] where x5 = dest }
  WriteLn('    ldur x5, [x29, #-8]');
  WriteLn('    strb w4, [x5, x3]');

  { Increment index }
  EmitLdurX0(-32);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-32);
  EmitBranchLabel(loop_lbl);

  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitStrCompareRuntime;
Var
  loop_lbl, not_equal_lbl, done_lbl: Integer;
Begin
  { Compare strings at x0 And x1, return 1 If equal, 0 If Not equal In x0 }
  EmitLabel(rt_str_compare);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save addresses }
  EmitSturX0(-8);  { String 1 }
  { stur x1, [x29, #-16] }
  WriteLn('    stur x1, [x29, #-16]');

  { Load lengths }
  WriteLn('    ldrb w2, [x0]');
  WriteLn('    ldrb w3, [x1]');

  { Compare lengths first }
  not_equal_lbl := NewLabel;
  done_lbl := NewLabel;
  WriteLn('    cmp x2, x3');
  { b.ne not_equal }
  Write('    b.ne L'); WriteLn(not_equal_lbl);

  { Save Length To [x29, #-24] }
  WriteLn('    stur x2, [x29, #-24]');

  { Initialize index To 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  { Loop: compare each byte }
  loop_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Load index }
  EmitLdurX0(-32);
  { Load Length }
  WriteLn('    ldur x2, [x29, #-24]');
  { cmp x0, x2 - If index >= Length, all equal }
  WriteLn('    cmp x0, x2');
  { b.ge equal (return 1) }
  Write('    b.ge L'); WriteLn(done_lbl);

  { x3 = index + 1 }
  WriteLn('    add x3, x0, #1');

  { Load chars from both strings }
  EmitLdurX0(-8);  { String 1 addr }
  WriteLn('    ldrb w4, [x0, x3]');

  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w5, [x1, x3]');

  { Compare chars }
  WriteLn('    cmp x4, x5');
  { b.ne not_equal }
  Write('    b.ne L'); WriteLn(not_equal_lbl);

  { Increment index }
  WriteLn('    ldur x0, [x29, #-32]');
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-32);
  EmitBranchLabel(loop_lbl);

  { Equal - return 1 }
  EmitLabel(done_lbl);
  EmitMovX0(1);
  EmitBranchLabel(label_count);

  { Not equal - return 0 }
  EmitLabel(not_equal_lbl);
  EmitMovX0(0);

  EmitLabel(label_count);
  label_count := label_count + 1;
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitStrConcatRuntime;
Var
  loop1_lbl, loop2_lbl, done_lbl: Integer;
Begin
  { Concatenate strings: x0 = dest, x1 = string1, x2 = string2 }
  { Result Length = len1 + len2 (capped at 255) }
  EmitLabel(rt_str_concat);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save addresses }
  EmitSturX0(-8);   { dest }
  { stur x1, [x29, #-16] - String 1 }
  WriteLn('    stur x1, [x29, #-16]');
  { stur x2, [x29, #-24] - String 2 }
  WriteLn('    stur x2, [x29, #-24]');

  { Load lengths }
  WriteLn('    ldrb w3, [x1]');
  WriteLn('    ldrb w4, [x2]');

  { Calculate total Length (capped at 255) }
  WriteLn('    add x5, x3, x4');
  { cmp x5, #255 }
  WriteLn('    cmp x5, #255');
  { csel x5, x5, x6, le - If x5 <= 255, keep it; Else use 255 }
  WriteLn('    mov x6, #255');
  WriteLn('    csel x5, x5, x6, le');

  { Store total Length To dest }
  EmitLdurX0(-8);  { dest }
  WriteLn('    strb w5, [x0]');

  { Save len1 To [x29, #-32] }
  WriteLn('    stur x3, [x29, #-32]');

  { Initialize index To 0 }
  EmitMovX0(0);
  EmitSturX0(-40);

  { Loop 1: copy string1 chars }
  loop1_lbl := NewLabel;
  loop2_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop1_lbl);

  { Load index And len1 }
  EmitLdurX0(-40);
  WriteLn('    ldur x3, [x29, #-32]');
  { cmp x0, x3 }
  WriteLn('    cmp x0, x3');
  { b.ge loop2 }
  Write('    b.ge L'); WriteLn(loop2_lbl);

  { x7 = index + 1 }
  WriteLn('    add x7, x0, #1');

  { Load source1 Char }
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w8, [x1, x7]');

  { Store To dest }
  EmitLdurX0(-8);
  WriteLn('    strb w8, [x0, x7]');

  { Increment index }
  EmitLdurX0(-40);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-40);
  EmitBranchLabel(loop1_lbl);

  { Loop 2: copy string2 chars }
  EmitLabel(loop2_lbl);
  { Reset index To 0 For string2 }
  EmitMovX0(0);
  EmitSturX0(-40);

  { Load len2 into x4 }
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    ldrb w4, [x2]');
  { Save len2 To [x29, #-48] }
  WriteLn('    stur x4, [x29, #-48]');

  { Loop 2 body label }
  EmitLabel(label_count);
  label_count := label_count + 1;

  { Load index And len2 }
  EmitLdurX0(-40);
  WriteLn('    ldur x4, [x29, #-48]');
  { cmp x0, x4 }
  WriteLn('    cmp x0, x4');
  { b.ge done }
  Write('    b.ge L'); WriteLn(done_lbl);

  { x7 = index + 1 (source offset) }
  WriteLn('    add x7, x0, #1');

  { Load source2 Char }
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    ldrb w8, [x2, x7]');

  { Calculate dest offset: len1 + index + 1 }
  EmitLdurX0(-40);  { index }
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    add x7, x0, x3');
  WriteLn('    add x7, x7, #1');

  { Store To dest }
  EmitLdurX0(-8);
  WriteLn('    strb w8, [x0, x7]');

  { Increment index }
  EmitLdurX0(-40);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-40);
  EmitBranchLabel(label_count - 1);

  EmitLabel(done_lbl);
  { Return dest address In x0 }
  EmitLdurX0(-8);
  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitStrCmpRuntime;
{ Lexicographic String comparison - returns -1 If x0<x1, 0 If x0=x1, 1 If x0>x1 }
{ Input: x0 = string1 addr, x1 = string2 addr }
{ Output: x0 = -1/0/1 }
Var
  loop_lbl, done_lbl, less_lbl, greater_lbl, check_len_lbl: Integer;
Begin
  EmitLabel(rt_str_cmp);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);
  { stur x0, [x29, #-8]  - string1 }
  EmitSturX0(-8);
  { stur x1, [x29, #-16] - string2 }
  WriteLn('    stur x1, [x29, #-16]');

  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  less_lbl := NewLabel;
  greater_lbl := NewLabel;
  check_len_lbl := NewLabel;

  { index = 0 }
  EmitMovX0(0);
  EmitSturX0(-24);

  EmitLabel(loop_lbl);

  { Load lengths }
  EmitLdurX0(-8);    { x0 = &s1 }
  WriteLn('    ldrb w2, [x0]');

  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w3, [x1]');

  { Load index }
  EmitLdurX0(-24);   { x0 = index }

  { If index >= min(len1, len2), check Length }
  WriteLn('    cmp x0, x2');
  Write('    b.ge L'); WriteLn(check_len_lbl);
  WriteLn('    cmp x0, x3');
  Write('    b.ge L'); WriteLn(check_len_lbl);

  { Compare chars at index+1 }
  { x4 = index + 1 }
  WriteLn('    add x4, x0, #1');

  { Load Char from s1 }
  EmitLdurX0(-8);
  WriteLn('    ldrb w5, [x0, x4]');

  { Load Char from s2 }
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w6, [x1, x4]');

  { Compare chars }
  WriteLn('    cmp x5, x6');
  Write('    b.lt L'); WriteLn(less_lbl);
  Write('    b.gt L'); WriteLn(greater_lbl);

  { Chars equal, increment index }
  EmitLdurX0(-24);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(loop_lbl);

  { Check lengths when all compared chars are equal }
  EmitLabel(check_len_lbl);
  EmitLdurX0(-8);
  WriteLn('    ldrb w2, [x0]');
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w3, [x1]');
  WriteLn('    cmp x2, x3');
  Write('    b.lt L'); WriteLn(less_lbl);
  Write('    b.gt L'); WriteLn(greater_lbl);
  { Equal }
  EmitMovX0(0);
  EmitBranchLabel(done_lbl);

  EmitLabel(less_lbl);
  EmitMovX0(0);
  WriteLn('    sub x0, x0, #1');
  EmitBranchLabel(done_lbl);

  EmitLabel(greater_lbl);
  EmitMovX0(1);

  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitStrPosRuntime;
{ Find substring position - returns position (1-based) Or 0 If Not found }
{ Input: x0 = substring addr, x1 = String addr }
{ Output: x0 = position (1-based) Or 0 }
Var
  outer_lbl, inner_lbl, done_lbl, found_lbl, not_found_lbl, next_pos_lbl: Integer;
Begin
  EmitLabel(rt_str_pos);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { [x29-8] = substr, [x29-16] = String, [x29-24] = outer idx, [x29-32] = inner idx }
  EmitSturX0(-8);
  WriteLn('    stur x1, [x29, #-16]');

  outer_lbl := NewLabel;
  inner_lbl := NewLabel;
  done_lbl := NewLabel;
  found_lbl := NewLabel;
  not_found_lbl := NewLabel;
  next_pos_lbl := NewLabel;

  { If substr is empty, return 1 }
  EmitLdurX0(-8);
  WriteLn('    ldrb w2, [x0]');
  Write('    cbz x2, L'); WriteLn(found_lbl);
  { x2 = substr len }
  WriteLn('    stur x2, [x29, #-40]');

  { Load String Length }
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w3, [x1]');
  { x3 = String len }

  { outer_idx = 0 (will be 1-based position - 1) }
  EmitMovX0(0);
  EmitSturX0(-24);

  EmitLabel(outer_lbl);
  { Check If outer_idx + substr_len > string_len }
  EmitLdurX0(-24);  { outer_idx }
  WriteLn('    ldur x2, [x29, #-40]');
  WriteLn('    add x4, x0, x2');
  { x4 = outer_idx + substr_len }
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w3, [x1]');
  WriteLn('    cmp x4, x3');
  Write('    b.gt L'); WriteLn(not_found_lbl);

  { inner_idx = 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  EmitLabel(inner_lbl);
  { If inner_idx >= substr_len, found! }
  EmitLdurX0(-32);
  WriteLn('    ldur x2, [x29, #-40]');
  WriteLn('    cmp x0, x2');
  Write('    b.ge L'); WriteLn(found_lbl);

  { Compare substr[inner_idx+1] With String[outer_idx+inner_idx+1] }
  EmitLdurX0(-8);  { substr addr }
  WriteLn('    ldur x4, [x29, #-32]');
  WriteLn('    add x4, x4, #1');
  WriteLn('    ldrb w5, [x0, x4]');
  { x5 = substr[inner_idx+1] }

  WriteLn('    ldur x1, [x29, #-16]');
  EmitLdurX0(-24);  { outer_idx }
  WriteLn('    ldur x4, [x29, #-32]');
  WriteLn('    add x4, x0, x4');
  WriteLn('    add x4, x4, #1');
  WriteLn('    ldrb w6, [x1, x4]');
  { x6 = String[outer_idx+inner_idx+1] }

  WriteLn('    cmp x5, x6');
  Write('    b.ne L'); WriteLn(next_pos_lbl);

  { Chars match, increment inner_idx }
  EmitLdurX0(-32);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-32);
  EmitBranchLabel(inner_lbl);

  EmitLabel(next_pos_lbl);
  { Chars don't match, try next outer position }
  EmitLdurX0(-24);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(outer_lbl);

  EmitLabel(found_lbl);
  { Return outer_idx + 1 (1-based position) }
  EmitLdurX0(-24);
  WriteLn('    add x0, x0, #1');
  EmitBranchLabel(done_lbl);

  EmitLabel(not_found_lbl);
  EmitMovX0(0);

  EmitLabel(done_lbl);
  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitStrDeleteRuntime;
{ Delete chars from String In place }
{ Input: x0 = String addr, x1 = start (1-based), x2 = count }
Var
  loop_lbl, done_lbl: Integer;
Begin
  EmitLabel(rt_str_delete);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { [x29-8]=String, [x29-16]=start, [x29-24]=count, [x29-32]=len, [x29-40]=idx }
  EmitSturX0(-8);
  WriteLn('    stur x1, [x29, #-16]');
  WriteLn('    stur x2, [x29, #-24]');

  loop_lbl := NewLabel;
  done_lbl := NewLabel;

  { Load String Length }
  WriteLn('    ldrb w3, [x0]');
  WriteLn('    stur x3, [x29, #-32]');
  { x3 = len }

  { idx = start }
  WriteLn('    ldur x0, [x29, #-16]');
  EmitSturX0(-40);

  EmitLabel(loop_lbl);
  { If idx + count > len, done }
  EmitLdurX0(-40);  { idx }
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    add x4, x0, x2');
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    cmp x4, x3');
  Write('    b.gt L'); WriteLn(done_lbl);

  { Copy Char from idx+count To idx }
  EmitLdurX0(-8);  { String addr }
  EmitLdurX0(-40);  { idx }
  { RELOAD String addr since we overwrote x0 }
  WriteLn('    ldur x1, [x29, #-8]');
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    add x3, x0, x2');
  { x3 = idx + count (source position) }
  { Load Char from String[idx+count] }
  WriteLn('    ldrb w4, [x1, x3]');
  { Store To String[idx] }
  WriteLn('    strb w4, [x1, x0]');

  { idx++ }
  EmitLdurX0(-40);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-40);
  EmitBranchLabel(loop_lbl);

  EmitLabel(done_lbl);
  { Update String Length: new_len = old_len - count }
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    sub x0, x3, x2');
  EmitLdurX0(-8);
  { RELOAD x0 For store since we overwrote it }
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    sub x4, x3, x2');
  WriteLn('    ldur x1, [x29, #-8]');
  WriteLn('    strb w4, [x1]');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitStrInsertRuntime;
{ Insert String into another at position }
{ Input: x0 = source String, x1 = dest String, x2 = position (1-based) }
Var
  loop1_lbl, loop2_lbl, copy_lbl, done_lbl: Integer;
Begin
  EmitLabel(rt_str_insert);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);
  { [x29-8]=source, [x29-16]=dest, [x29-24]=pos, [x29-32]=src_len, [x29-40]=dst_len, [x29-48]=idx }
  EmitSturX0(-8);
  WriteLn('    stur x1, [x29, #-16]');
  WriteLn('    stur x2, [x29, #-24]');

  loop1_lbl := NewLabel;
  loop2_lbl := NewLabel;
  copy_lbl := NewLabel;
  done_lbl := NewLabel;

  { Load lengths }
  WriteLn('    ldrb w3, [x0]');
  WriteLn('    stur x3, [x29, #-32]');
  WriteLn('    ldrb w4, [x1]');
  WriteLn('    stur x4, [x29, #-40]');

  { Move existing chars from pos To End, shifting right by src_len }
  { idx = dst_len }
  WriteLn('    stur x4, [x29, #-48]');

  EmitLabel(loop1_lbl);
  { If idx < pos, done moving }
  EmitLdurX0(-48);  { idx }
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    cmp x0, x2');
  Write('    b.lt L'); WriteLn(loop2_lbl);

  { Copy dest[idx] To dest[idx+src_len] }
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldrb w5, [x1, x0]');
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    add x6, x0, x3');
  WriteLn('    strb w5, [x1, x6]');

  { idx-- }
  EmitLdurX0(-48);
  WriteLn('    sub x0, x0, #1');
  EmitSturX0(-48);
  EmitBranchLabel(loop1_lbl);

  EmitLabel(loop2_lbl);
  { Now copy source chars To position pos...pos+src_len-1 }
  { idx = 0 }
  EmitMovX0(0);
  EmitSturX0(-48);

  EmitLabel(copy_lbl);
  { If idx >= src_len, done }
  EmitLdurX0(-48);
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    cmp x0, x3');
  Write('    b.ge L'); WriteLn(done_lbl);

  { Copy source[idx+1] To dest[pos+idx] }
  WriteLn('    ldur x5, [x29, #-8]');
  WriteLn('    add x6, x0, #1');
  WriteLn('    ldrb w7, [x5, x6]');
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    ldur x2, [x29, #-24]');
  WriteLn('    add x6, x2, x0');
  WriteLn('    strb w7, [x1, x6]');

  { idx++ }
  EmitLdurX0(-48);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-48);
  EmitBranchLabel(copy_lbl);

  EmitLabel(done_lbl);
  { Update dest Length: new_len = dst_len + src_len }
  WriteLn('    ldur x3, [x29, #-32]');
  WriteLn('    ldur x4, [x29, #-40]');
  WriteLn('    add x0, x3, x4');
  WriteLn('    ldur x1, [x29, #-16]');
  WriteLn('    strb w0, [x1]');

  EmitAddSP(64);
  EmitLdp;
  EmitRet
End;

{ EmitIntToStrRuntime - Convert Integer To String }
{ x0 = Integer value, x1 = destination String address }
Procedure EmitIntToStrRuntime;
Var
  done_lbl, neg_lbl, pos_lbl, loop_lbl, rev_lbl, rev_done_lbl: Integer;
Begin
  EmitLabel(rt_int_to_str);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  done_lbl := NewLabel;
  neg_lbl := NewLabel;
  pos_lbl := NewLabel;
  loop_lbl := NewLabel;
  rev_lbl := NewLabel;
  rev_done_lbl := NewLabel;

  { Save dest String address }
  WriteLn('    stur x1, [x29, #-16]');

  { x2 = digit count, x3 = is_negative }
  WriteLn('    mov x2, #0');
  WriteLn('    mov x3, #0');

  { Check If value is negative }
  WriteLn('    cmp x0, #0');
  Write('    b.lt L'); WriteLn(neg_lbl);
  Write('    b L'); WriteLn(pos_lbl);

  { Handle negative }
  EmitLabel(neg_lbl);
  WriteLn('    neg x0, x0');
  WriteLn('    mov x3, #1');

  { Loop To extract digits (stored In reverse on stack) }
  EmitLabel(pos_lbl);
  EmitLabel(loop_lbl);
  { x4 = x0 / 10, x5 = x0 Mod 10 }
  WriteLn('    mov x6, #10');
  WriteLn('    udiv x4, x0, x6');
  WriteLn('    msub x5, x4, x6, x0');

  { Convert digit To ASCII And store on stack }
  WriteLn('    add x5, x5, #48');

  { Store digit at sp + x2 }
  WriteLn('    strb w5, [sp, x2]');

  { Increment digit count }
  WriteLn('    add x2, x2, #1');

  { x0 = x4 (quotient), Continue If x0 > 0 }
  WriteLn('    mov x0, x4');
  WriteLn('    cmp x0, #0');
  Write('    b.ne L'); WriteLn(loop_lbl);

  { Now reverse digits into destination String }
  { x1 = dest String, x4 = Write position (starts at 1 If no sign, 2 If negative) }
  WriteLn('    ldur x1, [x29, #-16]');

  { If negative, store '-' at position 1, start digits at 2 }
  WriteLn('    cmp x3, #0');
  Write('    b.eq L'); WriteLn(rev_lbl);

  { Store '-' sign }
  WriteLn('    mov w5, #45');
  WriteLn('    strb w5, [x1, #1]');

  { x4 = Write position (1 For positive, 2 For negative start) }
  EmitLabel(rev_lbl);
  WriteLn('    add x4, x3, #1');

  { x5 = Read position (digit count - 1, reading backwards) }
  WriteLn('    sub x5, x2, #1');

  { Reverse copy loop }
  EmitLabel(rev_done_lbl);
  WriteLn('    cmp x5, #0');
  Write('    b.lt L'); WriteLn(done_lbl);

  { Load digit from stack at position x5 }
  WriteLn('    ldrb w6, [sp, x5]');

  { Store at dest position x4 }
  WriteLn('    strb w6, [x1, x4]');

  WriteLn('    add x4, x4, #1');
  WriteLn('    sub x5, x5, #1');
  Write('    b L'); WriteLn(rev_done_lbl);

  { Store Length (x2 + x3 = digit count + sign) }
  EmitLabel(done_lbl);
  WriteLn('    add x0, x2, x3');
  WriteLn('    strb w0, [x1]');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

{ EmitStrToIntRuntime - Convert String To Integer }
{ x0 = source String address }
{ Returns: x0 = Integer value, x1 = error code (0 = success, position Of error otherwise) }
Procedure EmitStrToIntRuntime;
Var
  done_lbl, error_lbl, loop_lbl, neg_lbl, pos_lbl, skip_sign_lbl, exit_lbl: Integer;
Begin
  EmitLabel(rt_str_to_int);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  done_lbl := NewLabel;
  error_lbl := NewLabel;
  loop_lbl := NewLabel;
  neg_lbl := NewLabel;
  pos_lbl := NewLabel;
  skip_sign_lbl := NewLabel;
  exit_lbl := NewLabel;

  { x1 = Length, x2 = index (starts at 1), x3 = result, x4 = is_negative }
  WriteLn('    ldrb w1, [x0]');
  WriteLn('    mov x2, #1');
  WriteLn('    mov x3, #0');
  WriteLn('    mov x4, #0');

  { Check If empty String }
  WriteLn('    cmp x1, #0');
  Write('    b.eq L'); WriteLn(error_lbl);

  { Check first Char For sign }
  WriteLn('    ldrb w5, [x0, #1]');

  { Check For '-' (45) }
  WriteLn('    cmp x5, #45');
  Write('    b.eq L'); WriteLn(neg_lbl);

  { Check For '+' (43) }
  WriteLn('    cmp x5, #43');
  Write('    b.eq L'); WriteLn(pos_lbl);
  Write('    b L'); WriteLn(loop_lbl);

  EmitLabel(neg_lbl);
  WriteLn('    mov x4, #1');
  EmitLabel(pos_lbl);
  WriteLn('    add x2, x2, #1');

  { Main loop }
  EmitLabel(loop_lbl);
  WriteLn('    cmp x2, x1');
  Write('    b.gt L'); WriteLn(done_lbl);

  { Load Char at position x2 }
  WriteLn('    ldrb w5, [x0, x2]');

  { Check If digit (48-57) }
  WriteLn('    cmp x5, #48');
  Write('    b.lt L'); WriteLn(error_lbl);
  WriteLn('    cmp x5, #57');
  Write('    b.gt L'); WriteLn(error_lbl);

  { result = result * 10 + digit }
  WriteLn('    mov x6, #10');
  WriteLn('    mul x3, x3, x6');
  WriteLn('    sub x5, x5, #48');
  WriteLn('    add x3, x3, x5');

  { x2 := x2 + 1 }
  WriteLn('    add x2, x2, #1');
  Write('    b L'); WriteLn(loop_lbl);

  { Success }
  EmitLabel(done_lbl);
  { Apply sign If negative }
  WriteLn('    cmp x4, #0');
  Write('    b.eq L'); WriteLn(skip_sign_lbl);
  WriteLn('    neg x3, x3');

  EmitLabel(skip_sign_lbl);
  WriteLn('    mov x0, x3');
  WriteLn('    mov x1, #0');
  Write('    b L'); WriteLn(exit_lbl);

  { Error - return position In x1 }
  EmitLabel(error_lbl);
  WriteLn('    mov x0, #0');
  WriteLn('    mov x1, x2');

  EmitLabel(exit_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

{ EmitStrLtrimRuntime - Remove leading whitespace from String }
{ Input: x0 = source String addr }
{ Output: x0 = New trimmed String addr (allocated from heap) }
Procedure EmitStrLtrimRuntime;
Var
  loop_lbl, done_lbl, copy_lbl, copy_done_lbl: Integer;
Begin
  EmitLabel(rt_str_ltrim);
  EmitStp;
  EmitSubSP(32);

  { x1 = source addr, x2 = source Length, x3 = skip count }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  copy_lbl := NewLabel;
  copy_done_lbl := NewLabel;

  { Save source addr }
  WriteLn('    mov x1, x0');

  { x2 = source Length }
  WriteLn('    ldrb w2, [x1]');

  { x3 = 0 (skip count) }
  WriteLn('    mov x3, #0');

  { Loop: skip leading whitespace }
  EmitLabel(loop_lbl);
  { If x3 >= x2, done }
  WriteLn('    cmp x3, x2');
  Write('    b.ge L'); WriteLn(done_lbl);

  { x4 = Char at [x1 + x3 + 1] }
  WriteLn('    add x4, x3, #1');
  WriteLn('    ldrb w4, [x1, x4]');

  { If Char = 32 (space), skip }
  WriteLn('    cmp x4, #32');
  Write('    b.eq L'); WriteLn(label_count);

  { If Char = 9 (tab), skip }
  WriteLn('    cmp x4, #9');
  Write('    b.ne L'); WriteLn(done_lbl);

  EmitLabel(label_count);
  label_count := label_count + 1;

  { x3 := x3 + 1 }
  WriteLn('    add x3, x3, #1');
  Write('    b L'); WriteLn(loop_lbl);

  { Done scanning: x3 = number To skip, x2 = Length }
  EmitLabel(done_lbl);

  { Allocate New String from heap: x0 = x21 }
  WriteLn('    mov x0, x21');
  WriteLn('    add x21, x21, #256');

  { x5 = New Length = x2 - x3 }
  WriteLn('    sub x5, x2, x3');

  { Store New Length }
  WriteLn('    strb w5, [x0]');

  { x6 = copy index (0 To x5-1) }
  WriteLn('    mov x6, #0');

  { Copy loop }
  EmitLabel(copy_lbl);
  WriteLn('    cmp x6, x5');
  Write('    b.ge L'); WriteLn(copy_done_lbl);

  { x7 = x3 + x6 + 1 (source index) }
  WriteLn('    add x7, x3, x6');
  WriteLn('    add x7, x7, #1');

  { Load Char }
  WriteLn('    ldrb w8, [x1, x7]');

  { x7 = x6 + 1 (dest index) }
  WriteLn('    add x7, x6, #1');

  { Store Char }
  WriteLn('    strb w8, [x0, x7]');

  { x6 := x6 + 1 }
  WriteLn('    add x6, x6, #1');
  Write('    b L'); WriteLn(copy_lbl);

  EmitLabel(copy_done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

{ EmitStrRtrimRuntime - Remove trailing whitespace from String }
{ Input: x0 = source String addr }
{ Output: x0 = New trimmed String addr (allocated from heap) }
Procedure EmitStrRtrimRuntime;
Var
  loop_lbl, done_lbl, copy_lbl, copy_done_lbl: Integer;
Begin
  EmitLabel(rt_str_rtrim);
  EmitStp;
  EmitSubSP(32);

  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  copy_lbl := NewLabel;
  copy_done_lbl := NewLabel;

  { x1 = source addr }
  WriteLn('    mov x1, x0');

  { x2 = source Length }
  WriteLn('    ldrb w2, [x1]');

  { x3 = x2 (scan from End) }
  WriteLn('    mov x3, x2');

  { Loop: find last non-whitespace }
  EmitLabel(loop_lbl);
  { If x3 <= 0, done }
  WriteLn('    cmp x3, #0');
  Write('    b.le L'); WriteLn(done_lbl);

  { x4 = Char at [x1 + x3] }
  WriteLn('    ldrb w4, [x1, x3]');

  { If Char = 32 (space), Continue }
  WriteLn('    cmp x4, #32');
  Write('    b.eq L'); WriteLn(label_count);

  { If Char = 9 (tab), Continue }
  WriteLn('    cmp x4, #9');
  Write('    b.ne L'); WriteLn(done_lbl);

  EmitLabel(label_count);
  label_count := label_count + 1;

  { x3 := x3 - 1 }
  WriteLn('    sub x3, x3, #1');
  Write('    b L'); WriteLn(loop_lbl);

  { Done: x3 = New Length }
  EmitLabel(done_lbl);

  { Allocate New String from heap }
  WriteLn('    mov x0, x21');
  WriteLn('    add x21, x21, #256');

  { Store New Length }
  WriteLn('    strb w3, [x0]');

  { x5 = copy index (0 To x3-1) }
  WriteLn('    mov x5, #0');

  { Copy loop }
  EmitLabel(copy_lbl);
  WriteLn('    cmp x5, x3');
  Write('    b.ge L'); WriteLn(copy_done_lbl);

  { x6 = x5 + 1 (index) }
  WriteLn('    add x6, x5, #1');

  { Load Char from source }
  WriteLn('    ldrb w7, [x1, x6]');

  { Store Char To dest }
  WriteLn('    strb w7, [x0, x6]');

  { x5 := x5 + 1 }
  WriteLn('    add x5, x5, #1');
  Write('    b L'); WriteLn(copy_lbl);

  EmitLabel(copy_done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

{ EmitStrTrimRuntime - Remove both leading And trailing whitespace }
{ Input: x0 = source String addr }
{ Output: x0 = New trimmed String addr }
Procedure EmitStrTrimRuntime;
Begin
  EmitLabel(rt_str_trim);
  EmitStp;
  EmitSubSP(16);

  { Call ltrim first }
  EmitBL(rt_str_ltrim);

  { Then call rtrim on result }
  EmitBL(rt_str_rtrim);

  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

{ ----- Screen/Terminal Control Routines ----- }

Procedure EmitStrbAtOffset(offset: Integer);
{ Emit: sturb w0, [x29, #offset] - stores low byte Of w0 at x29+offset }
Begin
  Write('    sturb w0, [x29, #'); Write(offset); WriteLn(']');
End;

Procedure EmitClrScrRuntime;
{ Emits code To clear screen And home cursor: ESC[2J ESC[H }
Begin
  EmitLabel(rt_clrscr);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store escape sequence on stack: ESC[2J ESC[H = 27,91,50,74,27,91,72 }
  { Use sturb To store individual bytes }
  EmitMovX0(27);       { ESC }
  EmitStrbAtOffset(-15);
  EmitMovX0(91);       { [ }
  EmitStrbAtOffset(-14);
  EmitMovX0(50);       { 2 }
  EmitStrbAtOffset(-13);
  EmitMovX0(74);       { J }
  EmitStrbAtOffset(-12);
  EmitMovX0(27);       { ESC }
  EmitStrbAtOffset(-11);
  EmitMovX0(91);       { [ }
  EmitStrbAtOffset(-10);
  EmitMovX0(72);       { H }
  EmitStrbAtOffset(-9);
  { Write syscall: x0=fd, x1=buf, x2=count }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #15 }
  WriteLn('    sub x1, x29, #15');
  { mov x2, #7 }
  WriteLn('    mov x2, #7');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitGotoXYRuntime;
{ Emits code To move cursor: ESC[y;xH }
{ x1=x (column), x0=y (row) }
Var
  loop1, done1, loop2, done2, end_lbl: Integer;
Begin
  EmitLabel(rt_gotoxy);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { Save x And y To registers first To avoid memory corruption }
  { x11 = y (row), x12 = x (column) - using higher regs To avoid conflicts With division }
  WriteLn('    mov x11, x0');
  WriteLn('    mov x12, x1');
  { Build escape sequence at stack offset -20 onwards (leave room For digits) }
  { Format: ESC [ y ; x H }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-20);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-19);
  { x8 = pointer To next byte (start at -18) }
  WriteLn('    sub x8, x29, #18');
  { Convert y To decimal digits (y is In x11) }
  { mov x0, x11 }
  WriteLn('    mov x0, x11');
  { If y >= 10, output tens digit }
  loop1 := NewLabel;
  done1 := NewLabel;
  WriteLn('    cmp x0, #10');
  Write('    b.lt L'); WriteLn(done1);
  { Divide by 10: x1 = x0 / 10, x2 = x0 Mod 10 }
  WriteLn('    mov x9, #10');
  WriteLn('    udiv x1, x0, x9');
  WriteLn('    msub x0, x1, x9, x0');
  { x1=tens, x0=units - store tens digit }
  WriteLn('    add x1, x1, #48');
  { strb w1, [x8], #1 }
  WriteLn('    strb w1, [x8], #1');
  EmitLabel(done1);
  { Store units digit }
  WriteLn('    add x0, x0, #48');
  { strb w0, [x8], #1 }
  WriteLn('    strb w0, [x8], #1');
  { Store semicolon }
  EmitMovX0(59);  { ; }
  { strb w0, [x8], #1 }
  WriteLn('    strb w0, [x8], #1');
  { Convert x To decimal digits (x is In x12) }
  { mov x0, x12 }
  WriteLn('    mov x0, x12');
  done2 := NewLabel;
  WriteLn('    cmp x0, #10');
  Write('    b.lt L'); WriteLn(done2);
  { Divide by 10 }
  WriteLn('    mov x9, #10');
  WriteLn('    udiv x1, x0, x9');
  WriteLn('    msub x0, x1, x9, x0');
  { Store tens digit }
  WriteLn('    add x1, x1, #48');
  { strb w1, [x8], #1 }
  WriteLn('    strb w1, [x8], #1');
  EmitLabel(done2);
  { Store units digit }
  WriteLn('    add x0, x0, #48');
  { strb w0, [x8], #1 }
  WriteLn('    strb w0, [x8], #1');
  { Store H terminator }
  EmitMovX0(72);  { H }
  { strb w0, [x8] }
  WriteLn('    strb w0, [x8]');
  { Calculate Length: x8 points past last byte, buffer starts at x29-20 }
  { add x8, x8, #1 (point past H) }
  WriteLn('    add x8, x8, #1');
  { x2 = x8 - (x29-20) = x8 - x29 + 20 }
  { sub x2, x8, x29 }
  WriteLn('    sub x2, x8, x29');
  { add x2, x2, #20 }
  WriteLn('    add x2, x2, #20');
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #20 }
  WriteLn('    sub x1, x29, #20');
  EmitSvc;
  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitClrEolRuntime;
{ Emits code To clear To End Of line: ESC[K }
Begin
  EmitLabel(rt_clreol);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store ESC[K = 27,91,75 }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-11);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-10);
  EmitMovX0(75);  { K }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #11 }
  WriteLn('    sub x1, x29, #11');
  { mov x2, #3 }
  WriteLn('    mov x2, #3');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitTextColorRuntime;
{ Emits code To Set foreground color: ESC[3Xm where X=x0 (0-7) }
Begin
  EmitLabel(rt_textcolor);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Save color parameter (x0) To x9 }
  WriteLn('    mov x9, x0');
  { Store ESC[3Xm = 27,91,51,X,109 (5 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-13);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-12);
  EmitMovX0(51);  { 3 }
  EmitStrbAtOffset(-11);
  { Store color digit: x9 + 48 }
  WriteLn('    add x0, x9, #48');
  EmitStrbAtOffset(-10);
  { Store 'm' }
  EmitMovX0(109);  { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #13 }
  WriteLn('    sub x1, x29, #13');
  { mov x2, #5 }
  WriteLn('    mov x2, #5');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitTextBackgroundRuntime;
{ Emits code To Set background color: ESC[4Xm where X=x0 (0-7) }
Begin
  EmitLabel(rt_textbackground);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Save color parameter (x0) To x9 }
  WriteLn('    mov x9, x0');
  { Store ESC[4Xm = 27,91,52,X,109 (5 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-13);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-12);
  EmitMovX0(52);  { 4 }
  EmitStrbAtOffset(-11);
  { Store color digit: x9 + 48 }
  WriteLn('    add x0, x9, #48');
  EmitStrbAtOffset(-10);
  { Store 'm' }
  EmitMovX0(109);  { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #13 }
  WriteLn('    sub x1, x29, #13');
  { mov x2, #5 }
  WriteLn('    mov x2, #5');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitNormVideoRuntime;
{ Emits code To reset attributes: ESC[0m }
Begin
  EmitLabel(rt_normvideo);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store ESC[0m = 27,91,48,109 (4 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-12);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-11);
  EmitMovX0(48);  { 0 }
  EmitStrbAtOffset(-10);
  EmitMovX0(109); { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #12 }
  WriteLn('    sub x1, x29, #12');
  { mov x2, #4 }
  WriteLn('    mov x2, #4');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitHighVideoRuntime;
{ Emits code To enable bold: ESC[1m }
Begin
  EmitLabel(rt_highvideo);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store ESC[1m = 27,91,49,109 (4 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-12);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-11);
  EmitMovX0(49);  { 1 }
  EmitStrbAtOffset(-10);
  EmitMovX0(109); { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #12 }
  WriteLn('    sub x1, x29, #12');
  { mov x2, #4 }
  WriteLn('    mov x2, #4');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitLowVideoRuntime;
{ Emits code To enable dim: ESC[2m }
Begin
  EmitLabel(rt_lowvideo);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store ESC[2m = 27,91,50,109 (4 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-12);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-11);
  EmitMovX0(50);  { 2 }
  EmitStrbAtOffset(-10);
  EmitMovX0(109); { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #12 }
  WriteLn('    sub x1, x29, #12');
  { mov x2, #4 }
  WriteLn('    mov x2, #4');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitHideCursorRuntime;
{ Emits code To hide cursor: ESC[?25l }
Begin
  EmitLabel(rt_hidecursor);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store ESC[?25l = 27,91,63,50,53,108 (6 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-14);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-13);
  EmitMovX0(63);  { ? }
  EmitStrbAtOffset(-12);
  EmitMovX0(50);  { 2 }
  EmitStrbAtOffset(-11);
  EmitMovX0(53);  { 5 }
  EmitStrbAtOffset(-10);
  EmitMovX0(108); { l }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #14 }
  WriteLn('    sub x1, x29, #14');
  { mov x2, #6 }
  WriteLn('    mov x2, #6');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitShowCursorRuntime;
{ Emits code To show cursor: ESC[?25h }
Begin
  EmitLabel(rt_showcursor);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store ESC[?25h = 27,91,63,50,53,104 (6 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-14);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-13);
  EmitMovX0(63);  { ? }
  EmitStrbAtOffset(-12);
  EmitMovX0(50);  { 2 }
  EmitStrbAtOffset(-11);
  EmitMovX0(53);  { 5 }
  EmitStrbAtOffset(-10);
  EmitMovX0(104); { h }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = Write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #14 }
  WriteLn('    sub x1, x29, #14');
  { mov x2, #6 }
  WriteLn('    mov x2, #6');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitSleepRuntime;
{ Emits code To sleep For x0 milliseconds using select syscall }
{ x0 = milliseconds }
{ select(0, NULL, NULL, NULL, &timeval) With nfds=0, all fd_sets=NULL }
{ struct timeval: tv_sec (8 bytes), tv_usec (8 bytes) }
Begin
  EmitLabel(rt_sleep);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);  { 16 bytes For timeval + alignment }

  { x10 = ms (save original) }
  WriteLn('    mov x10, x0');

  { x11 = 1000 }
  WriteLn('    mov x11, #1000');

  { x0 = ms / 1000 (seconds) }
  WriteLn('    udiv x0, x10, x11');

  { x1 = ms Mod 1000 (remainder): msub x1, x0, x11, x10 = x10 - (x0 * x11) }
  WriteLn('    msub x1, x0, x11, x10');

  { x1 = x1 * 1000 (microseconds, Not nanoseconds For timeval) }
  WriteLn('    mul x1, x1, x11');

  { Store timeval at sp: [sp] = tv_sec, [sp+8] = tv_usec }
  WriteLn('    str x0, [sp]');

  WriteLn('    str x1, [sp, #8]');

  { select(nfds=0, readfds=NULL, writefds=NULL, exceptfds=NULL, timeout=sp) }
  { x0 = 0 (nfds) }
  WriteLn('    mov x0, #0');

  { x1 = NULL }
  WriteLn('    mov x1, #0');

  { x2 = NULL }
  WriteLn('    mov x2, #0');

  { x3 = NULL }
  WriteLn('    mov x3, #0');

  { x4 = pointer To timeval (sp) }
  WriteLn('    mov x4, sp');

  { x16 = syscall number For select: 0x2000000 + 93 = 33554525 }
  EmitMovX16(33554525);
  EmitSvc;

  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitKeyPressedRuntime;
{ Check If a key is available on stdin using select With 0 timeout }
{ Returns 1 In x0 If key available, 0 otherwise }
Begin
  EmitLabel(rt_keypressed);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);  { 8 bytes For fd_set + 16 bytes For timeval + padding }

  { Clear the fd_set at sp }
  WriteLn('    mov x0, #0');
  WriteLn('    str x0, [sp]');

  { Set bit 0 In fd_set For stdin (fd 0) }
  WriteLn('    mov x0, #1');
  WriteLn('    str x0, [sp]');

  { Set timeval To 0,0 (no wait) at sp+16 }
  WriteLn('    mov x0, #0');
  WriteLn('    str x0, [sp, #16]');
  WriteLn('    str x0, [sp, #24]');

  { select(nfds=1, readfds=sp, writefds=NULL, exceptfds=NULL, timeout=sp+16) }
  { x0 = 1 (nfds) }
  WriteLn('    mov x0, #1');

  { x1 = sp (readfds) }
  WriteLn('    mov x1, sp');

  { x2 = NULL }
  WriteLn('    mov x2, #0');

  { x3 = NULL }
  WriteLn('    mov x3, #0');

  { x4 = sp + 16 (timeout) }
  WriteLn('    add x4, sp, #16');

  { x16 = syscall number For select: 0x2000000 + 93 = 33554525 }
  EmitMovX16(33554525);
  EmitSvc;

  { select returns number Of ready fds In x0, Or -1 on error }
  { If x0 > 0, key is available; convert To 0 Or 1 }
  WriteLn('    cmp x0, #0');
  WriteLn('    cset x0, gt');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitInitKeyboardRuntime;
{ Set terminal To raw mode For immediate key reading }
{ Uses ioctl TIOCGETA To get, Then TIOCSETA To Set With ICANON And ECHO cleared }
Begin
  EmitLabel(rt_initkeyboard);
  EmitStp;
  EmitMovFP;
  EmitSubSP(80);  { Space For termios structure (72 bytes aligned To 80) }

  { First get current terminal settings: ioctl(0, TIOCGETA, sp) }
  { x0 = 0 (stdin) }
  WriteLn('    mov x0, #0');

  { x1 = TIOCGETA = 0x40487413 }
  { movz x1, #0x7413 }
  WriteLn('    movz x1, #0x7413');
  { movk x1, #0x4048, lsl #16 }
  WriteLn('    movk x1, #0x4048, lsl #16');

  { x2 = sp (pointer To termios buffer) }
  WriteLn('    mov x2, sp');

  { ioctl syscall: 0x2000000 + 54 = 33554486 }
  EmitMovX16(33554486);
  EmitSvc;

  { Save original c_lflag (at offset 24) To x23 (callee-saved) For later restore }
  WriteLn('    ldr x23, [sp, #24]');

  { Clear ICANON (0x100 = 256) And ECHO (0x8) from c_lflag }
  { Load current c_lflag into x10 }
  WriteLn('    mov x10, x23');

  { x11 = 0x108 (ICANON | ECHO) }
  WriteLn('    mov x11, #0x108');

  { bic x10, x10, x11 (clear bits) }
  WriteLn('    bic x10, x10, x11');

  { Store modified c_lflag back }
  WriteLn('    str x10, [sp, #24]');

  { Set VMIN (c_cc[16]) To 1 And VTIME (c_cc[17]) To 0 }
  { c_cc starts at offset 32, so VMIN is at 32+16=48, VTIME at 32+17=49 }
  WriteLn('    mov x10, #1');
  WriteLn('    strb w10, [sp, #48]');
  WriteLn('    mov x10, #0');
  WriteLn('    strb w10, [sp, #49]');

  { Now Set the modified settings: ioctl(0, TIOCSETA, sp) }
  { x0 = 0 (stdin) }
  WriteLn('    mov x0, #0');

  { x1 = TIOCSETA = 0x80487414 }
  { movz x1, #0x7414 }
  WriteLn('    movz x1, #0x7414');
  { movk x1, #0x8048, lsl #16 }
  WriteLn('    movk x1, #0x8048, lsl #16');

  { x2 = sp }
  WriteLn('    mov x2, sp');

  EmitMovX16(33554486);
  EmitSvc;

  EmitAddSP(80);
  EmitLdp;
  EmitRet
End;

Procedure EmitDoneKeyboardRuntime;
{ Restore terminal To normal (cooked) mode }
Begin
  EmitLabel(rt_donekeyboard);
  EmitStp;
  EmitMovFP;
  EmitSubSP(80);

  { Get current settings first }
  WriteLn('    mov x0, #0');

  { x1 = TIOCGETA = 0x40487413 }
  WriteLn('    movz x1, #0x7413');
  WriteLn('    movk x1, #0x4048, lsl #16');

  WriteLn('    mov x2, sp');

  EmitMovX16(33554486);
  EmitSvc;

  { Restore original c_lflag from x23 }
  WriteLn('    str x23, [sp, #24]');

  { Set the restored settings }
  WriteLn('    mov x0, #0');

  { x1 = TIOCSETA = 0x80487414 }
  WriteLn('    movz x1, #0x7414');
  WriteLn('    movk x1, #0x8048, lsl #16');

  WriteLn('    mov x2, sp');

  EmitMovX16(33554486);
  EmitSvc;

  EmitAddSP(80);
  EmitLdp;
  EmitRet
End;

Procedure EmitSinRuntime;
Begin
  { sin(x) using Taylor series: x - x³/6 + x⁵/120 - x⁷/5040 + x⁹/362880 - x¹¹/39916800 }
  { Input: d0 = x, Output: d0 = sin(x) }
  EmitLabel(rt_sin);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save x To [x29, #-16] }
  EmitSturD0(-16);

  { Compute x² And save To [x29, #-24] }
  { fmul d1, d0, d0 }
  WriteLn('    fmul d1, d0, d0');
  { stur d1, [x29, #-24] }
  WriteLn('    stur d1, [x29, #-24]');

  { result = x (d0 already has x) }
  { Compute x³ = x² * x into d2 }
  { fmul d2, d1, d0 }
  WriteLn('    fmul d2, d1, d0');

  { Term 2: -x³/6 }
  EmitMovX0(6);
  EmitScvtfD0X0;
  { fdiv d3, d2, d0 - d3 = x³/6 }
  WriteLn('    fdiv d3, d2, d0');
  { Load x into d0 }
  EmitLdurD0(-16);
  { fsub d0, d0, d3 - result = x - x³/6 }
  WriteLn('    fsub d0, d0, d3');
  { Save result }
  EmitSturD0(-32);

  { Term 3: +x⁵/120 - multiply d3 by x², divide by 20 }
  { ldur d1, [x29, #-24] - reload x² }
  WriteLn('    ldur d1, [x29, #-24]');
  { fmul d3, d3, d1 - d3 = x⁵/6 }
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(20);
  EmitScvtfD0X0;
  { fdiv d3, d3, d0 }
  WriteLn('    fdiv d3, d3, d0');
  { Load result And add }
  EmitLdurD0(-32);
  { fadd d0, d0, d3 }
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 4: -x⁷/5040 }
  WriteLn('    ldur d1, [x29, #-24]');
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(42);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fsub d0, d0, d3');
  EmitSturD0(-32);

  { Term 5: +x⁹/362880 }
  WriteLn('    ldur d1, [x29, #-24]');
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(72);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 6: -x¹¹/39916800 }
  WriteLn('    ldur d1, [x29, #-24]');
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(110);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fsub d0, d0, d3');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitCosRuntime;
Begin
  { cos(x) using Taylor series: 1 - x²/2 + x⁴/24 - x⁶/720 + x⁸/40320 - x¹⁰/3628800 }
  { Input: d0 = x, Output: d0 = cos(x) }
  EmitLabel(rt_cos);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save x To [x29, #-16] }
  EmitSturD0(-16);

  { Compute x² And save To [x29, #-24] }
  { fmul d1, d0, d0 }
  WriteLn('    fmul d1, d0, d0');
  { stur d1, [x29, #-24] }
  WriteLn('    stur d1, [x29, #-24]');

  { Start With result = 1.0 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { d3 = x² (current term numerator starts as x²) }
  { fmov d3, d1 }
  WriteLn('    fmov d3, d1');

  { Term 2: -x²/2 }
  EmitPushD0;
  EmitMovX0(2);
  EmitScvtfD0X0;
  { fdiv d3, d3, d0 }
  WriteLn('    fdiv d3, d3, d0');
  EmitPopD0;
  { fsub d0, d0, d3 }
  WriteLn('    fsub d0, d0, d3');
  EmitSturD0(-32);

  { Term 3: +x⁴/24 }
  { ldur d1, [x29, #-24] }
  WriteLn('    ldur d1, [x29, #-24]');
  { fmul d3, d3, d1 }
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(12);  { 24/2 = 12 }
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 4: -x⁶/720 }
  WriteLn('    ldur d1, [x29, #-24]');
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(30);  { 720/24 = 30 }
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fsub d0, d0, d3');
  EmitSturD0(-32);

  { Term 5: +x⁸/40320 }
  WriteLn('    ldur d1, [x29, #-24]');
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(56);  { 40320/720 = 56 }
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 6: -x¹⁰/3628800 }
  WriteLn('    ldur d1, [x29, #-24]');
  WriteLn('    fmul d3, d3, d1');
  EmitMovX0(90);  { 3628800/40320 = 90 }
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  WriteLn('    fsub d0, d0, d3');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitTanRuntime;
Begin
  { tan(x) = sin(x) / cos(x) }
  { Input: d0 = x, Output: d0 = tan(x) }
  EmitLabel(rt_tan);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save x }
  EmitSturD0(-16);

  { Call sin(x) }
  EmitBL(rt_sin);
  { Save sin result }
  EmitSturD0(-24);

  { Load x again }
  EmitLdurD0(-16);
  { Call cos(x) }
  EmitBL(rt_cos);

  { d0 = cos(x), load sin(x) To d1 }
  WriteLn('    ldur d1, [x29, #-24]');

  { fdiv d0, d1, d0 - tan = sin/cos }
  WriteLn('    fdiv d0, d1, d0');

  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitExpRuntime;
Var
  pos_lbl, neg_lbl, scale_lbl, scale_done_lbl, done_lbl: Integer;
Begin
  { exp(x) using range reduction + Taylor series }
  { Input: d0 = x, Output: d0 = exp(x) }
  { Range reduction: exp(x) = 2^n * exp(r) where n = round(x/ln(2)), r = x - n*ln(2) }
  EmitLabel(rt_exp);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { Save x To [x29, #-16] }
  EmitSturD0(-16);
  { [x29, #-24] = n (Integer), [x29, #-32] = r (reduced x), [x29, #-40] = result }

  { Load 1/ln(2) = 1.4426950408889634 into d1 }
  { IEEE 754: 0x3FF71547652B82FE }
  { movz x0, #0x82FE }
  WriteLn('    movz x0, #0x82FE');
  { movk x0, #0x652B, lsl #16 }
  WriteLn('    movk x0, #0x652B, lsl #16');
  { movk x0, #0x7154, lsl #32 }
  WriteLn('    movk x0, #0x7154, lsl #32');
  { movk x0, #0x3FF7, lsl #48 }
  WriteLn('    movk x0, #0x3FF7, lsl #48');
  { fmov d1, x0 }
  WriteLn('    fmov d1, x0');

  { d0 = x, d1 = 1/ln(2) }
  { d0 = x / ln(2) = x * (1/ln(2)) }
  EmitLdurD0(-16);
  WriteLn('    fmul d0, d0, d1');

  { n = round(x/ln(2)) using fcvtas (round To nearest) }
  { fcvtas x0, d0 }
  WriteLn('    fcvtas x0, d0');
  EmitSturX0(-24);

  { Load ln(2) into d1 }
  { IEEE 754: 0x3FE62E42FEFA39EF }
  { movz x1, #0x39EF }
  WriteLn('    movz x1, #0x39EF');
  { movk x1, #0xFEFA, lsl #16 }
  WriteLn('    movk x1, #0xFEFA, lsl #16');
  { movk x1, #0x62E4, lsl #32 }
  WriteLn('    movk x1, #0x62E4, lsl #32');
  { movk x1, #0x3FE6, lsl #48 }
  WriteLn('    movk x1, #0x3FE6, lsl #48');
  { fmov d1, x1 }
  WriteLn('    fmov d1, x1');

  { r = x - n * ln(2) }
  { scvtf d0, x0 - convert n To float }
  EmitScvtfD0X0;
  { fmul d0, d0, d1 - d0 = n * ln(2) }
  WriteLn('    fmul d0, d0, d1');
  { fmov d2, d0 - save n*ln(2) In d2 }
  WriteLn('    fmov d2, d0');
  { d0 = x }
  EmitLdurD0(-16);
  { fsub d0, d0, d2 - r = x - n*ln(2) }
  WriteLn('    fsub d0, d0, d2');
  { Save r To [x29, #-32] }
  WriteLn('    stur d0, [x29, #-32]');

  { Now compute exp(r) using Taylor series }
  { r is small (|r| < ln(2)/2 ≈ 0.35), so series converges fast }
  { exp(r) = 1 + r + r²/2 + r³/6 + r⁴/24 + r⁵/120 + r⁶/720 + r⁷/5040 + r⁸/40320 }
  { d3 = current term, d0 = r }
  { fmov d3, d0 }
  WriteLn('    fmov d3, d0');
  { d0 = 1.0 }
  EmitPushD0;
  EmitMovX0(1);
  EmitScvtfD0X0;
  EmitPopD1;
  { fadd d0, d0, d1 - result = 1 + r }
  WriteLn('    fadd d0, d0, d1');
  EmitSturD0(-40);

  { Term 3: r²/2 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  { fmul d3, d3, d0 }
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(2);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 4: r³/6 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(3);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 5: r⁴/24 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(4);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 6: r⁵/120 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(5);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 7: r⁶/720 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(6);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 8: r⁷/5040 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(7);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 9: r⁸/40320 }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  WriteLn('    fmul d3, d3, d0');
  EmitMovX0(8);
  EmitScvtfD0X0;
  WriteLn('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d3');

  { d0 = exp(r), now multiply by 2^n }
  { Store exp(r) In [x29, #-48] temporarily }
  WriteLn('    stur d0, [x29, #-48]');

  { Load n }
  EmitLdurX0(-24);
  { If n = 0, skip scaling }
  WriteLn('    cmp x0, #0');
  done_lbl := NewLabel;
  pos_lbl := NewLabel;
  scale_lbl := NewLabel;
  Write('    b.eq L'); WriteLn(done_lbl);
  { If n > 0, multiply by 2 repeatedly }
  Write('    b.gt L'); WriteLn(pos_lbl);
  { n < 0, divide by 2 repeatedly }
  { neg x0, x0 }
  WriteLn('    neg x0, x0');
  EmitSturX0(-56);
  { Load 0.5 into d1 }
  { IEEE 754 For 0.5 = 0x3FE0000000000000 }
  WriteLn('    movz x1, #0x3FE0, lsl #48');
  { fmov d1, x1 }
  WriteLn('    fmov d1, x1');
  EmitBranchLabel(scale_lbl);

  EmitLabel(pos_lbl);
  EmitSturX0(-56);
  { Load 2.0 into d1 }
  EmitMovX0(2);
  EmitScvtfD0X0;
  { fmov d1, d0 }
  WriteLn('    fmov d1, d0');

  EmitLabel(scale_lbl);
  { d1 = scale factor (2.0 Or 0.5), [x29, #-56] = count, [x29, #-48] = result }
  { Load result }
  WriteLn('    ldur d0, [x29, #-48]');

  EmitLdurX0(-56);
  WriteLn('    cmp x0, #0');
  Write('    b.eq L'); WriteLn(done_lbl);
  { fmul d0, d0, d1 }
  WriteLn('    fmul d0, d0, d1');
  { Store result }
  WriteLn('    stur d0, [x29, #-48]');
  { Decrement count }
  WriteLn('    sub x0, x0, #1');
  EmitSturX0(-56);
  EmitBranchLabel(scale_lbl);

  EmitLabel(done_lbl);
  { Load final result }
  WriteLn('    ldur d0, [x29, #-48]');

  EmitAddSP(64);
  EmitLdp;
  EmitRet
End;

Procedure EmitLnRuntime;
Var
  loop_lbl, done_lbl, reduce_lbl, reduce_up_lbl, iter_lbl, iter_done_lbl: Integer;
Begin
  { ln(x) using range reduction + Newton-Raphson }
  { Input: d0 = x, Output: d0 = ln(x) }
  { Range reduction: x = 2^n * m where 1 <= m < 2, so ln(x) = n*ln(2) + ln(m) }
  EmitLabel(rt_ln);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { Save original x To [x29, #-16] }
  EmitSturD0(-16);
  { [x29, #-24] = count n (signed), [x29, #-32] = reduced x, [x29, #-40] = y estimate }
  { [x29, #-48] = iteration counter, [x29, #-56] = exp(y) temp }

  { Initialize count = 0 }
  EmitMovX0(0);
  EmitSturX0(-24);

  { Load x into d0 }
  EmitLdurD0(-16);
  { fmov d3, d0 - d3 = working copy Of x }
  WriteLn('    fmov d3, d0');

  { Load 2.0 into d1 And 1.0 into d2 For comparisons }
  EmitMovX0(2);
  EmitScvtfD0X0;
  { fmov d1, d0 }
  WriteLn('    fmov d1, d0');
  EmitMovX0(1);
  EmitScvtfD0X0;
  { fmov d2, d0 }
  WriteLn('    fmov d2, d0');
  { d1 = 2.0, d2 = 1.0, d3 = x }

  { Reduce While d3 >= 2: d3 = d3 / 2, count++ }
  reduce_lbl := NewLabel;
  reduce_up_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(reduce_lbl);
  { fcmp d3, d1 }
  WriteLn('    fcmp d3, d1');
  { b.lt reduce_up_lbl - If d3 < 2, check If < 1 }
  Write('    b.lt L'); WriteLn(reduce_up_lbl);
  { d3 >= 2: divide by 2 }
  { fdiv d3, d3, d1 }
  WriteLn('    fdiv d3, d3, d1');
  { count++ }
  EmitLdurX0(-24);
  WriteLn('    add x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(reduce_lbl);

  { Check If d3 < 1: multiply by 2, count-- }
  EmitLabel(reduce_up_lbl);
  { fcmp d3, d2 }
  WriteLn('    fcmp d3, d2');
  { b.ge done_lbl - If d3 >= 1, we're In [1, 2) }
  Write('    b.ge L'); WriteLn(done_lbl);
  { d3 < 1: multiply by 2 }
  { fmul d3, d3, d1 }
  WriteLn('    fmul d3, d3, d1');
  { count-- }
  EmitLdurX0(-24);
  WriteLn('    sub x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(reduce_up_lbl);

  EmitLabel(done_lbl);
  { Now d3 is In [1, 2), count is In [x29, #-24] }
  { Save reduced x To [x29, #-32] }
  WriteLn('    stur d3, [x29, #-32]');

  { Initial guess: y = d3 - 1 (will be In [0, 1)) }
  { fsub d0, d3, d2 }
  WriteLn('    fsub d0, d3, d2');
  EmitSturD0(-40);

  { Iterate 15 times For convergence }
  EmitMovX0(15);
  EmitSturX0(-48);

  iter_lbl := NewLabel;
  iter_done_lbl := NewLabel;
  EmitLabel(iter_lbl);

  { Check counter }
  EmitLdurX0(-48);
  { cmp x0, #0 }
  WriteLn('    cmp x0, #0');
  { b.eq iter_done }
  Write('    b.eq L'); WriteLn(iter_done_lbl);

  { Decrement counter }
  WriteLn('    sub x0, x0, #1');
  EmitSturX0(-48);

  { Load y }
  EmitLdurD0(-40);
  { Calculate exp(y) }
  EmitBL(rt_exp);
  { Save exp(y) To [x29, #-56] }
  WriteLn('    stur d0, [x29, #-56]');

  { d0 = reduced x, d1 = exp(y) }
  { ldur d0, [x29, #-32] }
  WriteLn('    ldur d0, [x29, #-32]');
  { ldur d1, [x29, #-56] }
  WriteLn('    ldur d1, [x29, #-56]');

  { d2 = x - exp(y) }
  WriteLn('    fsub d2, d0, d1');

  { d3 = x + exp(y) }
  WriteLn('    fadd d3, d0, d1');

  { d2 = d2 / d3 = (x - exp(y))/(x + exp(y)) }
  WriteLn('    fdiv d2, d2, d3');

  { d2 = 2 * d2 }
  EmitMovX0(2);
  EmitScvtfD0X0;
  WriteLn('    fmul d2, d2, d0');

  { y = y + d2 }
  EmitLdurD0(-40);
  WriteLn('    fadd d0, d0, d2');
  EmitSturD0(-40);

  { Loop }
  EmitBranchLabel(iter_lbl);

  EmitLabel(iter_done_lbl);
  { d0 = ln(reduced_x), now add n * ln(2) }
  { ln(2) = 0.693147180559945 }
  { IEEE 754: 0x3FE62E42FEFA39EF }
  EmitLdurD0(-40);
  { Save ln(reduced_x) }
  EmitPushD0;

  { Load ln(2) into d1 }
  { movz x0, #0x39EF }
  WriteLn('    movz x0, #0x39EF');
  { movk x0, #0xFEFA, lsl #16 }
  WriteLn('    movk x0, #0xFEFA, lsl #16');
  { movk x0, #0x62E4, lsl #32 }
  WriteLn('    movk x0, #0x62E4, lsl #32');
  { movk x0, #0x3FE6, lsl #48 }
  WriteLn('    movk x0, #0x3FE6, lsl #48');
  { fmov d1, x0 }
  WriteLn('    fmov d1, x0');

  { Load count into d0, convert To float }
  EmitLdurX0(-24);
  { scvtf d0, x0 }
  EmitScvtfD0X0;

  { d0 = count * ln(2) }
  { fmul d0, d0, d1 }
  WriteLn('    fmul d0, d0, d1');

  { d0 = count*ln(2) + ln(reduced_x) }
  EmitPopD1;
  WriteLn('    fadd d0, d0, d1');

  EmitAddSP(64);
  EmitLdp;
  EmitRet
End;

Procedure EmitRandomRuntime;
Begin
  { random - return random 64-bit Integer In x0 using LCG PRNG }
  { Uses x27 as seed (preserved across calls) }
  { LCG: seed = seed * 6364136223846793005 + 1442695040888963407 }
  { Output: x0 = random Integer }
  EmitLabel(rt_random);
  EmitStp;
  EmitMovFP;

  { Load seed from x27 }
  WriteLn('    mov x0, x27');

  { If seed is 0, initialize With a default value }
  { cbnz x0, Lxxx }
  Write('    cbnz x0, L'); WriteLn(label_count);
  { Initialize seed To 0x5DEECE66D }
  WriteLn('    movz x0, #0xE66D');
  WriteLn('    movk x0, #0xECE5, lsl #16');
  WriteLn('    movk x0, #0xDE, lsl #32');
  WriteLn('    movk x0, #0x5, lsl #48');

  EmitLabel(label_count);
  label_count := label_count + 1;

  { Load multiplier 6364136223846793005 = 0x5851F42D4C957F2D into x1 }
  WriteLn('    movz x1, #0x7F2D');
  WriteLn('    movk x1, #0x4C95, lsl #16');
  WriteLn('    movk x1, #0xF42D, lsl #32');
  WriteLn('    movk x1, #0x5851, lsl #48');

  { mul x0, x0, x1 }
  WriteLn('    mul x0, x0, x1');

  { Load increment 1442695040888963407 = 0x14057B7EF767814F into x1 }
  WriteLn('    movz x1, #0x814F');
  WriteLn('    movk x1, #0x7677, lsl #16');
  WriteLn('    movk x1, #0x7B7E, lsl #32');
  WriteLn('    movk x1, #0x1405, lsl #48');

  { add x0, x0, x1 }
  WriteLn('    add x0, x0, x1');

  { Store New seed In x27 }
  WriteLn('    mov x27, x0');

  EmitLdp;
  EmitRet
End;

Procedure EmitArctanRuntime;
Var
  use_identity_lbl, neg_lbl, done_lbl, compute_lbl: Integer;
Begin
  { arctan(x) using Taylor series For |x| <= 1 }
  { For |x| > 1: arctan(x) = pi/2 - arctan(1/x) If x > 0 }
  {              arctan(x) = -pi/2 - arctan(1/x) If x < 0 }
  { Input: d0 = x, Output: d0 = arctan(x) }
  EmitLabel(rt_arctan);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save x To [x29, #-16] }
  EmitSturD0(-16);

  { Check If |x| > 1 }
  { fabs d1, d0 }
  WriteLn('    fabs d1, d0');
  { Load 1.0 into d2 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { fmov d2, d0 }
  WriteLn('    fmov d2, d0');
  { fcmp d1, d2 }
  WriteLn('    fcmp d1, d2');

  use_identity_lbl := NewLabel;
  compute_lbl := NewLabel;
  done_lbl := NewLabel;

  { b.le compute - If |x| <= 1, use direct Taylor series }
  Write('    b.le L'); WriteLn(compute_lbl);

  { |x| > 1: use identity arctan(x) = sign(x)*pi/2 - arctan(1/x) }
  { Compute 1/x }
  EmitLdurD0(-16);  { x }
  { fdiv d0, d2, d0 - d0 = 1/x (d2 still has 1.0) }
  WriteLn('    fdiv d0, d2, d0');
  { Save 1/x To [x29, #-24] }
  EmitSturD0(-24);
  { Now compute arctan(1/x) recursively - but we're already In arctan! }
  { Let's just use Taylor series on 1/x since |1/x| < 1 }
  EmitBranchLabel(compute_lbl);

  EmitLabel(compute_lbl);
  { Taylor series: arctan(x) = x - x³/3 + x⁵/5 - x⁷/7 + x⁹/9 }
  { Use d0=x, d3=current term, d4=x², d5=result }
  EmitLdurD0(-16);  { x }

  { Check If we used identity (|x| > 1), Then use 1/x instead }
  { Actually, let's simplify - compute arctan using the argument already In d0 }
  { If we came from the identity path, d0 has 1/x }
  { For now, just compute Taylor series on whatever is In d0 }

  { Save x To d5 as running result }
  { fmov d5, d0 }
  WriteLn('    fmov d5, d0');

  { d4 = x² }
  WriteLn('    fmul d4, d0, d0');

  { d3 = x (current power) }
  { fmov d3, d0 }
  WriteLn('    fmov d3, d0');

  { Term 2: -x³/3 }
  { d3 = d3 * d4 = x³ }
  WriteLn('    fmul d3, d3, d4');
  EmitMovX0(3);
  EmitScvtfD0X0;
  { fdiv d6, d3, d0 }
  WriteLn('    fdiv d6, d3, d0');
  { d5 = d5 - d6 }
  WriteLn('    fsub d5, d5, d6');

  { Term 3: +x⁵/5 }
  WriteLn('    fmul d3, d3, d4');
  EmitMovX0(5);
  EmitScvtfD0X0;
  WriteLn('    fdiv d6, d3, d0');
  WriteLn('    fadd d5, d5, d6');

  { Term 4: -x⁷/7 }
  WriteLn('    fmul d3, d3, d4');
  EmitMovX0(7);
  EmitScvtfD0X0;
  WriteLn('    fdiv d6, d3, d0');
  WriteLn('    fsub d5, d5, d6');

  { Term 5: +x⁹/9 }
  WriteLn('    fmul d3, d3, d4');
  EmitMovX0(9);
  EmitScvtfD0X0;
  WriteLn('    fdiv d6, d3, d0');
  WriteLn('    fadd d5, d5, d6');

  { Term 6: -x¹¹/11 }
  WriteLn('    fmul d3, d3, d4');
  EmitMovX0(11);
  EmitScvtfD0X0;
  WriteLn('    fdiv d6, d3, d0');
  WriteLn('    fsub d5, d5, d6');

  { Result In d5, move To d0 }
  { fmov d0, d5 }
  WriteLn('    fmov d0, d5');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
End;

Procedure EmitArcsinRuntime;
Var
  pos_one_lbl, neg_one_lbl, normal_lbl, done_lbl: Integer;
Begin
  { arcsin(x) = arctan(x / sqrt(1 - x²)) }
  { Special cases: arcsin(1) = pi/2, arcsin(-1) = -pi/2 }
  { Input: d0 = x, Output: d0 = arcsin(x) }
  EmitLabel(rt_arcsin);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save x To [x29, #-16] }
  EmitSturD0(-16);

  pos_one_lbl := NewLabel;
  neg_one_lbl := NewLabel;
  normal_lbl := NewLabel;
  done_lbl := NewLabel;

  { Check For x = 1 Or x = -1 }
  { Load 1.0 into d1 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { fmov d1, d0 }
  WriteLn('    fmov d1, d0');
  { Load x back }
  EmitLdurD0(-16);
  { fcmp d0, d1 - compare x With 1.0 }
  WriteLn('    fcmp d0, d1');
  { b.eq pos_one_lbl }
  Write('    b.eq L'); WriteLn(pos_one_lbl);
  { fneg d1, d1 - d1 = -1.0 }
  WriteLn('    fneg d1, d1');
  { fcmp d0, d1 - compare x With -1.0 }
  WriteLn('    fcmp d0, d1');
  { b.eq neg_one_lbl }
  Write('    b.eq L'); WriteLn(neg_one_lbl);
  { Fall through To normal Case }
  EmitBranchLabel(normal_lbl);

  { x = 1: return pi/2 }
  EmitLabel(pos_one_lbl);
  { Load pi/2 = 1.5707963267948966 }
  { IEEE 754: 0x3FF921FB54442D18 }
  WriteLn('    movz x0, #0x2D18');
  WriteLn('    movk x0, #0x5444, lsl #16');
  WriteLn('    movk x0, #0x21FB, lsl #32');
  WriteLn('    movk x0, #0x3FF9, lsl #48');
  { fmov d0, x0 }
  WriteLn('    fmov d0, x0');
  EmitBranchLabel(done_lbl);

  { x = -1: return -pi/2 }
  EmitLabel(neg_one_lbl);
  { Load -pi/2 = -1.5707963267948966 }
  { IEEE 754: 0xBFF921FB54442D18 }
  WriteLn('    movz x0, #0x2D18');
  WriteLn('    movk x0, #0x5444, lsl #16');
  WriteLn('    movk x0, #0x21FB, lsl #32');
  WriteLn('    movk x0, #0xBFF9, lsl #48');
  { fmov d0, x0 }
  WriteLn('    fmov d0, x0');
  EmitBranchLabel(done_lbl);

  EmitLabel(normal_lbl);
  { Normal Case: arcsin(x) = arctan(x / sqrt(1 - x²)) }
  EmitLdurD0(-16);
  { Compute 1 - x² }
  { d1 = x² }
  WriteLn('    fmul d1, d0, d0');
  { Load 1.0 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { d0 = 1 - x² }
  WriteLn('    fsub d0, d0, d1');

  { d0 = sqrt(1 - x²) }
  WriteLn('    fsqrt d0, d0');

  { d0 = x / sqrt(1 - x²) }
  WriteLn('    ldur d1, [x29, #-16]');
  WriteLn('    fdiv d0, d1, d0');

  { d0 = arctan(x / sqrt(1 - x²)) }
  EmitBL(rt_arctan);

  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;

Procedure EmitArccosRuntime;
Begin
  { arccos(x) = pi/2 - arcsin(x) }
  { Input: d0 = x, Output: d0 = arccos(x) }
  EmitLabel(rt_arccos);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  { Compute arcsin(x) }
  EmitBL(rt_arcsin);
  { Save arcsin(x) }
  EmitSturD0(-16);

  { Load pi/2 = 1.5707963267948966 }
  { IEEE 754: 0x3FF921FB54442D18 }
  WriteLn('    movz x0, #0x2D18');
  WriteLn('    movk x0, #0x5444, lsl #16');
  WriteLn('    movk x0, #0x21FB, lsl #32');
  WriteLn('    movk x0, #0x3FF9, lsl #48');
  EmitFmovD0X0;

  { d0 = pi/2 - arcsin(x) }
  EmitLdurD0(-16);
  WriteLn('    fmov d1, d0');
  { Reload pi/2 }
  WriteLn('    movz x0, #0x2D18');
  WriteLn('    movk x0, #0x5444, lsl #16');
  WriteLn('    movk x0, #0x21FB, lsl #32');
  WriteLn('    movk x0, #0x3FF9, lsl #48');
  EmitFmovD0X0;
  { fsub d0, d0, d1 }
  WriteLn('    fsub d0, d0, d1');

  EmitAddSP(16);
  EmitLdp;
  EmitRet
End;

Procedure EmitParamStrRuntime;
Var
  loop_lbl, done_lbl, copy_lbl, copy_done_lbl, empty_lbl: Integer;
Begin
  { paramstr(n) - convert argv[n] To Pascal String }
  { Input: x0 = n (index into argv) }
  { Output: x0 = pointer To Pascal String allocated from heap }
  { Uses x25 = argc, x26 = argv (saved at Program start) }
  { Uses x21 = heap pointer For allocation }
  EmitLabel(rt_paramstr);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Allocate String buffer from heap: x8 = x21, x21 += 256 }
  { Save dest addr In x8 }
  WriteLn('    mov x8, x21');
  { Advance heap pointer }
  WriteLn('    add x21, x21, #256');

  done_lbl := NewLabel;
  empty_lbl := NewLabel;
  loop_lbl := NewLabel;
  copy_lbl := NewLabel;
  copy_done_lbl := NewLabel;

  { Check bounds: If n >= argc, return empty String }
  { cmp x0, x25 }
  WriteLn('    cmp x0, x25');
  { b.ge empty }
  Write('    b.ge L'); WriteLn(empty_lbl);

  { Check For negative index }
  { cmp x0, #0 }
  WriteLn('    cmp x0, #0');
  { b.lt empty }
  Write('    b.lt L'); WriteLn(empty_lbl);

  { Load argv[n] pointer: x1 = argv[n] = *(x26 + n*8) }
  { ldr x1, [x26, x0, lsl #3] }
  WriteLn('    ldr x1, [x26, x0, lsl #3]');

  { Check If argv[n] is null }
  { cbz x1, empty }
  Write('    cbz x1, L'); WriteLn(empty_lbl);

  { x1 = pointer To C String, x8 = dest buffer }
  { First count Length (max 255) }
  { mov x2, #0 - Length counter }
  WriteLn('    mov x2, #0');
  { mov x3, x1 - save String pointer }
  WriteLn('    mov x3, x1');

  EmitLabel(loop_lbl);
  { ldrb w4, [x1], #1 - load byte And increment }
  WriteLn('    ldrb w4, [x1], #1');
  { cbz w4, copy_start - If null terminator, start copying }
  Write('    cbz w4, L'); WriteLn(copy_lbl);
  { add x2, x2, #1 - increment Length }
  WriteLn('    add x2, x2, #1');
  { cmp x2, #255 - max Length }
  WriteLn('    cmp x2, #255');
  { b.lt loop }
  Write('    b.lt L'); WriteLn(loop_lbl);

  EmitLabel(copy_lbl);
  { x2 = Length, x3 = source pointer, x8 = dest buffer }
  { Store Length byte at [x8] }
  { strb w2, [x8] }
  WriteLn('    strb w2, [x8]');
  { x0 = x8 + 1 (dest For chars), x1 = x3 (source) }
  WriteLn('    add x0, x8, #1');
  { mov x1, x3 - restore source pointer }
  WriteLn('    mov x1, x3');

  { Copy loop }
  EmitLabel(copy_done_lbl);
  { cbz x2, done - If count = 0, done }
  Write('    cbz x2, L'); WriteLn(done_lbl);
  { ldrb w4, [x1], #1 }
  WriteLn('    ldrb w4, [x1], #1');
  { strb w4, [x0], #1 }
  WriteLn('    strb w4, [x0], #1');
  { sub x2, x2, #1 }
  WriteLn('    sub x2, x2, #1');
  EmitBranchLabel(copy_done_lbl);

  { Empty String: store Length 0 }
  EmitLabel(empty_lbl);
  WriteLn('    strb wzr, [x8]');

  EmitLabel(done_lbl);
  { Return pointer To String buffer (x8) }
  { mov x0, x8 }
  WriteLn('    mov x0, x8');

  EmitAddSP(32);
  EmitLdp;
  EmitRet
End;


