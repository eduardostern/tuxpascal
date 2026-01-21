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
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #0 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;

  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge Lxx }
  writechar(76); write(neg_lbl);
  EmitNL;

  { Print minus sign }
  EmitMovX0(1);
  EmitSturX0(-32);
  EmitMovX0(45);  { '-' }
  EmitSturX0(-8);
  EmitMovX16(33554436); { 0x2000004 }
  EmitMovX0X20;
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #8 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
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
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x0, x0, #48 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;

  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur x1, [x29, #-40] }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);
  EmitNL;

  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x2, x29, #48 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;

  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb w0, [x2, x1] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(93);
  EmitNL;

  { digit count++ }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x1, x1, #1 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;

  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur x1, [x29, #-40] }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);
  EmitNL;

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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x0, x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitSturX0(-40);

  { Load digit }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #48 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;

  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w0, [x1, x0] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(93);
  EmitNL;

  { Print char }
  EmitSturX0(-8);
  EmitMovX16(33554436);
  EmitMovX0X20;
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #8 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x1, x29, #9 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read returned >= 1 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge Lxx }
  writechar(76); write(label_count);
  EmitNL;
  EmitMovX0(-1);  { EOF }
  EmitBranchLabel(label_count + 1);
  EmitLabel(label_count);
  label_count := label_count + 1;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w0, [sp] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb w0, [sp] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  { Load length from [x0] into x1 and save to [x29, #-16] }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(49); writechar(44); writechar(32);  { w1, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  { Initialize index to 0 at [x29, #-24] }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  { Loop label }
  EmitLabel(loop_lbl);
  { Load index and length, compare }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(49);  { x1 }
  EmitNL;
  { b.ge done_lbl }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;
  { Load char at [base + index + 1] }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  { Call print_char }
  EmitBL(rt_print_char);
  { Increment index }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x21, #0 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x22, #0 }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;

  { Skip whitespace loop }
  skip_ws_lbl := NewLabel;
  EmitLabel(skip_ws_lbl);
  { Read one char }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read failed }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  done_lbl := NewLabel;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt done }
  writechar(76); write(done_lbl);
  EmitNL;
  { Load char into x23 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w23, [sp] }
  writechar(119); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  { Check if space (32), tab (9), newline (10), or carriage return (13) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #32 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(51); writechar(50);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #9 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #10 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #13 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(51);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;

  { Check for minus sign (45) }
  neg_lbl := NewLabel;
  not_neg_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #45 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(53);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne not_neg }
  writechar(76); write(not_neg_lbl);
  EmitNL;
  { Set negative flag }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x22, #1 }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitBranchLabel(neg_lbl);
  EmitLabel(not_neg_lbl);
  { Not a minus, so it should be a digit - process it }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x23, x23, #48 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;
  { Check if valid digit (0-9) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #9 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(104); writechar(105); writechar(32);  { b.hi done (unsigned >9) }
  writechar(76); write(done_lbl);
  EmitNL;
  { Add to accumulated value: x21 = x21 * 10 + x23 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x24, #10 }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul x21, x21, x24 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(52);
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x21, x21, x23 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(51);
  EmitNL;

  { Read digit loop }
  EmitLabel(neg_lbl);
  read_digit_lbl := NewLabel;
  EmitLabel(read_digit_lbl);
  { Read one char }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read failed }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt done }
  writechar(76); write(done_lbl);
  EmitNL;
  { Load char into x23 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w23, [sp] }
  writechar(119); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  { Save original char to x18 for pushback }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x18, x23 }
  writechar(120); writechar(49); writechar(56); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(51);
  EmitNL;
  { Convert to digit }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x23, x23, #48 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;
  { Check if valid digit (0-9) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x23, #9 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(104); writechar(105); writechar(32);  { b.hi done }
  writechar(76); write(done_lbl);
  EmitNL;
  { Add to accumulated value: x21 = x21 * 10 + x23 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x24, #10 }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul x21, x21, x24 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(52);
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x21, x21, x23 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(51);
  EmitNL;
  EmitBranchLabel(read_digit_lbl);

  { Done - apply negative if needed }
  EmitLabel(done_lbl);
  skip_neg_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(98); writechar(122); writechar(32);  { cbz x22, Lxx (skip neg) }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(76); write(skip_neg_lbl);
  EmitNL;
  EmitIndent;
  writechar(110); writechar(101); writechar(103); writechar(32);  { neg x21, x21 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49);
  EmitNL;
  EmitLabel(skip_neg_lbl);
  { Move result to x0 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x21 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49);
  EmitNL;
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
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x18, #0 }
  writechar(120); writechar(49); writechar(56); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt loop (no pushback) }
  writechar(76); write(loop_lbl);
  EmitNL;
  { Check if pushback is newline }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x18, #10 }
  writechar(120); writechar(49); writechar(56); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  { Clear pushback }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x18, #-1 }
  writechar(120); writechar(49); writechar(56); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq done (was newline, done) }
  writechar(76); write(done_lbl);
  EmitNL;

  EmitLabel(loop_lbl);
  { Read one char }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  { Check if read failed }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt done }
  writechar(76); write(done_lbl);
  EmitNL;
  { Load char }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w0, [sp] }
  writechar(119); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  { Check if newline (10) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #10 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne loop }
  writechar(76); write(loop_lbl);
  EmitNL;

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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { Check for negative }
  neg_lbl := NewLabel;
  skip_neg_lbl := NewLabel;
  { fcmp d0, #0.0 }
  EmitIndent;
  writechar(102); writechar(99); writechar(109); writechar(112); writechar(32);  { fcmp }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(35); writechar(48); writechar(46); writechar(48);  { #0.0 }
  EmitNL;
  { b.ge skip_neg }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(neg_lbl);
  EmitNL;

  { Print '-' and negate }
  EmitMovX0(45);  { '-' }
  EmitBL(rt_print_char);
  { fneg d0, d0 }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitFNeg;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  EmitLabel(neg_lbl);

  { Load value and get integer part }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { fcvtzs x0, d0 - truncate to integer }
  EmitFcvtzsX0D0;

  { Save integer part to [x29, #-24] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

  { Print integer part }
  EmitBL(rt_print_int);

  { Print '.' }
  EmitMovX0(46);  { '.' }
  EmitBL(rt_print_char);

  { Calculate fractional part: (original - int_part) * 1000000 }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { Load integer part and convert to float }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

  { scvtf d1, x0 }
  EmitIndent;
  writechar(115); writechar(99); writechar(118); writechar(116); writechar(102); writechar(32);  { scvtf }
  writechar(100); writechar(49); writechar(44); writechar(32);  { d1, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  { fsub d0, d0, d1 - fractional part (d0 = original - int_part) }
  { EmitFSub does d0=d1-d0, so emit inline for d0=d0-d1 }
  EmitIndent;
  writechar(102); writechar(115); writechar(117); writechar(98); writechar(32);  { fsub }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(100); writechar(49);  { d1 }
  EmitNL;

  { Multiply by 1000000 }
  EmitMovX0(1000000);
  EmitScvtfD0X0;  { d0 = 1000000.0 }
  EmitPushD0;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(99); writechar(118); writechar(116); writechar(102); writechar(32);  { scvtf }
  writechar(100); writechar(49); writechar(44); writechar(32);  { d1, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  { fsub d0, d0, d1 - d0 = frac part (original - int_part) }
  { EmitFSub does d0=d1-d0, so emit inline for d0=d0-d1 }
  EmitIndent;
  writechar(102); writechar(115); writechar(117); writechar(98); writechar(32);  { fsub }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(100); writechar(49);  { d1 }
  EmitNL;
  EmitPopD1;  { d1 = 1000000.0 }
  EmitFMul;  { d0 = frac * 1000000 }

  { Convert to integer }
  EmitFcvtzsX0D0;

  { Save fractional digits to [x29, #-32] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;

  { Print fractional part with leading zeros (6 digits) }
  { Count digits needed }
  frac_loop_lbl := NewLabel;
  print_frac_lbl := NewLabel;

  { Print leading zeros: if frac < 100000, print '0', etc }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;

  { x1 = 100000 - use movz since 100000 > 65535 }
  { movz x1, #34464; movk x1, #1, lsl #16 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);  { movz }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(51); writechar(52); writechar(52); writechar(54); writechar(52);  { #34464 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);  { movk }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(49);  { #1 }
  writechar(44); writechar(32);
  writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
  writechar(35); writechar(49); writechar(54);  { #16 }
  EmitNL;

  EmitLabel(frac_loop_lbl);
  { cmp x1, #10 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  { b.lt print_frac }
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(print_frac_lbl);
  EmitNL;
  { cmp x0, x1 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49);  { x1 }
  EmitNL;
  { b.ge print_frac }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(print_frac_lbl);
  EmitNL;
  { Print '0' }
  EmitPushX0;
  EmitPushX1;
  EmitMovX0(48);
  EmitBL(rt_print_char);
  EmitPopX1;
  EmitPopX0;
  { x1 = x1 / 10 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(117); writechar(100); writechar(105); writechar(118); writechar(32);  { udiv }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitBranchLabel(frac_loop_lbl);

  EmitLabel(print_frac_lbl);
  { Load and print the fractional value }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x21, #0 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x22, #0 }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x23, #0 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x24, #1 }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;

  { Skip whitespace }
  skip_ws_lbl := NewLabel;
  EmitLabel(skip_ws_lbl);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  done_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt done }
  writechar(76); write(done_lbl);
  EmitNL;
  { Load char }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w25, [sp] }
  writechar(119); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;
  { Check whitespace }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #32 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(51); writechar(50);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #9 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #10 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #13 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(51);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq skip_ws }
  writechar(76); write(skip_ws_lbl);
  EmitNL;

  { Check for minus }
  neg_lbl := NewLabel;
  not_neg_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #45 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(53);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne not_neg }
  writechar(76); write(not_neg_lbl);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x23, #1 }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitBranchLabel(neg_lbl);
  EmitLabel(not_neg_lbl);

  { First char is a digit - process it }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x25, x25, #48 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #9 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(104); writechar(105); writechar(32);  { b.hi done }
  writechar(76); write(done_lbl);
  EmitNL;
  { x21 = x21 * 10 + x25 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x26, #10 }
  writechar(120); writechar(50); writechar(54); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul x21, x21, x26 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(54);
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x21, x21, x25 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(53);
  EmitNL;

  { Read integer part loop }
  EmitLabel(neg_lbl);
  read_int_lbl := NewLabel;
  read_frac_lbl := NewLabel;
  EmitLabel(read_int_lbl);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt done }
  writechar(76); write(done_lbl);
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w25, [sp] }
  writechar(119); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;

  { Check for '.' }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #46 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(54);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq read_frac }
  writechar(76); write(read_frac_lbl);
  EmitNL;

  { Check if digit }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x25, x25, #48 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #9 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(104); writechar(105); writechar(32);  { b.hi done }
  writechar(76); write(done_lbl);
  EmitNL;
  { x21 = x21 * 10 + x25 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x26, #10 }
  writechar(120); writechar(50); writechar(54); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul x21, x21, x26 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(54);
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x21, x21, x25 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(53);
  EmitNL;
  EmitBranchLabel(read_int_lbl);

  { Read fractional part }
  EmitLabel(read_frac_lbl);
  skip_neg_lbl := NewLabel;
  EmitLabel(skip_neg_lbl);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x0, x19 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, sp }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #1 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read }
  EmitSvc;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x0, #1 }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt done }
  writechar(76); write(done_lbl);
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w25, [sp] }
  writechar(119); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  EmitNL;

  { Check if digit }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub x25, x25, #48 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(56);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x25, #9 }
  writechar(120); writechar(50); writechar(53); writechar(44); writechar(32);
  writechar(35); writechar(57);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(104); writechar(105); writechar(32);  { b.hi done }
  writechar(76); write(done_lbl);
  EmitNL;
  { x22 = x22 * 10 + x25, x24 = x24 * 10 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x26, #10 }
  writechar(120); writechar(50); writechar(54); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul x22, x22, x26 }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(54);
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add x22, x22, x25 }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(53);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul x24, x24, x26 }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(54);
  EmitNL;
  EmitBranchLabel(skip_neg_lbl);

  { Done - combine integer and fractional parts }
  EmitLabel(done_lbl);
  { d0 = x21 (integer part) }
  EmitIndent;
  writechar(115); writechar(99); writechar(118); writechar(116); writechar(102); writechar(32);  { scvtf }
  writechar(100); writechar(48); writechar(44); writechar(32);  { d0, }
  writechar(120); writechar(50); writechar(49);  { x21 }
  EmitNL;
  { d1 = x22 (fractional part) }
  EmitIndent;
  writechar(115); writechar(99); writechar(118); writechar(116); writechar(102); writechar(32);  { scvtf }
  writechar(100); writechar(49); writechar(44); writechar(32);  { d1, }
  writechar(120); writechar(50); writechar(50);  { x22 }
  EmitNL;
  { d2 = x24 (scale) }
  EmitIndent;
  writechar(115); writechar(99); writechar(118); writechar(116); writechar(102); writechar(32);  { scvtf }
  writechar(100); writechar(50); writechar(44); writechar(32);  { d2, }
  writechar(120); writechar(50); writechar(52);  { x24 }
  EmitNL;
  { d1 = d1 / d2 }
  EmitIndent;
  writechar(102); writechar(100); writechar(105); writechar(118); writechar(32);  { fdiv }
  writechar(100); writechar(49); writechar(44); writechar(32);  { d1, }
  writechar(100); writechar(49); writechar(44); writechar(32);  { d1, }
  writechar(100); writechar(50);  { d2 }
  EmitNL;
  { d0 = d0 + d1 }
  EmitFAdd;

  { Apply negative if needed }
  skip_neg_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(98); writechar(122); writechar(32);  { cbz x23, skip }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);
  writechar(76); write(skip_neg_lbl);
  EmitNL;
  EmitFNeg;
  EmitLabel(skip_neg_lbl);

  EmitAddSP(64);
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x21, x0 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x22, x1 }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(49);
  EmitNL;

  { Default: x19 = 0 (stdin), x20 = 1 (stdout), x18 = -1 (no pushback) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x19, #0 }
  writechar(120); writechar(49); writechar(57); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x20, #1 }
  writechar(120); writechar(50); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x18, #-1 }
  writechar(120); writechar(49); writechar(56); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49);
  EmitNL;

  { If argc < 2, skip input file open }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x21, #2 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(50);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt Lxx }
  writechar(76); write(skip_input_lbl);
  EmitNL;

  { Load argv[1] into x0 (input filename) }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);  { ldr x0, [x22, #8] }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(56); writechar(93);
  EmitNL;

  { Open input file: open(filename, O_RDONLY, 0) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, #0 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #0 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(48);
  EmitNL;
  EmitMovX16(33554437);  { 0x2000005 = open }
  EmitSvc;

  { Move input fd to x19 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x19, x0 }
  writechar(120); writechar(49); writechar(57); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL;

  { If argc < 4, skip output file open (need: prog input.pas -o output.s) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp x21, #4 }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(52);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt Lxx }
  writechar(76); write(skip_output_lbl);
  EmitNL;

  { Check if argv[2] == "-o": load argv[2], check first two bytes }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);  { ldr x0, [x22, #16] }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(54); writechar(93);
  EmitNL;
  { Load first byte, check for '-' (45) }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w1, [x0] }
  writechar(119); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(48); writechar(93);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp w1, #45 }
  writechar(119); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(53);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne skip }
  writechar(76); write(skip_output_lbl);
  EmitNL;
  { Load second byte, check for 'o' (111) }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb w1, [x0, #1] }
  writechar(119); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(93);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp w1, #111 }
  writechar(119); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(49); writechar(49);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne skip }
  writechar(76); write(skip_output_lbl);
  EmitNL;

  { Load argv[3] into x0 (output filename) }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);  { ldr x0, [x22, #24] }
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(50); writechar(52); writechar(93);
  EmitNL;

  { Open output file: open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644) }
  { O_WRONLY=1, O_CREAT=0x200, O_TRUNC=0x400 => 0x601 = 1537 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x1, #1537 }
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(35); writechar(49); writechar(53); writechar(51); writechar(55);
  EmitNL;
  { mode 0644 = 420 decimal }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x2, #420 }
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(35); writechar(52); writechar(50); writechar(48);
  EmitNL;
  EmitMovX16(33554437);  { 0x2000005 = open }
  EmitSvc;

  { Move output fd to x20 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov x20, x0 }
  writechar(120); writechar(50); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL;

  EmitLabel(skip_output_lbl);
  EmitLabel(skip_input_lbl)
end;

