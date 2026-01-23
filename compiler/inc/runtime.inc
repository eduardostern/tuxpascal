{ ----- Print Runtime ----- }

procedure EmitPrintIntRuntime;
var
  loop_lbl, done_lbl, neg_lbl, print_lbl: integer;
begin
  { Runtime routine to print integer in x0 }
  EmitLabel(rt_print_int);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { Save value }
  EmitSturX0(-24);

  { Handle negative }
  neg_lbl := NewLabel;
  done_lbl := NewLabel;
  writeln('    cmp x0, #0');

  write('    b.ge L'); writeln(neg_lbl);

  { Print minus sign }
  EmitMovX0(1);
  EmitSturX0(-32);
  EmitMovX0(45);  { '-' }
  EmitSturX0(-8);
  EmitMovX16(33554436); { 0x2000004 }
  EmitMovX0X20;
  writeln('    sub x1, x29, #8');
  writeln('    mov x2, #1');
  EmitSvc;

  { Negate }
  EmitLdurX0(-24);
  EmitNeg;
  EmitSturX0(-24);

  EmitLabel(neg_lbl);

  { Convert to string (digits in reverse) }
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
  writeln('    add x0, x0, #48');

  writeln('    ldur x1, [x29, #-40]');

  writeln('    sub x2, x29, #48');

  writeln('    strb w0, [x2, x1]');

  { digit count++ }
  writeln('    add x1, x1, #1');

  writeln('    stur x1, [x29, #-40]');

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

  { Print digits in reverse order }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);
  EmitLdurX0(-40);
  EmitBranchLabelZ(done_lbl);

  { digit count-- }
  writeln('    sub x0, x0, #1');
  EmitSturX0(-40);

  { Load digit }
  writeln('    sub x1, x29, #48');

  writeln('    ldrb w0, [x1, x0]');

  { Print char }
  EmitSturX0(-8);
  EmitMovX16(33554436);
  EmitMovX0X20;
  writeln('    sub x1, x29, #8');
  writeln('    mov x2, #1');
  EmitSvc;

  EmitBranchLabel(loop_lbl);

  { Exit label }
  EmitLabel(done_lbl);

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitNewlineRuntime;
begin
  { Newline routine - print chr(10) }
  EmitLabel(rt_newline);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  EmitMovX0(10);
  EmitSturX0(-9);
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;
  writeln('    sub x1, x29, #9');
  writeln('    mov x2, #1');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitReadcharRuntime;
begin
  { Readchar routine - read one char, return in x0 (-1 for EOF) }
  { Uses x19 as input file descriptor (0=stdin, or opened file) }
  EmitLabel(rt_readchar);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read returned >= 1 }
  writeln('    cmp x0, #1');
  write('    b.ge L'); writeln(label_count);
  EmitMovX0(-1);  { EOF }
  EmitBranchLabel(label_count + 1);
  EmitLabel(label_count);
  label_count := label_count + 1;
  writeln('    ldrb w0, [sp]');
  EmitLabel(label_count);
  label_count := label_count + 1;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitPrintCharRuntime;
begin
  { Print char routine - print char in x0 }
  EmitLabel(rt_print_char);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  writeln('    strb w0, [sp]');
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitWriteCharFdRuntime;
begin
  { Write char to fd routine - x0=fd, x1=char }
  EmitLabel(rt_write_char_fd);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store char on stack }
  writeln('    strb w1, [sp]');
  { x0 already has fd, set up rest of syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitPrintStringRuntime;
var
  loop_lbl, done_lbl: integer;
begin
  { Print string routine - x0 = address of pascal string (length byte + chars) }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(rt_print_string);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);
  { Save base address to [x29, #-8] }
  writeln('    stur x0, [x29, #-8]');
  { Load length from [x0] into x1 and save to [x29, #-16] }
  writeln('    ldrb w1, [x0]');
  writeln('    stur x1, [x29, #-16]');
  { Initialize index to 0 at [x29, #-24] }
  writeln('    mov x2, #0');
  writeln('    stur x2, [x29, #-24]');
  { Loop label }
  EmitLabel(loop_lbl);
  { Load index and length, compare }
  writeln('    ldur x2, [x29, #-24]');
  writeln('    ldur x1, [x29, #-16]');
  writeln('    cmp x2, x1');
  { b.ge done_lbl }
  write('    b.ge L'); writeln(done_lbl);
  { Load char at [base + index + 1] }
  writeln('    ldur x0, [x29, #-8]');
  writeln('    add x0, x0, x2');
  writeln('    add x0, x0, #1');
  writeln('    ldrb w0, [x0]');
  { Call print_char }
  EmitBL(rt_print_char);
  { Increment index }
  writeln('    ldur x2, [x29, #-24]');
  writeln('    add x2, x2, #1');
  writeln('    stur x2, [x29, #-24]');
  { Branch back to loop }
  EmitBranchLabel(loop_lbl);
  { Done label }
  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitReadIntRuntime;
var
  skip_ws_lbl, read_digit_lbl, done_lbl, neg_lbl, not_neg_lbl, skip_neg_lbl: integer;
begin
  { Read integer routine - reads from x19 (input fd), returns in x0 }
  { Skips whitespace, handles optional minus sign, reads digits }
  EmitLabel(rt_read_int);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { x21 = accumulated value, x22 = negative flag }
  writeln('    mov x21, #0');
  writeln('    mov x22, #0');

  { Skip whitespace loop }
  skip_ws_lbl := NewLabel;
  EmitLabel(skip_ws_lbl);
  { Read one char }
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read failed }
  writeln('    cmp x0, #1');
  done_lbl := NewLabel;
  write('    b.lt L'); writeln(done_lbl);
  { Load char into x23 }
  writeln('    ldrb w23, [sp]');
  { Check if space (32), tab (9), newline (10), or carriage return (13) }
  writeln('    cmp x23, #32');
  write('    b.eq L'); writeln(skip_ws_lbl);
  writeln('    cmp x23, #9');
  write('    b.eq L'); writeln(skip_ws_lbl);
  writeln('    cmp x23, #10');
  write('    b.eq L'); writeln(skip_ws_lbl);
  writeln('    cmp x23, #13');
  write('    b.eq L'); writeln(skip_ws_lbl);

  { Check for minus sign (45) }
  neg_lbl := NewLabel;
  not_neg_lbl := NewLabel;
  writeln('    cmp x23, #45');
  write('    b.ne L'); writeln(not_neg_lbl);
  { Set negative flag }
  writeln('    mov x22, #1');
  EmitBranchLabel(neg_lbl);
  EmitLabel(not_neg_lbl);
  { Not a minus, so it should be a digit - process it }
  writeln('    sub x23, x23, #48');
  { Check if valid digit (0-9) }
  writeln('    cmp x23, #9');
  write('    b.hi L'); writeln(done_lbl);
  { Add to accumulated value: x21 = x21 * 10 + x23 }
  writeln('    mov x24, #10');
  writeln('    mul x21, x21, x24');
  writeln('    add x21, x21, x23');

  { Read digit loop }
  EmitLabel(neg_lbl);
  read_digit_lbl := NewLabel;
  EmitLabel(read_digit_lbl);
  { Read one char }
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read failed }
  writeln('    cmp x0, #1');
  write('    b.lt L'); writeln(done_lbl);
  { Load char into x23 }
  writeln('    ldrb w23, [sp]');
  { Save original char to x18 for pushback }
  writeln('    mov x18, x23');
  { Convert to digit }
  writeln('    sub x23, x23, #48');
  { Check if valid digit (0-9) }
  writeln('    cmp x23, #9');
  write('    b.hi L'); writeln(done_lbl);
  { Add to accumulated value: x21 = x21 * 10 + x23 }
  writeln('    mov x24, #10');
  writeln('    mul x21, x21, x24');
  writeln('    add x21, x21, x23');
  EmitBranchLabel(read_digit_lbl);

  { Done - apply negative if needed }
  EmitLabel(done_lbl);
  skip_neg_lbl := NewLabel;
  write('    cbz x22, L'); writeln(skip_neg_lbl);
  writeln('    neg x21, x21');
  EmitLabel(skip_neg_lbl);
  { Move result to x0 }
  writeln('    mov x0, x21');
  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitSkipLineRuntime;
var
  loop_lbl, done_lbl, check_pb_lbl: integer;
begin
  { Skip to end of line routine - reads chars until newline or EOF }
  { Uses x18 as pushback character from read_int (-1 means none) }
  EmitLabel(rt_skip_line);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  check_pb_lbl := NewLabel;

  { First check if pushback character is available }
  writeln('    cmp x18, #0');
  write('    b.lt L'); writeln(loop_lbl);
  { Check if pushback is newline }
  writeln('    cmp x18, #10');
  { Clear pushback }
  writeln('    mov x18, #-1');
  write('    b.eq L'); writeln(done_lbl);

  EmitLabel(loop_lbl);
  { Read one char }
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read failed }
  writeln('    cmp x0, #1');
  write('    b.lt L'); writeln(done_lbl);
  { Load char }
  writeln('    ldrb w0, [sp]');
  { Check if newline (10) }
  writeln('    cmp x0, #10');
  write('    b.ne L'); writeln(loop_lbl);

  EmitLabel(done_lbl);
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitPrintRealRuntime;
var
  neg_lbl, skip_neg_lbl, print_frac_lbl, frac_loop_lbl: integer;
begin
  { Print real in d0 - format: integer.fraction (6 decimal places) }
  EmitLabel(rt_print_real);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { Save original value to [x29, #-16] as double }
  writeln('    stur d0, [x29, #-16]');

  { Check for negative }
  neg_lbl := NewLabel;
  skip_neg_lbl := NewLabel;
  { fcmp d0, #0.0 }
  writeln('    fcmp d0, #0.0');
  { b.ge skip_neg }
  write('    b.ge L'); writeln(neg_lbl);

  { Print '-' and negate }
  EmitMovX0(45);  { '-' }
  EmitBL(rt_print_char);
  { fneg d0, d0 }
  writeln('    ldur d0, [x29, #-16]');
  EmitFNeg;
  writeln('    stur d0, [x29, #-16]');

  EmitLabel(neg_lbl);

  { Load value and get integer part }
  writeln('    ldur d0, [x29, #-16]');

  { fcvtzs x0, d0 - truncate to integer }
  EmitFcvtzsX0D0;

  { Save integer part to [x29, #-24] }
  writeln('    stur x0, [x29, #-24]');

  { Print integer part }
  EmitBL(rt_print_int);

  { Print '.' }
  EmitMovX0(46);  { '.' }
  EmitBL(rt_print_char);

  { Calculate fractional part: (original - int_part) * 1000000 }
  writeln('    ldur d0, [x29, #-16]');

  { Load integer part and convert to float }
  writeln('    ldur x0, [x29, #-24]');

  { scvtf d1, x0 }
  writeln('    scvtf d1, x0');

  { fsub d0, d0, d1 - fractional part (d0 = original - int_part) }
  { EmitFSub does d0=d1-d0, so emit inline for d0=d0-d1 }
  writeln('    fsub d0, d0, d1');

  { Multiply by 1000000 }
  EmitMovX0(1000000);
  EmitScvtfD0X0;  { d0 = 1000000.0 }
  EmitPushD0;
  writeln('    ldur d0, [x29, #-16]');
  writeln('    ldur x0, [x29, #-24]');
  writeln('    scvtf d1, x0');
  { fsub d0, d0, d1 - d0 = frac part (original - int_part) }
  { EmitFSub does d0=d1-d0, so emit inline for d0=d0-d1 }
  writeln('    fsub d0, d0, d1');
  EmitPopD1;  { d1 = 1000000.0 }
  EmitFMul;  { d0 = frac * 1000000 }

  { Convert to integer }
  EmitFcvtzsX0D0;

  { Save fractional digits to [x29, #-32] }
  writeln('    stur x0, [x29, #-32]');

  { Print fractional part with leading zeros (6 digits) }
  { Count digits needed }
  frac_loop_lbl := NewLabel;
  print_frac_lbl := NewLabel;

  { Print leading zeros: if frac < 100000, print '0', etc }
  writeln('    ldur x0, [x29, #-32]');

  { x1 = 100000 - use movz since 100000 > 65535 }
  { movz x1, #34464; movk x1, #1, lsl #16 }
  writeln('    movz x1, #34464');
  writeln('    movk x1, #1, lsl #16');

  EmitLabel(frac_loop_lbl);
  { cmp x1, #10 }
  writeln('    cmp x1, #10');
  { b.lt print_frac }
  write('    b.lt L'); writeln(print_frac_lbl);
  { cmp x0, x1 }
  writeln('    cmp x0, x1');
  { b.ge print_frac }
  write('    b.ge L'); writeln(print_frac_lbl);
  { Print '0' }
  EmitPushX0;
  EmitPushX1;
  EmitMovX0(48);
  EmitBL(rt_print_char);
  EmitPopX1;
  EmitPopX0;
  { x1 = x1 / 10 }
  writeln('    mov x2, #10');
  writeln('    udiv x1, x1, x2');
  EmitBranchLabel(frac_loop_lbl);

  EmitLabel(print_frac_lbl);
  { Load and print the fractional value }
  writeln('    ldur x0, [x29, #-32]');
  { Only print if non-zero }
  EmitBranchLabelZ(label_count);
  EmitBL(rt_print_int);
  EmitLabel(label_count);
  label_count := label_count + 1;

  EmitAddSP(64);
  EmitLdp;
  EmitRet
end;

procedure EmitReadRealRuntime;
var
  skip_ws_lbl, read_int_lbl, read_frac_lbl, done_lbl, neg_lbl, not_neg_lbl, skip_neg_lbl: integer;
begin
  { Read real from input, return in d0 }
  EmitLabel(rt_read_real);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { x21 = integer part, x22 = fractional part (scaled), x23 = neg flag, x24 = frac scale }
  writeln('    mov x21, #0');
  writeln('    mov x22, #0');
  writeln('    mov x23, #0');
  writeln('    mov x24, #1');

  { Skip whitespace }
  skip_ws_lbl := NewLabel;
  EmitLabel(skip_ws_lbl);
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  done_lbl := NewLabel;
  writeln('    cmp x0, #1');
  write('    b.lt L'); writeln(done_lbl);
  { Load char }
  writeln('    ldrb w25, [sp]');
  { Check whitespace }
  writeln('    cmp x25, #32');
  write('    b.eq L'); writeln(skip_ws_lbl);
  writeln('    cmp x25, #9');
  write('    b.eq L'); writeln(skip_ws_lbl);
  writeln('    cmp x25, #10');
  write('    b.eq L'); writeln(skip_ws_lbl);
  writeln('    cmp x25, #13');
  write('    b.eq L'); writeln(skip_ws_lbl);

  { Check for minus }
  neg_lbl := NewLabel;
  not_neg_lbl := NewLabel;
  writeln('    cmp x25, #45');
  write('    b.ne L'); writeln(not_neg_lbl);
  writeln('    mov x23, #1');
  EmitBranchLabel(neg_lbl);
  EmitLabel(not_neg_lbl);

  { First char is a digit - process it }
  writeln('    sub x25, x25, #48');
  writeln('    cmp x25, #9');
  write('    b.hi L'); writeln(done_lbl);
  { x21 = x21 * 10 + x25 }
  writeln('    mov x26, #10');
  writeln('    mul x21, x21, x26');
  writeln('    add x21, x21, x25');

  { Read integer part loop }
  EmitLabel(neg_lbl);
  read_int_lbl := NewLabel;
  read_frac_lbl := NewLabel;
  EmitLabel(read_int_lbl);
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  writeln('    cmp x0, #1');
  write('    b.lt L'); writeln(done_lbl);
  writeln('    ldrb w25, [sp]');

  { Check for '.' }
  writeln('    cmp x25, #46');
  write('    b.eq L'); writeln(read_frac_lbl);

  { Check if digit }
  writeln('    sub x25, x25, #48');
  writeln('    cmp x25, #9');
  write('    b.hi L'); writeln(done_lbl);
  { x21 = x21 * 10 + x25 }
  writeln('    mov x26, #10');
  writeln('    mul x21, x21, x26');
  writeln('    add x21, x21, x25');
  EmitBranchLabel(read_int_lbl);

  { Read fractional part }
  EmitLabel(read_frac_lbl);
  skip_neg_lbl := NewLabel;
  EmitLabel(skip_neg_lbl);
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  writeln('    cmp x0, #1');
  write('    b.lt L'); writeln(done_lbl);
  writeln('    ldrb w25, [sp]');

  { Check if digit }
  writeln('    sub x25, x25, #48');
  writeln('    cmp x25, #9');
  write('    b.hi L'); writeln(done_lbl);
  { x22 = x22 * 10 + x25, x24 = x24 * 10 }
  writeln('    mov x26, #10');
  writeln('    mul x22, x22, x26');
  writeln('    add x22, x22, x25');
  writeln('    mul x24, x24, x26');
  EmitBranchLabel(skip_neg_lbl);

  { Done - combine integer and fractional parts }
  EmitLabel(done_lbl);
  { d0 = x21 (integer part) }
  writeln('    scvtf d0, x21');
  { d1 = x22 (fractional part) }
  writeln('    scvtf d1, x22');
  { d2 = x24 (scale) }
  writeln('    scvtf d2, x24');
  { d1 = d1 / d2 }
  writeln('    fdiv d1, d1, d2');
  { d0 = d0 + d1 }
  EmitFAdd;

  { Apply negative if needed }
  skip_neg_lbl := NewLabel;
  write('    cbz x23, L'); writeln(skip_neg_lbl);
  EmitFNeg;
  EmitLabel(skip_neg_lbl);

  EmitAddSP(64);
  EmitLdp;
  EmitRet
end;

procedure EmitReadStringRuntime;
var
  loop_lbl, done_lbl, store_len_lbl: integer;
begin
  { Read string from input (x19), string buffer address passed in x0 }
  { String format: byte 0 = length, bytes 1-255 = characters }
  { Reads until newline or max 255 chars }
  EmitLabel(rt_read_string);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save string buffer address in x21 }
  writeln('    mov x21, x0');

  { x22 = character count (starts at 0) }
  writeln('    mov x22, #0');

  { Read loop }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Check if count >= 255 }
  writeln('    cmp x22, #255');
  write('    b.ge L'); writeln(done_lbl);

  { Read one character: read(x19, sp, 1) }
  writeln('    mov x0, x19');
  writeln('    mov x1, sp');
  writeln('    mov x2, #1');
  EmitMovX16(33554435);  { 0x2000003 = read syscall }
  EmitSvc;

  { Check if read failed or EOF (x0 < 1) }
  writeln('    cmp x0, #1');
  write('    b.lt L'); writeln(done_lbl);

  { Load character into x23 }
  writeln('    ldrb w23, [sp]');

  { Check if newline (10) or carriage return (13) }
  writeln('    cmp x23, #10');
  write('    b.eq L'); writeln(done_lbl);
  writeln('    cmp x23, #13');
  write('    b.eq L'); writeln(done_lbl);

  { Store character at buffer[count+1] }
  { x24 = x21 + x22 + 1 }
  writeln('    add x24, x21, x22');
  writeln('    add x24, x24, #1');
  writeln('    strb w23, [x24]');

  { Increment count }
  writeln('    add x22, x22, #1');

  { Loop back }
  write('    b L'); writeln(loop_lbl);

  { Done - store length at buffer[0] }
  EmitLabel(done_lbl);
  writeln('    strb w22, [x21]');

  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitFileOpenInit;
var
  skip_input_lbl, skip_output_lbl: integer;
begin
  { Initialize x19 (input fd) and x20 (output fd) from command line }
  { On entry: x0 = argc, x1 = argv }
  { Usage: tpcv2 input.pas output.s }
  skip_input_lbl := NewLabel;
  skip_output_lbl := NewLabel;

  { Save argc and argv to callee-saved registers }
  writeln('    mov x21, x0');
  writeln('    mov x22, x1');

  { Default: x19 = 0 (stdin), x20 = 1 (stdout), x18 = -1 (no pushback) }
  writeln('    mov x19, #0');
  writeln('    mov x20, #1');
  writeln('    mov x18, #-1');

  { If argc < 2, skip input file open }
  writeln('    cmp x21, #2');
  write('    b.lt L'); writeln(skip_input_lbl);

  { Load argv[1] into x0 (input filename) }
  writeln('    ldr x0, [x22, #8]');

  { Open input file: open(filename, O_RDONLY, 0) }
  writeln('    mov x1, #0');
  writeln('    mov x2, #0');
  EmitMovX16(33554437);  { 0x2000005 = open }
  EmitSvc;

  { Move input fd to x19 }
  writeln('    mov x19, x0');

  { If argc < 4, skip output file open (need: prog input.pas -o output.s) }
  writeln('    cmp x21, #4');
  write('    b.lt L'); writeln(skip_output_lbl);

  { Check if argv[2] == "-o": load argv[2], check first two bytes }
  writeln('    ldr x0, [x22, #16]');
  { Load first byte, check for '-' (45) }
  writeln('    ldrb w1, [x0]');
  writeln('    cmp w1, #45');
  write('    b.ne L'); writeln(skip_output_lbl);
  { Load second byte, check for 'o' (111) }
  writeln('    ldrb w1, [x0, #1]');
  writeln('    cmp w1, #111');
  write('    b.ne L'); writeln(skip_output_lbl);

  { Load argv[3] into x0 (output filename) }
  writeln('    ldr x0, [x22, #24]');

  { Open output file: open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644) }
  { O_WRONLY=1, O_CREAT=0x200, O_TRUNC=0x400 => 0x601 = 1537 }
  writeln('    mov x1, #1537');
  { mode 0644 = 420 decimal }
  writeln('    mov x2, #420');
  EmitMovX16(33554437);  { 0x2000005 = open }
  EmitSvc;

  { Move output fd to x20 }
  writeln('    mov x20, x0');

  EmitLabel(skip_output_lbl);
  EmitLabel(skip_input_lbl)
end;

procedure EmitHeapInitRuntime;
begin
  { Initialize heap using mmap syscall }
  { Allocates 1MB of memory for heap, stores base in x21 }
  { Initializes free list head in x22 pointing to entire block }
  EmitLabel(rt_heap_init);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  { mov x0, #0  - addr = NULL (let kernel choose) }
  writeln('    mov x0, #0');

  { mov x1, #1048576  - length = 1MB (0x100000) }
  { Need movz + movk since 1048576 > 65535 }
  writeln('    movz x1, #0');
  writeln('    movk x1, #16, lsl #16');

  { mov x2, #3  - PROT_READ | PROT_WRITE }
  writeln('    mov x2, #3');

  { mov x3, #4098  - MAP_PRIVATE | MAP_ANON (0x1002) }
  writeln('    mov x3, #4098');

  { mov x4, #-1  - fd = -1 }
  writeln('    movn x4, #0');

  { mov x5, #0  - offset = 0 }
  writeln('    mov x5, #0');

  { mmap syscall: 0x20000C5 }
  writeln('    movz x16, #0xC5');
  writeln('    movk x16, #0x200, lsl #16');

  EmitSvc;

  { mov x21, x0  - store heap base in x21 }
  writeln('    mov x21, x0');

  { Initialize free list: x22 = x21 (free list head points to heap base) }
  writeln('    mov x22, x21');

  { Store block size (1MB) at [x21]: first free block header }
  { movz x1, #0; movk x1, #16, lsl #16 = 1048576 }
  writeln('    movz x1, #0');
  writeln('    movk x1, #16, lsl #16');
  { str x1, [x21] - store size }
  writeln('    str x1, [x21]');

  { Store next=0 at [x21, #8]: no next free block }
  writeln('    str xzr, [x21, #8]');

  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitAllocRuntime;
var
  loop_lbl, found_lbl, split_lbl, no_split_lbl, update_head_lbl, done_lbl, oom_lbl: integer;
begin
  { Allocate memory from free list with block splitting }
  { Input: x0 = requested size }
  { Output: x0 = pointer to user data, or 0 if OOM }
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

  { Add 16 for header and align to 16 bytes: x0 = (x0 + 31) & ~15 }
  writeln('    add x0, x0, #31');
  { and x0, x0, #-16 }
  writeln('    and x0, x0, #-16');
  { Save required size to x3 }
  writeln('    mov x3, x0');

  { x1 = prev (0 initially), x2 = curr (x22 = free list head) }
  writeln('    mov x1, #0');
  writeln('    mov x2, x22');

  { Loop: walk free list looking for big enough block }
  EmitLabel(loop_lbl);
  { cbz x2, oom - end of list }
  write('    cbz x2, L'); writeln(oom_lbl);
  { ldr x4, [x2] - x4 = block size }
  writeln('    ldr x4, [x2]');
  { cmp x4, x3 - compare block size to required }
  writeln('    cmp x4, x3');
  { b.ge found }
  write('    b.ge L'); writeln(found_lbl);
  { Not big enough - move to next }
  { mov x1, x2 - prev = curr }
  writeln('    mov x1, x2');
  { ldr x2, [x2, #8] - curr = curr->next }
  writeln('    ldr x2, [x2, #8]');
  EmitBranchLabel(loop_lbl);

  { Found a big enough block - check if we can split it }
  EmitLabel(found_lbl);
  { x2 = block addr, x3 = required size, x4 = block size }
  { ldr x5, [x2, #8] - x5 = found->next }
  writeln('    ldr x5, [x2, #8]');
  { Check if block can be split: x4 - x3 >= 32 }
  { sub x6, x4, x3 - x6 = remainder size }
  writeln('    sub x6, x4, x3');
  { cmp x6, #32 }
  writeln('    cmp x6, #32');
  { b.lt no_split - if remainder < 32, don't split }
  write('    b.lt L'); writeln(no_split_lbl);

  { Split the block }
  EmitLabel(split_lbl);
  { x6 = remainder size, x7 = remainder block address = x2 + x3 }
  { add x7, x2, x3 }
  writeln('    add x7, x2, x3');
  { str x6, [x7] - remainder->size = x6 }
  writeln('    str x6, [x7]');
  { str x5, [x7, #8] - remainder->next = found->next }
  writeln('    str x5, [x7, #8]');
  { str x3, [x2] - found->size = required size }
  writeln('    str x3, [x2]');
  { mov x5, x7 - use remainder as new "next" for unlinking }
  writeln('    mov x5, x7');

  { Unlink found block from free list (and link remainder in its place) }
  EmitLabel(no_split_lbl);
  { cbz x1, update_head - if prev==0, update head }
  write('    cbz x1, L'); writeln(update_head_lbl);
  { str x5, [x1, #8] - prev->next = x5 (remainder or found->next) }
  writeln('    str x5, [x1, #8]');
  EmitBranchLabel(done_lbl);

  EmitLabel(update_head_lbl);
  { mov x22, x5 - free list head = x5 (remainder or found->next) }
  writeln('    mov x22, x5');

  EmitLabel(done_lbl);
  { Mark as allocated: str xzr, [x2, #8] - next = 0 }
  writeln('    str xzr, [x2, #8]');
  { Return user pointer: add x0, x2, #16 }
  writeln('    add x0, x2, #16');
  EmitAddSP(16);
  EmitLdp;
  EmitRet;

  { Out of memory }
  EmitLabel(oom_lbl);
  EmitMovX0(0);
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitFreeRuntime;
begin
  { Free memory back to free list }
  { Input: x0 = pointer to user data }
  EmitLabel(rt_free);
  { Get block header: sub x0, x0, #16 }
  writeln('    sub x0, x0, #16');
  { Link into free list: str x22, [x0, #8] - block->next = free_head }
  writeln('    str x22, [x0, #8]');
  { Update free list head: mov x22, x0 }
  writeln('    mov x22, x0');
  EmitRet
end;

procedure EmitStrCopyRuntime;
var
  loop_lbl, done_lbl: integer;
begin
  { Copy string from x1 (source) to x0 (dest) }
  { Pascal strings: byte 0 = length, bytes 1..length = chars }
  EmitLabel(rt_str_copy);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save dest and source addresses }
  { stur x0, [x29, #-8] - dest }
  writeln('    stur x0, [x29, #-8]');
  { stur x1, [x29, #-16] - source }
  writeln('    stur x1, [x29, #-16]');

  { Load length from source [x1] into x2 }
  writeln('    ldrb w2, [x1]');
  { Store length to dest [x0] }
  writeln('    strb w2, [x0]');
  { Save length to [x29, #-24] }
  writeln('    stur x2, [x29, #-24]');

  { Initialize index to 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  { Loop: copy bytes 1..length }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Load index }
  EmitLdurX0(-32);
  { Load length }
  writeln('    ldur x2, [x29, #-24]');
  { cmp x0, x2 - if index >= length, done }
  writeln('    cmp x0, x2');
  { b.ge done }
  write('    b.ge L'); writeln(done_lbl);

  { x3 = index + 1 (offset into string) }
  writeln('    add x3, x0, #1');

  { Load source char: ldrb w4, [x1, x3] where x1 = source }
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w4, [x1, x3]');

  { Store to dest char: strb w4, [x5, x3] where x5 = dest }
  writeln('    ldur x5, [x29, #-8]');
  writeln('    strb w4, [x5, x3]');

  { Increment index }
  EmitLdurX0(-32);
  writeln('    add x0, x0, #1');
  EmitSturX0(-32);
  EmitBranchLabel(loop_lbl);

  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitStrCompareRuntime;
var
  loop_lbl, not_equal_lbl, done_lbl: integer;
begin
  { Compare strings at x0 and x1, return 1 if equal, 0 if not equal in x0 }
  EmitLabel(rt_str_compare);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save addresses }
  EmitSturX0(-8);  { string 1 }
  { stur x1, [x29, #-16] }
  writeln('    stur x1, [x29, #-16]');

  { Load lengths }
  writeln('    ldrb w2, [x0]');
  writeln('    ldrb w3, [x1]');

  { Compare lengths first }
  not_equal_lbl := NewLabel;
  done_lbl := NewLabel;
  writeln('    cmp x2, x3');
  { b.ne not_equal }
  write('    b.ne L'); writeln(not_equal_lbl);

  { Save length to [x29, #-24] }
  writeln('    stur x2, [x29, #-24]');

  { Initialize index to 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  { Loop: compare each byte }
  loop_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Load index }
  EmitLdurX0(-32);
  { Load length }
  writeln('    ldur x2, [x29, #-24]');
  { cmp x0, x2 - if index >= length, all equal }
  writeln('    cmp x0, x2');
  { b.ge equal (return 1) }
  write('    b.ge L'); writeln(done_lbl);

  { x3 = index + 1 }
  writeln('    add x3, x0, #1');

  { Load chars from both strings }
  EmitLdurX0(-8);  { string 1 addr }
  writeln('    ldrb w4, [x0, x3]');

  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w5, [x1, x3]');

  { Compare chars }
  writeln('    cmp x4, x5');
  { b.ne not_equal }
  write('    b.ne L'); writeln(not_equal_lbl);

  { Increment index }
  writeln('    ldur x0, [x29, #-32]');
  writeln('    add x0, x0, #1');
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
end;

procedure EmitStrConcatRuntime;
var
  loop1_lbl, loop2_lbl, done_lbl: integer;
begin
  { Concatenate strings: x0 = dest, x1 = string1, x2 = string2 }
  { Result length = len1 + len2 (capped at 255) }
  EmitLabel(rt_str_concat);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save addresses }
  EmitSturX0(-8);   { dest }
  { stur x1, [x29, #-16] - string 1 }
  writeln('    stur x1, [x29, #-16]');
  { stur x2, [x29, #-24] - string 2 }
  writeln('    stur x2, [x29, #-24]');

  { Load lengths }
  writeln('    ldrb w3, [x1]');
  writeln('    ldrb w4, [x2]');

  { Calculate total length (capped at 255) }
  writeln('    add x5, x3, x4');
  { cmp x5, #255 }
  writeln('    cmp x5, #255');
  { csel x5, x5, x6, le - if x5 <= 255, keep it; else use 255 }
  writeln('    mov x6, #255');
  writeln('    csel x5, x5, x6, le');

  { Store total length to dest }
  EmitLdurX0(-8);  { dest }
  writeln('    strb w5, [x0]');

  { Save len1 to [x29, #-32] }
  writeln('    stur x3, [x29, #-32]');

  { Initialize index to 0 }
  EmitMovX0(0);
  EmitSturX0(-40);

  { Loop 1: copy string1 chars }
  loop1_lbl := NewLabel;
  loop2_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop1_lbl);

  { Load index and len1 }
  EmitLdurX0(-40);
  writeln('    ldur x3, [x29, #-32]');
  { cmp x0, x3 }
  writeln('    cmp x0, x3');
  { b.ge loop2 }
  write('    b.ge L'); writeln(loop2_lbl);

  { x7 = index + 1 }
  writeln('    add x7, x0, #1');

  { Load source1 char }
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w8, [x1, x7]');

  { Store to dest }
  EmitLdurX0(-8);
  writeln('    strb w8, [x0, x7]');

  { Increment index }
  EmitLdurX0(-40);
  writeln('    add x0, x0, #1');
  EmitSturX0(-40);
  EmitBranchLabel(loop1_lbl);

  { Loop 2: copy string2 chars }
  EmitLabel(loop2_lbl);
  { Reset index to 0 for string2 }
  EmitMovX0(0);
  EmitSturX0(-40);

  { Load len2 into x4 }
  writeln('    ldur x2, [x29, #-24]');
  writeln('    ldrb w4, [x2]');
  { Save len2 to [x29, #-48] }
  writeln('    stur x4, [x29, #-48]');

  { Loop 2 body label }
  EmitLabel(label_count);
  label_count := label_count + 1;

  { Load index and len2 }
  EmitLdurX0(-40);
  writeln('    ldur x4, [x29, #-48]');
  { cmp x0, x4 }
  writeln('    cmp x0, x4');
  { b.ge done }
  write('    b.ge L'); writeln(done_lbl);

  { x7 = index + 1 (source offset) }
  writeln('    add x7, x0, #1');

  { Load source2 char }
  writeln('    ldur x2, [x29, #-24]');
  writeln('    ldrb w8, [x2, x7]');

  { Calculate dest offset: len1 + index + 1 }
  EmitLdurX0(-40);  { index }
  writeln('    ldur x3, [x29, #-32]');
  writeln('    add x7, x0, x3');
  writeln('    add x7, x7, #1');

  { Store to dest }
  EmitLdurX0(-8);
  writeln('    strb w8, [x0, x7]');

  { Increment index }
  EmitLdurX0(-40);
  writeln('    add x0, x0, #1');
  EmitSturX0(-40);
  EmitBranchLabel(label_count - 1);

  EmitLabel(done_lbl);
  { Return dest address in x0 }
  EmitLdurX0(-8);
  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitStrCmpRuntime;
{ Lexicographic string comparison - returns -1 if x0<x1, 0 if x0=x1, 1 if x0>x1 }
{ Input: x0 = string1 addr, x1 = string2 addr }
{ Output: x0 = -1/0/1 }
var
  loop_lbl, done_lbl, less_lbl, greater_lbl, check_len_lbl: integer;
begin
  EmitLabel(rt_str_cmp);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);
  { stur x0, [x29, #-8]  - string1 }
  EmitSturX0(-8);
  { stur x1, [x29, #-16] - string2 }
  writeln('    stur x1, [x29, #-16]');

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
  writeln('    ldrb w2, [x0]');

  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w3, [x1]');

  { Load index }
  EmitLdurX0(-24);   { x0 = index }

  { if index >= min(len1, len2), check length }
  writeln('    cmp x0, x2');
  write('    b.ge L'); writeln(check_len_lbl);
  writeln('    cmp x0, x3');
  write('    b.ge L'); writeln(check_len_lbl);

  { Compare chars at index+1 }
  { x4 = index + 1 }
  writeln('    add x4, x0, #1');

  { Load char from s1 }
  EmitLdurX0(-8);
  writeln('    ldrb w5, [x0, x4]');

  { Load char from s2 }
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w6, [x1, x4]');

  { Compare chars }
  writeln('    cmp x5, x6');
  write('    b.lt L'); writeln(less_lbl);
  write('    b.gt L'); writeln(greater_lbl);

  { Chars equal, increment index }
  EmitLdurX0(-24);
  writeln('    add x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(loop_lbl);

  { Check lengths when all compared chars are equal }
  EmitLabel(check_len_lbl);
  EmitLdurX0(-8);
  writeln('    ldrb w2, [x0]');
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w3, [x1]');
  writeln('    cmp x2, x3');
  write('    b.lt L'); writeln(less_lbl);
  write('    b.gt L'); writeln(greater_lbl);
  { Equal }
  EmitMovX0(0);
  EmitBranchLabel(done_lbl);

  EmitLabel(less_lbl);
  EmitMovX0(0);
  writeln('    sub x0, x0, #1');
  EmitBranchLabel(done_lbl);

  EmitLabel(greater_lbl);
  EmitMovX0(1);

  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitStrPosRuntime;
{ Find substring position - returns position (1-based) or 0 if not found }
{ Input: x0 = substring addr, x1 = string addr }
{ Output: x0 = position (1-based) or 0 }
var
  outer_lbl, inner_lbl, done_lbl, found_lbl, not_found_lbl, next_pos_lbl: integer;
begin
  EmitLabel(rt_str_pos);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { [x29-8] = substr, [x29-16] = string, [x29-24] = outer idx, [x29-32] = inner idx }
  EmitSturX0(-8);
  writeln('    stur x1, [x29, #-16]');

  outer_lbl := NewLabel;
  inner_lbl := NewLabel;
  done_lbl := NewLabel;
  found_lbl := NewLabel;
  not_found_lbl := NewLabel;
  next_pos_lbl := NewLabel;

  { If substr is empty, return 1 }
  EmitLdurX0(-8);
  writeln('    ldrb w2, [x0]');
  write('    cbz x2, L'); writeln(found_lbl);
  { x2 = substr len }
  writeln('    stur x2, [x29, #-40]');

  { Load string length }
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w3, [x1]');
  { x3 = string len }

  { outer_idx = 0 (will be 1-based position - 1) }
  EmitMovX0(0);
  EmitSturX0(-24);

  EmitLabel(outer_lbl);
  { Check if outer_idx + substr_len > string_len }
  EmitLdurX0(-24);  { outer_idx }
  writeln('    ldur x2, [x29, #-40]');
  writeln('    add x4, x0, x2');
  { x4 = outer_idx + substr_len }
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w3, [x1]');
  writeln('    cmp x4, x3');
  write('    b.gt L'); writeln(not_found_lbl);

  { inner_idx = 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  EmitLabel(inner_lbl);
  { if inner_idx >= substr_len, found! }
  EmitLdurX0(-32);
  writeln('    ldur x2, [x29, #-40]');
  writeln('    cmp x0, x2');
  write('    b.ge L'); writeln(found_lbl);

  { Compare substr[inner_idx+1] with string[outer_idx+inner_idx+1] }
  EmitLdurX0(-8);  { substr addr }
  writeln('    ldur x4, [x29, #-32]');
  writeln('    add x4, x4, #1');
  writeln('    ldrb w5, [x0, x4]');
  { x5 = substr[inner_idx+1] }

  writeln('    ldur x1, [x29, #-16]');
  EmitLdurX0(-24);  { outer_idx }
  writeln('    ldur x4, [x29, #-32]');
  writeln('    add x4, x0, x4');
  writeln('    add x4, x4, #1');
  writeln('    ldrb w6, [x1, x4]');
  { x6 = string[outer_idx+inner_idx+1] }

  writeln('    cmp x5, x6');
  write('    b.ne L'); writeln(next_pos_lbl);

  { Chars match, increment inner_idx }
  EmitLdurX0(-32);
  writeln('    add x0, x0, #1');
  EmitSturX0(-32);
  EmitBranchLabel(inner_lbl);

  EmitLabel(next_pos_lbl);
  { Chars don't match, try next outer position }
  EmitLdurX0(-24);
  writeln('    add x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(outer_lbl);

  EmitLabel(found_lbl);
  { Return outer_idx + 1 (1-based position) }
  EmitLdurX0(-24);
  writeln('    add x0, x0, #1');
  EmitBranchLabel(done_lbl);

  EmitLabel(not_found_lbl);
  EmitMovX0(0);

  EmitLabel(done_lbl);
  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitStrDeleteRuntime;
{ Delete chars from string in place }
{ Input: x0 = string addr, x1 = start (1-based), x2 = count }
var
  loop_lbl, done_lbl: integer;
begin
  EmitLabel(rt_str_delete);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { [x29-8]=string, [x29-16]=start, [x29-24]=count, [x29-32]=len, [x29-40]=idx }
  EmitSturX0(-8);
  writeln('    stur x1, [x29, #-16]');
  writeln('    stur x2, [x29, #-24]');

  loop_lbl := NewLabel;
  done_lbl := NewLabel;

  { Load string length }
  writeln('    ldrb w3, [x0]');
  writeln('    stur x3, [x29, #-32]');
  { x3 = len }

  { idx = start }
  writeln('    ldur x0, [x29, #-16]');
  EmitSturX0(-40);

  EmitLabel(loop_lbl);
  { if idx + count > len, done }
  EmitLdurX0(-40);  { idx }
  writeln('    ldur x2, [x29, #-24]');
  writeln('    add x4, x0, x2');
  writeln('    ldur x3, [x29, #-32]');
  writeln('    cmp x4, x3');
  write('    b.gt L'); writeln(done_lbl);

  { Copy char from idx+count to idx }
  EmitLdurX0(-8);  { string addr }
  EmitLdurX0(-40);  { idx }
  { RELOAD string addr since we overwrote x0 }
  writeln('    ldur x1, [x29, #-8]');
  writeln('    ldur x2, [x29, #-24]');
  writeln('    add x3, x0, x2');
  { x3 = idx + count (source position) }
  { Load char from string[idx+count] }
  writeln('    ldrb w4, [x1, x3]');
  { Store to string[idx] }
  writeln('    strb w4, [x1, x0]');

  { idx++ }
  EmitLdurX0(-40);
  writeln('    add x0, x0, #1');
  EmitSturX0(-40);
  EmitBranchLabel(loop_lbl);

  EmitLabel(done_lbl);
  { Update string length: new_len = old_len - count }
  writeln('    ldur x3, [x29, #-32]');
  writeln('    ldur x2, [x29, #-24]');
  writeln('    sub x0, x3, x2');
  EmitLdurX0(-8);
  { RELOAD x0 for store since we overwrote it }
  writeln('    ldur x3, [x29, #-32]');
  writeln('    ldur x2, [x29, #-24]');
  writeln('    sub x4, x3, x2');
  writeln('    ldur x1, [x29, #-8]');
  writeln('    strb w4, [x1]');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitStrInsertRuntime;
{ Insert string into another at position }
{ Input: x0 = source string, x1 = dest string, x2 = position (1-based) }
var
  loop1_lbl, loop2_lbl, copy_lbl, done_lbl: integer;
begin
  EmitLabel(rt_str_insert);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);
  { [x29-8]=source, [x29-16]=dest, [x29-24]=pos, [x29-32]=src_len, [x29-40]=dst_len, [x29-48]=idx }
  EmitSturX0(-8);
  writeln('    stur x1, [x29, #-16]');
  writeln('    stur x2, [x29, #-24]');

  loop1_lbl := NewLabel;
  loop2_lbl := NewLabel;
  copy_lbl := NewLabel;
  done_lbl := NewLabel;

  { Load lengths }
  writeln('    ldrb w3, [x0]');
  writeln('    stur x3, [x29, #-32]');
  writeln('    ldrb w4, [x1]');
  writeln('    stur x4, [x29, #-40]');

  { Move existing chars from pos to end, shifting right by src_len }
  { idx = dst_len }
  writeln('    stur x4, [x29, #-48]');

  EmitLabel(loop1_lbl);
  { if idx < pos, done moving }
  EmitLdurX0(-48);  { idx }
  writeln('    ldur x2, [x29, #-24]');
  writeln('    cmp x0, x2');
  write('    b.lt L'); writeln(loop2_lbl);

  { Copy dest[idx] to dest[idx+src_len] }
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldrb w5, [x1, x0]');
  writeln('    ldur x3, [x29, #-32]');
  writeln('    add x6, x0, x3');
  writeln('    strb w5, [x1, x6]');

  { idx-- }
  EmitLdurX0(-48);
  writeln('    sub x0, x0, #1');
  EmitSturX0(-48);
  EmitBranchLabel(loop1_lbl);

  EmitLabel(loop2_lbl);
  { Now copy source chars to position pos...pos+src_len-1 }
  { idx = 0 }
  EmitMovX0(0);
  EmitSturX0(-48);

  EmitLabel(copy_lbl);
  { if idx >= src_len, done }
  EmitLdurX0(-48);
  writeln('    ldur x3, [x29, #-32]');
  writeln('    cmp x0, x3');
  write('    b.ge L'); writeln(done_lbl);

  { Copy source[idx+1] to dest[pos+idx] }
  writeln('    ldur x5, [x29, #-8]');
  writeln('    add x6, x0, #1');
  writeln('    ldrb w7, [x5, x6]');
  writeln('    ldur x1, [x29, #-16]');
  writeln('    ldur x2, [x29, #-24]');
  writeln('    add x6, x2, x0');
  writeln('    strb w7, [x1, x6]');

  { idx++ }
  EmitLdurX0(-48);
  writeln('    add x0, x0, #1');
  EmitSturX0(-48);
  EmitBranchLabel(copy_lbl);

  EmitLabel(done_lbl);
  { Update dest length: new_len = dst_len + src_len }
  writeln('    ldur x3, [x29, #-32]');
  writeln('    ldur x4, [x29, #-40]');
  writeln('    add x0, x3, x4');
  writeln('    ldur x1, [x29, #-16]');
  writeln('    strb w0, [x1]');

  EmitAddSP(64);
  EmitLdp;
  EmitRet
end;

{ EmitIntToStrRuntime - Convert integer to string }
{ x0 = integer value, x1 = destination string address }
procedure EmitIntToStrRuntime;
var
  done_lbl, neg_lbl, pos_lbl, loop_lbl, rev_lbl, rev_done_lbl: integer;
begin
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

  { Save dest string address }
  writeln('    stur x1, [x29, #-16]');

  { x2 = digit count, x3 = is_negative }
  writeln('    mov x2, #0');
  writeln('    mov x3, #0');

  { Check if value is negative }
  writeln('    cmp x0, #0');
  write('    b.lt L'); writeln(neg_lbl);
  write('    b L'); writeln(pos_lbl);

  { Handle negative }
  EmitLabel(neg_lbl);
  writeln('    neg x0, x0');
  writeln('    mov x3, #1');

  { Loop to extract digits (stored in reverse on stack) }
  EmitLabel(pos_lbl);
  EmitLabel(loop_lbl);
  { x4 = x0 / 10, x5 = x0 mod 10 }
  writeln('    mov x6, #10');
  writeln('    udiv x4, x0, x6');
  writeln('    msub x5, x4, x6, x0');

  { Convert digit to ASCII and store on stack }
  writeln('    add x5, x5, #48');

  { Store digit at sp + x2 }
  writeln('    strb w5, [sp, x2]');

  { Increment digit count }
  writeln('    add x2, x2, #1');

  { x0 = x4 (quotient), continue if x0 > 0 }
  writeln('    mov x0, x4');
  writeln('    cmp x0, #0');
  write('    b.ne L'); writeln(loop_lbl);

  { Now reverse digits into destination string }
  { x1 = dest string, x4 = write position (starts at 1 if no sign, 2 if negative) }
  writeln('    ldur x1, [x29, #-16]');

  { If negative, store '-' at position 1, start digits at 2 }
  writeln('    cmp x3, #0');
  write('    b.eq L'); writeln(rev_lbl);

  { Store '-' sign }
  writeln('    mov w5, #45');
  writeln('    strb w5, [x1, #1]');

  { x4 = write position (1 for positive, 2 for negative start) }
  EmitLabel(rev_lbl);
  writeln('    add x4, x3, #1');

  { x5 = read position (digit count - 1, reading backwards) }
  writeln('    sub x5, x2, #1');

  { Reverse copy loop }
  EmitLabel(rev_done_lbl);
  writeln('    cmp x5, #0');
  write('    b.lt L'); writeln(done_lbl);

  { Load digit from stack at position x5 }
  writeln('    ldrb w6, [sp, x5]');

  { Store at dest position x4 }
  writeln('    strb w6, [x1, x4]');

  writeln('    add x4, x4, #1');
  writeln('    sub x5, x5, #1');
  write('    b L'); writeln(rev_done_lbl);

  { Store length (x2 + x3 = digit count + sign) }
  EmitLabel(done_lbl);
  writeln('    add x0, x2, x3');
  writeln('    strb w0, [x1]');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

{ EmitStrToIntRuntime - Convert string to integer }
{ x0 = source string address }
{ Returns: x0 = integer value, x1 = error code (0 = success, position of error otherwise) }
procedure EmitStrToIntRuntime;
var
  done_lbl, error_lbl, loop_lbl, neg_lbl, pos_lbl, skip_sign_lbl, exit_lbl: integer;
begin
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

  { x1 = length, x2 = index (starts at 1), x3 = result, x4 = is_negative }
  writeln('    ldrb w1, [x0]');
  writeln('    mov x2, #1');
  writeln('    mov x3, #0');
  writeln('    mov x4, #0');

  { Check if empty string }
  writeln('    cmp x1, #0');
  write('    b.eq L'); writeln(error_lbl);

  { Check first char for sign }
  writeln('    ldrb w5, [x0, #1]');

  { Check for '-' (45) }
  writeln('    cmp x5, #45');
  write('    b.eq L'); writeln(neg_lbl);

  { Check for '+' (43) }
  writeln('    cmp x5, #43');
  write('    b.eq L'); writeln(pos_lbl);
  write('    b L'); writeln(loop_lbl);

  EmitLabel(neg_lbl);
  writeln('    mov x4, #1');
  EmitLabel(pos_lbl);
  writeln('    add x2, x2, #1');

  { Main loop }
  EmitLabel(loop_lbl);
  writeln('    cmp x2, x1');
  write('    b.gt L'); writeln(done_lbl);

  { Load char at position x2 }
  writeln('    ldrb w5, [x0, x2]');

  { Check if digit (48-57) }
  writeln('    cmp x5, #48');
  write('    b.lt L'); writeln(error_lbl);
  writeln('    cmp x5, #57');
  write('    b.gt L'); writeln(error_lbl);

  { result = result * 10 + digit }
  writeln('    mov x6, #10');
  writeln('    mul x3, x3, x6');
  writeln('    sub x5, x5, #48');
  writeln('    add x3, x3, x5');

  { x2 := x2 + 1 }
  writeln('    add x2, x2, #1');
  write('    b L'); writeln(loop_lbl);

  { Success }
  EmitLabel(done_lbl);
  { Apply sign if negative }
  writeln('    cmp x4, #0');
  write('    b.eq L'); writeln(skip_sign_lbl);
  writeln('    neg x3, x3');

  EmitLabel(skip_sign_lbl);
  writeln('    mov x0, x3');
  writeln('    mov x1, #0');
  write('    b L'); writeln(exit_lbl);

  { Error - return position in x1 }
  EmitLabel(error_lbl);
  writeln('    mov x0, #0');
  writeln('    mov x1, x2');

  EmitLabel(exit_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

{ EmitStrLtrimRuntime - Remove leading whitespace from string }
{ Input: x0 = source string addr }
{ Output: x0 = new trimmed string addr (allocated from heap) }
procedure EmitStrLtrimRuntime;
var
  loop_lbl, done_lbl, copy_lbl, copy_done_lbl: integer;
begin
  EmitLabel(rt_str_ltrim);
  EmitStp;
  EmitSubSP(32);

  { x1 = source addr, x2 = source length, x3 = skip count }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  copy_lbl := NewLabel;
  copy_done_lbl := NewLabel;

  { Save source addr }
  writeln('    mov x1, x0');

  { x2 = source length }
  writeln('    ldrb w2, [x1]');

  { x3 = 0 (skip count) }
  writeln('    mov x3, #0');

  { Loop: skip leading whitespace }
  EmitLabel(loop_lbl);
  { if x3 >= x2, done }
  writeln('    cmp x3, x2');
  write('    b.ge L'); writeln(done_lbl);

  { x4 = char at [x1 + x3 + 1] }
  writeln('    add x4, x3, #1');
  writeln('    ldrb w4, [x1, x4]');

  { if char = 32 (space), skip }
  writeln('    cmp x4, #32');
  write('    b.eq L'); writeln(label_count);

  { if char = 9 (tab), skip }
  writeln('    cmp x4, #9');
  write('    b.ne L'); writeln(done_lbl);

  EmitLabel(label_count);
  label_count := label_count + 1;

  { x3 := x3 + 1 }
  writeln('    add x3, x3, #1');
  write('    b L'); writeln(loop_lbl);

  { Done scanning: x3 = number to skip, x2 = length }
  EmitLabel(done_lbl);

  { Allocate new string from heap: x0 = x21 }
  writeln('    mov x0, x21');
  writeln('    add x21, x21, #256');

  { x5 = new length = x2 - x3 }
  writeln('    sub x5, x2, x3');

  { Store new length }
  writeln('    strb w5, [x0]');

  { x6 = copy index (0 to x5-1) }
  writeln('    mov x6, #0');

  { Copy loop }
  EmitLabel(copy_lbl);
  writeln('    cmp x6, x5');
  write('    b.ge L'); writeln(copy_done_lbl);

  { x7 = x3 + x6 + 1 (source index) }
  writeln('    add x7, x3, x6');
  writeln('    add x7, x7, #1');

  { Load char }
  writeln('    ldrb w8, [x1, x7]');

  { x7 = x6 + 1 (dest index) }
  writeln('    add x7, x6, #1');

  { Store char }
  writeln('    strb w8, [x0, x7]');

  { x6 := x6 + 1 }
  writeln('    add x6, x6, #1');
  write('    b L'); writeln(copy_lbl);

  EmitLabel(copy_done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

{ EmitStrRtrimRuntime - Remove trailing whitespace from string }
{ Input: x0 = source string addr }
{ Output: x0 = new trimmed string addr (allocated from heap) }
procedure EmitStrRtrimRuntime;
var
  loop_lbl, done_lbl, copy_lbl, copy_done_lbl: integer;
begin
  EmitLabel(rt_str_rtrim);
  EmitStp;
  EmitSubSP(32);

  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  copy_lbl := NewLabel;
  copy_done_lbl := NewLabel;

  { x1 = source addr }
  writeln('    mov x1, x0');

  { x2 = source length }
  writeln('    ldrb w2, [x1]');

  { x3 = x2 (scan from end) }
  writeln('    mov x3, x2');

  { Loop: find last non-whitespace }
  EmitLabel(loop_lbl);
  { if x3 <= 0, done }
  writeln('    cmp x3, #0');
  write('    b.le L'); writeln(done_lbl);

  { x4 = char at [x1 + x3] }
  writeln('    ldrb w4, [x1, x3]');

  { if char = 32 (space), continue }
  writeln('    cmp x4, #32');
  write('    b.eq L'); writeln(label_count);

  { if char = 9 (tab), continue }
  writeln('    cmp x4, #9');
  write('    b.ne L'); writeln(done_lbl);

  EmitLabel(label_count);
  label_count := label_count + 1;

  { x3 := x3 - 1 }
  writeln('    sub x3, x3, #1');
  write('    b L'); writeln(loop_lbl);

  { Done: x3 = new length }
  EmitLabel(done_lbl);

  { Allocate new string from heap }
  writeln('    mov x0, x21');
  writeln('    add x21, x21, #256');

  { Store new length }
  writeln('    strb w3, [x0]');

  { x5 = copy index (0 to x3-1) }
  writeln('    mov x5, #0');

  { Copy loop }
  EmitLabel(copy_lbl);
  writeln('    cmp x5, x3');
  write('    b.ge L'); writeln(copy_done_lbl);

  { x6 = x5 + 1 (index) }
  writeln('    add x6, x5, #1');

  { Load char from source }
  writeln('    ldrb w7, [x1, x6]');

  { Store char to dest }
  writeln('    strb w7, [x0, x6]');

  { x5 := x5 + 1 }
  writeln('    add x5, x5, #1');
  write('    b L'); writeln(copy_lbl);

  EmitLabel(copy_done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

{ EmitStrTrimRuntime - Remove both leading and trailing whitespace }
{ Input: x0 = source string addr }
{ Output: x0 = new trimmed string addr }
procedure EmitStrTrimRuntime;
begin
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
end;

{ ----- Screen/Terminal Control Routines ----- }

procedure EmitStrbAtOffset(offset: integer);
{ Emit: sturb w0, [x29, #offset] - stores low byte of w0 at x29+offset }
begin
  write('    sturb w0, [x29, #'); write(offset); writeln(']');
end;

procedure EmitClrScrRuntime;
{ Emits code to clear screen and home cursor: ESC[2J ESC[H }
begin
  EmitLabel(rt_clrscr);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Store escape sequence on stack: ESC[2J ESC[H = 27,91,50,74,27,91,72 }
  { Use sturb to store individual bytes }
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #15 }
  writeln('    sub x1, x29, #15');
  { mov x2, #7 }
  writeln('    mov x2, #7');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitGotoXYRuntime;
{ Emits code to move cursor: ESC[y;xH }
{ x1=x (column), x0=y (row) }
var
  loop1, done1, loop2, done2, end_lbl: integer;
begin
  EmitLabel(rt_gotoxy);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);
  { Save x and y to registers first to avoid memory corruption }
  { x11 = y (row), x12 = x (column) - using higher regs to avoid conflicts with division }
  writeln('    mov x11, x0');
  writeln('    mov x12, x1');
  { Build escape sequence at stack offset -20 onwards (leave room for digits) }
  { Format: ESC [ y ; x H }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-20);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-19);
  { x8 = pointer to next byte (start at -18) }
  writeln('    sub x8, x29, #18');
  { Convert y to decimal digits (y is in x11) }
  { mov x0, x11 }
  writeln('    mov x0, x11');
  { If y >= 10, output tens digit }
  loop1 := NewLabel;
  done1 := NewLabel;
  writeln('    cmp x0, #10');
  write('    b.lt L'); writeln(done1);
  { Divide by 10: x1 = x0 / 10, x2 = x0 mod 10 }
  writeln('    mov x9, #10');
  writeln('    udiv x1, x0, x9');
  writeln('    msub x0, x1, x9, x0');
  { x1=tens, x0=units - store tens digit }
  writeln('    add x1, x1, #48');
  { strb w1, [x8], #1 }
  writeln('    strb w1, [x8], #1');
  EmitLabel(done1);
  { Store units digit }
  writeln('    add x0, x0, #48');
  { strb w0, [x8], #1 }
  writeln('    strb w0, [x8], #1');
  { Store semicolon }
  EmitMovX0(59);  { ; }
  { strb w0, [x8], #1 }
  writeln('    strb w0, [x8], #1');
  { Convert x to decimal digits (x is in x12) }
  { mov x0, x12 }
  writeln('    mov x0, x12');
  done2 := NewLabel;
  writeln('    cmp x0, #10');
  write('    b.lt L'); writeln(done2);
  { Divide by 10 }
  writeln('    mov x9, #10');
  writeln('    udiv x1, x0, x9');
  writeln('    msub x0, x1, x9, x0');
  { Store tens digit }
  writeln('    add x1, x1, #48');
  { strb w1, [x8], #1 }
  writeln('    strb w1, [x8], #1');
  EmitLabel(done2);
  { Store units digit }
  writeln('    add x0, x0, #48');
  { strb w0, [x8], #1 }
  writeln('    strb w0, [x8], #1');
  { Store H terminator }
  EmitMovX0(72);  { H }
  { strb w0, [x8] }
  writeln('    strb w0, [x8]');
  { Calculate length: x8 points past last byte, buffer starts at x29-20 }
  { add x8, x8, #1 (point past H) }
  writeln('    add x8, x8, #1');
  { x2 = x8 - (x29-20) = x8 - x29 + 20 }
  { sub x2, x8, x29 }
  writeln('    sub x2, x8, x29');
  { add x2, x2, #20 }
  writeln('    add x2, x2, #20');
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #20 }
  writeln('    sub x1, x29, #20');
  EmitSvc;
  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitClrEolRuntime;
{ Emits code to clear to end of line: ESC[K }
begin
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #11 }
  writeln('    sub x1, x29, #11');
  { mov x2, #3 }
  writeln('    mov x2, #3');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitTextColorRuntime;
{ Emits code to set foreground color: ESC[3Xm where X=x0 (0-7) }
begin
  EmitLabel(rt_textcolor);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Save color parameter (x0) to x9 }
  writeln('    mov x9, x0');
  { Store ESC[3Xm = 27,91,51,X,109 (5 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-13);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-12);
  EmitMovX0(51);  { 3 }
  EmitStrbAtOffset(-11);
  { Store color digit: x9 + 48 }
  writeln('    add x0, x9, #48');
  EmitStrbAtOffset(-10);
  { Store 'm' }
  EmitMovX0(109);  { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #13 }
  writeln('    sub x1, x29, #13');
  { mov x2, #5 }
  writeln('    mov x2, #5');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitTextBackgroundRuntime;
{ Emits code to set background color: ESC[4Xm where X=x0 (0-7) }
begin
  EmitLabel(rt_textbackground);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);
  { Save color parameter (x0) to x9 }
  writeln('    mov x9, x0');
  { Store ESC[4Xm = 27,91,52,X,109 (5 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-13);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-12);
  EmitMovX0(52);  { 4 }
  EmitStrbAtOffset(-11);
  { Store color digit: x9 + 48 }
  writeln('    add x0, x9, #48');
  EmitStrbAtOffset(-10);
  { Store 'm' }
  EmitMovX0(109);  { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #13 }
  writeln('    sub x1, x29, #13');
  { mov x2, #5 }
  writeln('    mov x2, #5');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitNormVideoRuntime;
{ Emits code to reset attributes: ESC[0m }
begin
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #12 }
  writeln('    sub x1, x29, #12');
  { mov x2, #4 }
  writeln('    mov x2, #4');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitHighVideoRuntime;
{ Emits code to enable bold: ESC[1m }
begin
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #12 }
  writeln('    sub x1, x29, #12');
  { mov x2, #4 }
  writeln('    mov x2, #4');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitLowVideoRuntime;
{ Emits code to enable dim: ESC[2m }
begin
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #12 }
  writeln('    sub x1, x29, #12');
  { mov x2, #4 }
  writeln('    mov x2, #4');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitHideCursorRuntime;
{ Emits code to hide cursor: ESC[?25l }
begin
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #14 }
  writeln('    sub x1, x29, #14');
  { mov x2, #6 }
  writeln('    mov x2, #6');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitShowCursorRuntime;
{ Emits code to show cursor: ESC[?25h }
begin
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
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #14 }
  writeln('    sub x1, x29, #14');
  { mov x2, #6 }
  writeln('    mov x2, #6');
  EmitSvc;
  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitSleepRuntime;
{ Emits code to sleep for x0 milliseconds using select syscall }
{ x0 = milliseconds }
{ select(0, NULL, NULL, NULL, &timeval) with nfds=0, all fd_sets=NULL }
{ struct timeval: tv_sec (8 bytes), tv_usec (8 bytes) }
begin
  EmitLabel(rt_sleep);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);  { 16 bytes for timeval + alignment }

  { x10 = ms (save original) }
  writeln('    mov x10, x0');

  { x11 = 1000 }
  writeln('    mov x11, #1000');

  { x0 = ms / 1000 (seconds) }
  writeln('    udiv x0, x10, x11');

  { x1 = ms mod 1000 (remainder): msub x1, x0, x11, x10 = x10 - (x0 * x11) }
  writeln('    msub x1, x0, x11, x10');

  { x1 = x1 * 1000 (microseconds, not nanoseconds for timeval) }
  writeln('    mul x1, x1, x11');

  { Store timeval at sp: [sp] = tv_sec, [sp+8] = tv_usec }
  writeln('    str x0, [sp]');

  writeln('    str x1, [sp, #8]');

  { select(nfds=0, readfds=NULL, writefds=NULL, exceptfds=NULL, timeout=sp) }
  { x0 = 0 (nfds) }
  writeln('    mov x0, #0');

  { x1 = NULL }
  writeln('    mov x1, #0');

  { x2 = NULL }
  writeln('    mov x2, #0');

  { x3 = NULL }
  writeln('    mov x3, #0');

  { x4 = pointer to timeval (sp) }
  writeln('    mov x4, sp');

  { x16 = syscall number for select: 0x2000000 + 93 = 33554525 }
  EmitMovX16(33554525);
  EmitSvc;

  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitKeyPressedRuntime;
{ Check if a key is available on stdin using select with 0 timeout }
{ Returns 1 in x0 if key available, 0 otherwise }
begin
  EmitLabel(rt_keypressed);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);  { 8 bytes for fd_set + 16 bytes for timeval + padding }

  { Clear the fd_set at sp }
  writeln('    mov x0, #0');
  writeln('    str x0, [sp]');

  { Set bit 0 in fd_set for stdin (fd 0) }
  writeln('    mov x0, #1');
  writeln('    str x0, [sp]');

  { Set timeval to 0,0 (no wait) at sp+16 }
  writeln('    mov x0, #0');
  writeln('    str x0, [sp, #16]');
  writeln('    str x0, [sp, #24]');

  { select(nfds=1, readfds=sp, writefds=NULL, exceptfds=NULL, timeout=sp+16) }
  { x0 = 1 (nfds) }
  writeln('    mov x0, #1');

  { x1 = sp (readfds) }
  writeln('    mov x1, sp');

  { x2 = NULL }
  writeln('    mov x2, #0');

  { x3 = NULL }
  writeln('    mov x3, #0');

  { x4 = sp + 16 (timeout) }
  writeln('    add x4, sp, #16');

  { x16 = syscall number for select: 0x2000000 + 93 = 33554525 }
  EmitMovX16(33554525);
  EmitSvc;

  { select returns number of ready fds in x0, or -1 on error }
  { If x0 > 0, key is available; convert to 0 or 1 }
  writeln('    cmp x0, #0');
  writeln('    cset x0, gt');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitInitKeyboardRuntime;
{ Set terminal to raw mode for immediate key reading }
{ Uses ioctl TIOCGETA to get, then TIOCSETA to set with ICANON and ECHO cleared }
begin
  EmitLabel(rt_initkeyboard);
  EmitStp;
  EmitMovFP;
  EmitSubSP(80);  { Space for termios structure (72 bytes aligned to 80) }

  { First get current terminal settings: ioctl(0, TIOCGETA, sp) }
  { x0 = 0 (stdin) }
  writeln('    mov x0, #0');

  { x1 = TIOCGETA = 0x40487413 }
  { movz x1, #0x7413 }
  writeln('    movz x1, #0x7413');
  { movk x1, #0x4048, lsl #16 }
  writeln('    movk x1, #0x4048, lsl #16');

  { x2 = sp (pointer to termios buffer) }
  writeln('    mov x2, sp');

  { ioctl syscall: 0x2000000 + 54 = 33554486 }
  EmitMovX16(33554486);
  EmitSvc;

  { Save original c_lflag (at offset 24) to x23 (callee-saved) for later restore }
  writeln('    ldr x23, [sp, #24]');

  { Clear ICANON (0x100 = 256) and ECHO (0x8) from c_lflag }
  { Load current c_lflag into x10 }
  writeln('    mov x10, x23');

  { x11 = 0x108 (ICANON | ECHO) }
  writeln('    mov x11, #0x108');

  { bic x10, x10, x11 (clear bits) }
  writeln('    bic x10, x10, x11');

  { Store modified c_lflag back }
  writeln('    str x10, [sp, #24]');

  { Set VMIN (c_cc[16]) to 1 and VTIME (c_cc[17]) to 0 }
  { c_cc starts at offset 32, so VMIN is at 32+16=48, VTIME at 32+17=49 }
  writeln('    mov x10, #1');
  writeln('    strb w10, [sp, #48]');
  writeln('    mov x10, #0');
  writeln('    strb w10, [sp, #49]');

  { Now set the modified settings: ioctl(0, TIOCSETA, sp) }
  { x0 = 0 (stdin) }
  writeln('    mov x0, #0');

  { x1 = TIOCSETA = 0x80487414 }
  { movz x1, #0x7414 }
  writeln('    movz x1, #0x7414');
  { movk x1, #0x8048, lsl #16 }
  writeln('    movk x1, #0x8048, lsl #16');

  { x2 = sp }
  writeln('    mov x2, sp');

  EmitMovX16(33554486);
  EmitSvc;

  EmitAddSP(80);
  EmitLdp;
  EmitRet
end;

procedure EmitDoneKeyboardRuntime;
{ Restore terminal to normal (cooked) mode }
begin
  EmitLabel(rt_donekeyboard);
  EmitStp;
  EmitMovFP;
  EmitSubSP(80);

  { Get current settings first }
  writeln('    mov x0, #0');

  { x1 = TIOCGETA = 0x40487413 }
  writeln('    movz x1, #0x7413');
  writeln('    movk x1, #0x4048, lsl #16');

  writeln('    mov x2, sp');

  EmitMovX16(33554486);
  EmitSvc;

  { Restore original c_lflag from x23 }
  writeln('    str x23, [sp, #24]');

  { Set the restored settings }
  writeln('    mov x0, #0');

  { x1 = TIOCSETA = 0x80487414 }
  writeln('    movz x1, #0x7414');
  writeln('    movk x1, #0x8048, lsl #16');

  writeln('    mov x2, sp');

  EmitMovX16(33554486);
  EmitSvc;

  EmitAddSP(80);
  EmitLdp;
  EmitRet
end;

procedure EmitSinRuntime;
begin
  { sin(x) using Taylor series: x - x³/6 + x⁵/120 - x⁷/5040 + x⁹/362880 - x¹¹/39916800 }
  { Input: d0 = x, Output: d0 = sin(x) }
  EmitLabel(rt_sin);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save x to [x29, #-16] }
  EmitSturD0(-16);

  { Compute x² and save to [x29, #-24] }
  { fmul d1, d0, d0 }
  writeln('    fmul d1, d0, d0');
  { stur d1, [x29, #-24] }
  writeln('    stur d1, [x29, #-24]');

  { result = x (d0 already has x) }
  { Compute x³ = x² * x into d2 }
  { fmul d2, d1, d0 }
  writeln('    fmul d2, d1, d0');

  { Term 2: -x³/6 }
  EmitMovX0(6);
  EmitScvtfD0X0;
  { fdiv d3, d2, d0 - d3 = x³/6 }
  writeln('    fdiv d3, d2, d0');
  { Load x into d0 }
  EmitLdurD0(-16);
  { fsub d0, d0, d3 - result = x - x³/6 }
  writeln('    fsub d0, d0, d3');
  { Save result }
  EmitSturD0(-32);

  { Term 3: +x⁵/120 - multiply d3 by x², divide by 20 }
  { ldur d1, [x29, #-24] - reload x² }
  writeln('    ldur d1, [x29, #-24]');
  { fmul d3, d3, d1 - d3 = x⁵/6 }
  writeln('    fmul d3, d3, d1');
  EmitMovX0(20);
  EmitScvtfD0X0;
  { fdiv d3, d3, d0 }
  writeln('    fdiv d3, d3, d0');
  { Load result and add }
  EmitLdurD0(-32);
  { fadd d0, d0, d3 }
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 4: -x⁷/5040 }
  writeln('    ldur d1, [x29, #-24]');
  writeln('    fmul d3, d3, d1');
  EmitMovX0(42);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fsub d0, d0, d3');
  EmitSturD0(-32);

  { Term 5: +x⁹/362880 }
  writeln('    ldur d1, [x29, #-24]');
  writeln('    fmul d3, d3, d1');
  EmitMovX0(72);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 6: -x¹¹/39916800 }
  writeln('    ldur d1, [x29, #-24]');
  writeln('    fmul d3, d3, d1');
  EmitMovX0(110);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fsub d0, d0, d3');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitCosRuntime;
begin
  { cos(x) using Taylor series: 1 - x²/2 + x⁴/24 - x⁶/720 + x⁸/40320 - x¹⁰/3628800 }
  { Input: d0 = x, Output: d0 = cos(x) }
  EmitLabel(rt_cos);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save x to [x29, #-16] }
  EmitSturD0(-16);

  { Compute x² and save to [x29, #-24] }
  { fmul d1, d0, d0 }
  writeln('    fmul d1, d0, d0');
  { stur d1, [x29, #-24] }
  writeln('    stur d1, [x29, #-24]');

  { Start with result = 1.0 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { d3 = x² (current term numerator starts as x²) }
  { fmov d3, d1 }
  writeln('    fmov d3, d1');

  { Term 2: -x²/2 }
  EmitPushD0;
  EmitMovX0(2);
  EmitScvtfD0X0;
  { fdiv d3, d3, d0 }
  writeln('    fdiv d3, d3, d0');
  EmitPopD0;
  { fsub d0, d0, d3 }
  writeln('    fsub d0, d0, d3');
  EmitSturD0(-32);

  { Term 3: +x⁴/24 }
  { ldur d1, [x29, #-24] }
  writeln('    ldur d1, [x29, #-24]');
  { fmul d3, d3, d1 }
  writeln('    fmul d3, d3, d1');
  EmitMovX0(12);  { 24/2 = 12 }
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 4: -x⁶/720 }
  writeln('    ldur d1, [x29, #-24]');
  writeln('    fmul d3, d3, d1');
  EmitMovX0(30);  { 720/24 = 30 }
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fsub d0, d0, d3');
  EmitSturD0(-32);

  { Term 5: +x⁸/40320 }
  writeln('    ldur d1, [x29, #-24]');
  writeln('    fmul d3, d3, d1');
  EmitMovX0(56);  { 40320/720 = 56 }
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-32);

  { Term 6: -x¹⁰/3628800 }
  writeln('    ldur d1, [x29, #-24]');
  writeln('    fmul d3, d3, d1');
  EmitMovX0(90);  { 3628800/40320 = 90 }
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-32);
  writeln('    fsub d0, d0, d3');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitTanRuntime;
begin
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

  { d0 = cos(x), load sin(x) to d1 }
  writeln('    ldur d1, [x29, #-24]');

  { fdiv d0, d1, d0 - tan = sin/cos }
  writeln('    fdiv d0, d1, d0');

  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitExpRuntime;
var
  pos_lbl, neg_lbl, scale_lbl, scale_done_lbl, done_lbl: integer;
begin
  { exp(x) using range reduction + Taylor series }
  { Input: d0 = x, Output: d0 = exp(x) }
  { Range reduction: exp(x) = 2^n * exp(r) where n = round(x/ln(2)), r = x - n*ln(2) }
  EmitLabel(rt_exp);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { Save x to [x29, #-16] }
  EmitSturD0(-16);
  { [x29, #-24] = n (integer), [x29, #-32] = r (reduced x), [x29, #-40] = result }

  { Load 1/ln(2) = 1.4426950408889634 into d1 }
  { IEEE 754: 0x3FF71547652B82FE }
  { movz x0, #0x82FE }
  writeln('    movz x0, #0x82FE');
  { movk x0, #0x652B, lsl #16 }
  writeln('    movk x0, #0x652B, lsl #16');
  { movk x0, #0x7154, lsl #32 }
  writeln('    movk x0, #0x7154, lsl #32');
  { movk x0, #0x3FF7, lsl #48 }
  writeln('    movk x0, #0x3FF7, lsl #48');
  { fmov d1, x0 }
  writeln('    fmov d1, x0');

  { d0 = x, d1 = 1/ln(2) }
  { d0 = x / ln(2) = x * (1/ln(2)) }
  EmitLdurD0(-16);
  writeln('    fmul d0, d0, d1');

  { n = round(x/ln(2)) using fcvtas (round to nearest) }
  { fcvtas x0, d0 }
  writeln('    fcvtas x0, d0');
  EmitSturX0(-24);

  { Load ln(2) into d1 }
  { IEEE 754: 0x3FE62E42FEFA39EF }
  { movz x1, #0x39EF }
  writeln('    movz x1, #0x39EF');
  { movk x1, #0xFEFA, lsl #16 }
  writeln('    movk x1, #0xFEFA, lsl #16');
  { movk x1, #0x62E4, lsl #32 }
  writeln('    movk x1, #0x62E4, lsl #32');
  { movk x1, #0x3FE6, lsl #48 }
  writeln('    movk x1, #0x3FE6, lsl #48');
  { fmov d1, x1 }
  writeln('    fmov d1, x1');

  { r = x - n * ln(2) }
  { scvtf d0, x0 - convert n to float }
  EmitScvtfD0X0;
  { fmul d0, d0, d1 - d0 = n * ln(2) }
  writeln('    fmul d0, d0, d1');
  { fmov d2, d0 - save n*ln(2) in d2 }
  writeln('    fmov d2, d0');
  { d0 = x }
  EmitLdurD0(-16);
  { fsub d0, d0, d2 - r = x - n*ln(2) }
  writeln('    fsub d0, d0, d2');
  { Save r to [x29, #-32] }
  writeln('    stur d0, [x29, #-32]');

  { Now compute exp(r) using Taylor series }
  { r is small (|r| < ln(2)/2 ≈ 0.35), so series converges fast }
  { exp(r) = 1 + r + r²/2 + r³/6 + r⁴/24 + r⁵/120 + r⁶/720 + r⁷/5040 + r⁸/40320 }
  { d3 = current term, d0 = r }
  { fmov d3, d0 }
  writeln('    fmov d3, d0');
  { d0 = 1.0 }
  EmitPushD0;
  EmitMovX0(1);
  EmitScvtfD0X0;
  EmitPopD1;
  { fadd d0, d0, d1 - result = 1 + r }
  writeln('    fadd d0, d0, d1');
  EmitSturD0(-40);

  { Term 3: r²/2 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  { fmul d3, d3, d0 }
  writeln('    fmul d3, d3, d0');
  EmitMovX0(2);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 4: r³/6 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  writeln('    fmul d3, d3, d0');
  EmitMovX0(3);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 5: r⁴/24 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  writeln('    fmul d3, d3, d0');
  EmitMovX0(4);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 6: r⁵/120 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  writeln('    fmul d3, d3, d0');
  EmitMovX0(5);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 7: r⁶/720 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  writeln('    fmul d3, d3, d0');
  EmitMovX0(6);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 8: r⁷/5040 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  writeln('    fmul d3, d3, d0');
  EmitMovX0(7);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');
  EmitSturD0(-40);

  { Term 9: r⁸/40320 }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  writeln('    fmul d3, d3, d0');
  EmitMovX0(8);
  EmitScvtfD0X0;
  writeln('    fdiv d3, d3, d0');
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d3');

  { d0 = exp(r), now multiply by 2^n }
  { Store exp(r) in [x29, #-48] temporarily }
  writeln('    stur d0, [x29, #-48]');

  { Load n }
  EmitLdurX0(-24);
  { If n = 0, skip scaling }
  writeln('    cmp x0, #0');
  done_lbl := NewLabel;
  pos_lbl := NewLabel;
  scale_lbl := NewLabel;
  write('    b.eq L'); writeln(done_lbl);
  { If n > 0, multiply by 2 repeatedly }
  write('    b.gt L'); writeln(pos_lbl);
  { n < 0, divide by 2 repeatedly }
  { neg x0, x0 }
  writeln('    neg x0, x0');
  EmitSturX0(-56);
  { Load 0.5 into d1 }
  { IEEE 754 for 0.5 = 0x3FE0000000000000 }
  writeln('    movz x1, #0x3FE0, lsl #48');
  { fmov d1, x1 }
  writeln('    fmov d1, x1');
  EmitBranchLabel(scale_lbl);

  EmitLabel(pos_lbl);
  EmitSturX0(-56);
  { Load 2.0 into d1 }
  EmitMovX0(2);
  EmitScvtfD0X0;
  { fmov d1, d0 }
  writeln('    fmov d1, d0');

  EmitLabel(scale_lbl);
  { d1 = scale factor (2.0 or 0.5), [x29, #-56] = count, [x29, #-48] = result }
  { Load result }
  writeln('    ldur d0, [x29, #-48]');

  EmitLdurX0(-56);
  writeln('    cmp x0, #0');
  write('    b.eq L'); writeln(done_lbl);
  { fmul d0, d0, d1 }
  writeln('    fmul d0, d0, d1');
  { Store result }
  writeln('    stur d0, [x29, #-48]');
  { Decrement count }
  writeln('    sub x0, x0, #1');
  EmitSturX0(-56);
  EmitBranchLabel(scale_lbl);

  EmitLabel(done_lbl);
  { Load final result }
  writeln('    ldur d0, [x29, #-48]');

  EmitAddSP(64);
  EmitLdp;
  EmitRet
end;

procedure EmitLnRuntime;
var
  loop_lbl, done_lbl, reduce_lbl, reduce_up_lbl, iter_lbl, iter_done_lbl: integer;
begin
  { ln(x) using range reduction + Newton-Raphson }
  { Input: d0 = x, Output: d0 = ln(x) }
  { Range reduction: x = 2^n * m where 1 <= m < 2, so ln(x) = n*ln(2) + ln(m) }
  EmitLabel(rt_ln);
  EmitStp;
  EmitMovFP;
  EmitSubSP(64);

  { Save original x to [x29, #-16] }
  EmitSturD0(-16);
  { [x29, #-24] = count n (signed), [x29, #-32] = reduced x, [x29, #-40] = y estimate }
  { [x29, #-48] = iteration counter, [x29, #-56] = exp(y) temp }

  { Initialize count = 0 }
  EmitMovX0(0);
  EmitSturX0(-24);

  { Load x into d0 }
  EmitLdurD0(-16);
  { fmov d3, d0 - d3 = working copy of x }
  writeln('    fmov d3, d0');

  { Load 2.0 into d1 and 1.0 into d2 for comparisons }
  EmitMovX0(2);
  EmitScvtfD0X0;
  { fmov d1, d0 }
  writeln('    fmov d1, d0');
  EmitMovX0(1);
  EmitScvtfD0X0;
  { fmov d2, d0 }
  writeln('    fmov d2, d0');
  { d1 = 2.0, d2 = 1.0, d3 = x }

  { Reduce while d3 >= 2: d3 = d3 / 2, count++ }
  reduce_lbl := NewLabel;
  reduce_up_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(reduce_lbl);
  { fcmp d3, d1 }
  writeln('    fcmp d3, d1');
  { b.lt reduce_up_lbl - if d3 < 2, check if < 1 }
  write('    b.lt L'); writeln(reduce_up_lbl);
  { d3 >= 2: divide by 2 }
  { fdiv d3, d3, d1 }
  writeln('    fdiv d3, d3, d1');
  { count++ }
  EmitLdurX0(-24);
  writeln('    add x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(reduce_lbl);

  { Check if d3 < 1: multiply by 2, count-- }
  EmitLabel(reduce_up_lbl);
  { fcmp d3, d2 }
  writeln('    fcmp d3, d2');
  { b.ge done_lbl - if d3 >= 1, we're in [1, 2) }
  write('    b.ge L'); writeln(done_lbl);
  { d3 < 1: multiply by 2 }
  { fmul d3, d3, d1 }
  writeln('    fmul d3, d3, d1');
  { count-- }
  EmitLdurX0(-24);
  writeln('    sub x0, x0, #1');
  EmitSturX0(-24);
  EmitBranchLabel(reduce_up_lbl);

  EmitLabel(done_lbl);
  { Now d3 is in [1, 2), count is in [x29, #-24] }
  { Save reduced x to [x29, #-32] }
  writeln('    stur d3, [x29, #-32]');

  { Initial guess: y = d3 - 1 (will be in [0, 1)) }
  { fsub d0, d3, d2 }
  writeln('    fsub d0, d3, d2');
  EmitSturD0(-40);

  { Iterate 15 times for convergence }
  EmitMovX0(15);
  EmitSturX0(-48);

  iter_lbl := NewLabel;
  iter_done_lbl := NewLabel;
  EmitLabel(iter_lbl);

  { Check counter }
  EmitLdurX0(-48);
  { cmp x0, #0 }
  writeln('    cmp x0, #0');
  { b.eq iter_done }
  write('    b.eq L'); writeln(iter_done_lbl);

  { Decrement counter }
  writeln('    sub x0, x0, #1');
  EmitSturX0(-48);

  { Load y }
  EmitLdurD0(-40);
  { Calculate exp(y) }
  EmitBL(rt_exp);
  { Save exp(y) to [x29, #-56] }
  writeln('    stur d0, [x29, #-56]');

  { d0 = reduced x, d1 = exp(y) }
  { ldur d0, [x29, #-32] }
  writeln('    ldur d0, [x29, #-32]');
  { ldur d1, [x29, #-56] }
  writeln('    ldur d1, [x29, #-56]');

  { d2 = x - exp(y) }
  writeln('    fsub d2, d0, d1');

  { d3 = x + exp(y) }
  writeln('    fadd d3, d0, d1');

  { d2 = d2 / d3 = (x - exp(y))/(x + exp(y)) }
  writeln('    fdiv d2, d2, d3');

  { d2 = 2 * d2 }
  EmitMovX0(2);
  EmitScvtfD0X0;
  writeln('    fmul d2, d2, d0');

  { y = y + d2 }
  EmitLdurD0(-40);
  writeln('    fadd d0, d0, d2');
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
  writeln('    movz x0, #0x39EF');
  { movk x0, #0xFEFA, lsl #16 }
  writeln('    movk x0, #0xFEFA, lsl #16');
  { movk x0, #0x62E4, lsl #32 }
  writeln('    movk x0, #0x62E4, lsl #32');
  { movk x0, #0x3FE6, lsl #48 }
  writeln('    movk x0, #0x3FE6, lsl #48');
  { fmov d1, x0 }
  writeln('    fmov d1, x0');

  { Load count into d0, convert to float }
  EmitLdurX0(-24);
  { scvtf d0, x0 }
  EmitScvtfD0X0;

  { d0 = count * ln(2) }
  { fmul d0, d0, d1 }
  writeln('    fmul d0, d0, d1');

  { d0 = count*ln(2) + ln(reduced_x) }
  EmitPopD1;
  writeln('    fadd d0, d0, d1');

  EmitAddSP(64);
  EmitLdp;
  EmitRet
end;

procedure EmitRandomRuntime;
begin
  { random - return random 64-bit integer in x0 using LCG PRNG }
  { Uses x27 as seed (preserved across calls) }
  { LCG: seed = seed * 6364136223846793005 + 1442695040888963407 }
  { Output: x0 = random integer }
  EmitLabel(rt_random);
  EmitStp;
  EmitMovFP;

  { Load seed from x27 }
  writeln('    mov x0, x27');

  { If seed is 0, initialize with a default value }
  { cbnz x0, Lxxx }
  write('    cbnz x0, L'); writeln(label_count);
  { Initialize seed to 0x5DEECE66D }
  writeln('    movz x0, #0xE66D');
  writeln('    movk x0, #0xECE5, lsl #16');
  writeln('    movk x0, #0xDE, lsl #32');
  writeln('    movk x0, #0x5, lsl #48');

  EmitLabel(label_count);
  label_count := label_count + 1;

  { Load multiplier 6364136223846793005 = 0x5851F42D4C957F2D into x1 }
  writeln('    movz x1, #0x7F2D');
  writeln('    movk x1, #0x4C95, lsl #16');
  writeln('    movk x1, #0xF42D, lsl #32');
  writeln('    movk x1, #0x5851, lsl #48');

  { mul x0, x0, x1 }
  writeln('    mul x0, x0, x1');

  { Load increment 1442695040888963407 = 0x14057B7EF767814F into x1 }
  writeln('    movz x1, #0x814F');
  writeln('    movk x1, #0x7677, lsl #16');
  writeln('    movk x1, #0x7B7E, lsl #32');
  writeln('    movk x1, #0x1405, lsl #48');

  { add x0, x0, x1 }
  writeln('    add x0, x0, x1');

  { Store new seed in x27 }
  writeln('    mov x27, x0');

  EmitLdp;
  EmitRet
end;

procedure EmitArctanRuntime;
var
  use_identity_lbl, neg_lbl, done_lbl, compute_lbl: integer;
begin
  { arctan(x) using Taylor series for |x| <= 1 }
  { For |x| > 1: arctan(x) = pi/2 - arctan(1/x) if x > 0 }
  {              arctan(x) = -pi/2 - arctan(1/x) if x < 0 }
  { Input: d0 = x, Output: d0 = arctan(x) }
  EmitLabel(rt_arctan);
  EmitStp;
  EmitMovFP;
  EmitSubSP(48);

  { Save x to [x29, #-16] }
  EmitSturD0(-16);

  { Check if |x| > 1 }
  { fabs d1, d0 }
  writeln('    fabs d1, d0');
  { Load 1.0 into d2 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { fmov d2, d0 }
  writeln('    fmov d2, d0');
  { fcmp d1, d2 }
  writeln('    fcmp d1, d2');

  use_identity_lbl := NewLabel;
  compute_lbl := NewLabel;
  done_lbl := NewLabel;

  { b.le compute - if |x| <= 1, use direct Taylor series }
  write('    b.le L'); writeln(compute_lbl);

  { |x| > 1: use identity arctan(x) = sign(x)*pi/2 - arctan(1/x) }
  { Compute 1/x }
  EmitLdurD0(-16);  { x }
  { fdiv d0, d2, d0 - d0 = 1/x (d2 still has 1.0) }
  writeln('    fdiv d0, d2, d0');
  { Save 1/x to [x29, #-24] }
  EmitSturD0(-24);
  { Now compute arctan(1/x) recursively - but we're already in arctan! }
  { Let's just use Taylor series on 1/x since |1/x| < 1 }
  EmitBranchLabel(compute_lbl);

  EmitLabel(compute_lbl);
  { Taylor series: arctan(x) = x - x³/3 + x⁵/5 - x⁷/7 + x⁹/9 }
  { Use d0=x, d3=current term, d4=x², d5=result }
  EmitLdurD0(-16);  { x }

  { Check if we used identity (|x| > 1), then use 1/x instead }
  { Actually, let's simplify - compute arctan using the argument already in d0 }
  { If we came from the identity path, d0 has 1/x }
  { For now, just compute Taylor series on whatever is in d0 }

  { Save x to d5 as running result }
  { fmov d5, d0 }
  writeln('    fmov d5, d0');

  { d4 = x² }
  writeln('    fmul d4, d0, d0');

  { d3 = x (current power) }
  { fmov d3, d0 }
  writeln('    fmov d3, d0');

  { Term 2: -x³/3 }
  { d3 = d3 * d4 = x³ }
  writeln('    fmul d3, d3, d4');
  EmitMovX0(3);
  EmitScvtfD0X0;
  { fdiv d6, d3, d0 }
  writeln('    fdiv d6, d3, d0');
  { d5 = d5 - d6 }
  writeln('    fsub d5, d5, d6');

  { Term 3: +x⁵/5 }
  writeln('    fmul d3, d3, d4');
  EmitMovX0(5);
  EmitScvtfD0X0;
  writeln('    fdiv d6, d3, d0');
  writeln('    fadd d5, d5, d6');

  { Term 4: -x⁷/7 }
  writeln('    fmul d3, d3, d4');
  EmitMovX0(7);
  EmitScvtfD0X0;
  writeln('    fdiv d6, d3, d0');
  writeln('    fsub d5, d5, d6');

  { Term 5: +x⁹/9 }
  writeln('    fmul d3, d3, d4');
  EmitMovX0(9);
  EmitScvtfD0X0;
  writeln('    fdiv d6, d3, d0');
  writeln('    fadd d5, d5, d6');

  { Term 6: -x¹¹/11 }
  writeln('    fmul d3, d3, d4');
  EmitMovX0(11);
  EmitScvtfD0X0;
  writeln('    fdiv d6, d3, d0');
  writeln('    fsub d5, d5, d6');

  { Result in d5, move to d0 }
  { fmov d0, d5 }
  writeln('    fmov d0, d5');

  EmitAddSP(48);
  EmitLdp;
  EmitRet
end;

procedure EmitArcsinRuntime;
var
  pos_one_lbl, neg_one_lbl, normal_lbl, done_lbl: integer;
begin
  { arcsin(x) = arctan(x / sqrt(1 - x²)) }
  { Special cases: arcsin(1) = pi/2, arcsin(-1) = -pi/2 }
  { Input: d0 = x, Output: d0 = arcsin(x) }
  EmitLabel(rt_arcsin);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Save x to [x29, #-16] }
  EmitSturD0(-16);

  pos_one_lbl := NewLabel;
  neg_one_lbl := NewLabel;
  normal_lbl := NewLabel;
  done_lbl := NewLabel;

  { Check for x = 1 or x = -1 }
  { Load 1.0 into d1 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { fmov d1, d0 }
  writeln('    fmov d1, d0');
  { Load x back }
  EmitLdurD0(-16);
  { fcmp d0, d1 - compare x with 1.0 }
  writeln('    fcmp d0, d1');
  { b.eq pos_one_lbl }
  write('    b.eq L'); writeln(pos_one_lbl);
  { fneg d1, d1 - d1 = -1.0 }
  writeln('    fneg d1, d1');
  { fcmp d0, d1 - compare x with -1.0 }
  writeln('    fcmp d0, d1');
  { b.eq neg_one_lbl }
  write('    b.eq L'); writeln(neg_one_lbl);
  { Fall through to normal case }
  EmitBranchLabel(normal_lbl);

  { x = 1: return pi/2 }
  EmitLabel(pos_one_lbl);
  { Load pi/2 = 1.5707963267948966 }
  { IEEE 754: 0x3FF921FB54442D18 }
  writeln('    movz x0, #0x2D18');
  writeln('    movk x0, #0x5444, lsl #16');
  writeln('    movk x0, #0x21FB, lsl #32');
  writeln('    movk x0, #0x3FF9, lsl #48');
  { fmov d0, x0 }
  writeln('    fmov d0, x0');
  EmitBranchLabel(done_lbl);

  { x = -1: return -pi/2 }
  EmitLabel(neg_one_lbl);
  { Load -pi/2 = -1.5707963267948966 }
  { IEEE 754: 0xBFF921FB54442D18 }
  writeln('    movz x0, #0x2D18');
  writeln('    movk x0, #0x5444, lsl #16');
  writeln('    movk x0, #0x21FB, lsl #32');
  writeln('    movk x0, #0xBFF9, lsl #48');
  { fmov d0, x0 }
  writeln('    fmov d0, x0');
  EmitBranchLabel(done_lbl);

  EmitLabel(normal_lbl);
  { Normal case: arcsin(x) = arctan(x / sqrt(1 - x²)) }
  EmitLdurD0(-16);
  { Compute 1 - x² }
  { d1 = x² }
  writeln('    fmul d1, d0, d0');
  { Load 1.0 }
  EmitMovX0(1);
  EmitScvtfD0X0;
  { d0 = 1 - x² }
  writeln('    fsub d0, d0, d1');

  { d0 = sqrt(1 - x²) }
  writeln('    fsqrt d0, d0');

  { d0 = x / sqrt(1 - x²) }
  writeln('    ldur d1, [x29, #-16]');
  writeln('    fdiv d0, d1, d0');

  { d0 = arctan(x / sqrt(1 - x²)) }
  EmitBL(rt_arctan);

  EmitLabel(done_lbl);
  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

procedure EmitArccosRuntime;
begin
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
  writeln('    movz x0, #0x2D18');
  writeln('    movk x0, #0x5444, lsl #16');
  writeln('    movk x0, #0x21FB, lsl #32');
  writeln('    movk x0, #0x3FF9, lsl #48');
  EmitFmovD0X0;

  { d0 = pi/2 - arcsin(x) }
  EmitLdurD0(-16);
  writeln('    fmov d1, d0');
  { Reload pi/2 }
  writeln('    movz x0, #0x2D18');
  writeln('    movk x0, #0x5444, lsl #16');
  writeln('    movk x0, #0x21FB, lsl #32');
  writeln('    movk x0, #0x3FF9, lsl #48');
  EmitFmovD0X0;
  { fsub d0, d0, d1 }
  writeln('    fsub d0, d0, d1');

  EmitAddSP(16);
  EmitLdp;
  EmitRet
end;

procedure EmitParamStrRuntime;
var
  loop_lbl, done_lbl, copy_lbl, copy_done_lbl, empty_lbl: integer;
begin
  { paramstr(n) - convert argv[n] to Pascal string }
  { Input: x0 = n (index into argv) }
  { Output: x0 = pointer to Pascal string allocated from heap }
  { Uses x25 = argc, x26 = argv (saved at program start) }
  { Uses x21 = heap pointer for allocation }
  EmitLabel(rt_paramstr);
  EmitStp;
  EmitMovFP;
  EmitSubSP(32);

  { Allocate string buffer from heap: x8 = x21, x21 += 256 }
  { Save dest addr in x8 }
  writeln('    mov x8, x21');
  { Advance heap pointer }
  writeln('    add x21, x21, #256');

  done_lbl := NewLabel;
  empty_lbl := NewLabel;
  loop_lbl := NewLabel;
  copy_lbl := NewLabel;
  copy_done_lbl := NewLabel;

  { Check bounds: if n >= argc, return empty string }
  { cmp x0, x25 }
  writeln('    cmp x0, x25');
  { b.ge empty }
  write('    b.ge L'); writeln(empty_lbl);

  { Check for negative index }
  { cmp x0, #0 }
  writeln('    cmp x0, #0');
  { b.lt empty }
  write('    b.lt L'); writeln(empty_lbl);

  { Load argv[n] pointer: x1 = argv[n] = *(x26 + n*8) }
  { ldr x1, [x26, x0, lsl #3] }
  writeln('    ldr x1, [x26, x0, lsl #3]');

  { Check if argv[n] is null }
  { cbz x1, empty }
  write('    cbz x1, L'); writeln(empty_lbl);

  { x1 = pointer to C string, x8 = dest buffer }
  { First count length (max 255) }
  { mov x2, #0 - length counter }
  writeln('    mov x2, #0');
  { mov x3, x1 - save string pointer }
  writeln('    mov x3, x1');

  EmitLabel(loop_lbl);
  { ldrb w4, [x1], #1 - load byte and increment }
  writeln('    ldrb w4, [x1], #1');
  { cbz w4, copy_start - if null terminator, start copying }
  write('    cbz w4, L'); writeln(copy_lbl);
  { add x2, x2, #1 - increment length }
  writeln('    add x2, x2, #1');
  { cmp x2, #255 - max length }
  writeln('    cmp x2, #255');
  { b.lt loop }
  write('    b.lt L'); writeln(loop_lbl);

  EmitLabel(copy_lbl);
  { x2 = length, x3 = source pointer, x8 = dest buffer }
  { Store length byte at [x8] }
  { strb w2, [x8] }
  writeln('    strb w2, [x8]');
  { x0 = x8 + 1 (dest for chars), x1 = x3 (source) }
  writeln('    add x0, x8, #1');
  { mov x1, x3 - restore source pointer }
  writeln('    mov x1, x3');

  { Copy loop }
  EmitLabel(copy_done_lbl);
  { cbz x2, done - if count = 0, done }
  write('    cbz x2, L'); writeln(done_lbl);
  { ldrb w4, [x1], #1 }
  writeln('    ldrb w4, [x1], #1');
  { strb w4, [x0], #1 }
  writeln('    strb w4, [x0], #1');
  { sub x2, x2, #1 }
  writeln('    sub x2, x2, #1');
  EmitBranchLabel(copy_done_lbl);

  { Empty string: store length 0 }
  EmitLabel(empty_lbl);
  writeln('    strb wzr, [x8]');

  EmitLabel(done_lbl);
  { Return pointer to string buffer (x8) }
  { mov x0, x8 }
  writeln('    mov x0, x8');

  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;


