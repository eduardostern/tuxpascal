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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  { x22 = character count (starts at 0) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);  { x22, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { Read loop }
  loop_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Check if count >= 255 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);  { x22, }
  writechar(35); writechar(50); writechar(53); writechar(53);  { #255 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;

  { Read one character: read(x19, sp, 1) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(57);  { x19 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(115); writechar(112);  { sp }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitMovX16(33554435);  { 0x2000003 = read syscall }
  EmitSvc;

  { Check if read failed or EOF (x0 < 1) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(done_lbl);
  EmitNL;

  { Load character into x23 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(51); writechar(44); writechar(32);  { w23, }
  writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
  EmitNL;

  { Check if newline (10) or carriage return (13) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);  { x23, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(done_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(51); writechar(44); writechar(32);  { x23, }
  writechar(35); writechar(49); writechar(51);  { #13 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(done_lbl);
  EmitNL;

  { Store character at buffer[count+1] }
  { x24 = x21 + x22 + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);  { x24, }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
  writechar(120); writechar(50); writechar(50);  { x22 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);  { x24, }
  writechar(120); writechar(50); writechar(52); writechar(44); writechar(32);  { x24, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(50); writechar(51); writechar(44); writechar(32);  { w23, }
  writechar(91); writechar(120); writechar(50); writechar(52); writechar(93);  { [x24] }
  EmitNL;

  { Increment count }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);  { x22, }
  writechar(120); writechar(50); writechar(50); writechar(44); writechar(32);  { x22, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Loop back }
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(loop_lbl);
  EmitNL;

  { Done - store length at buffer[0] }
  EmitLabel(done_lbl);
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(50); writechar(50); writechar(44); writechar(32);  { w22, }
  writechar(91); writechar(120); writechar(50); writechar(49); writechar(93);  { [x21] }
  EmitNL;

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

procedure EmitHeapInitRuntime;
begin
  { Initialize heap using mmap syscall }
  { Allocates 1MB of memory for heap, stores base in x21 }
  EmitLabel(rt_heap_init);
  EmitStp;
  EmitMovFP;
  EmitSubSP(16);

  { mov x0, #0  - addr = NULL (let kernel choose) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { mov x1, #1048576  - length = 1MB (0x100000) }
  { Need movz + movk since 1048576 > 65535 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);  { movz }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);  { movk }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(49); writechar(54);  { #16 }
  writechar(44); writechar(32);
  writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
  writechar(35); writechar(49); writechar(54);  { #16 }
  EmitNL;

  { mov x2, #3  - PROT_READ | PROT_WRITE }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(51);  { #3 }
  EmitNL;

  { mov x3, #4098  - MAP_PRIVATE | MAP_ANON (0x1002) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(52); writechar(48); writechar(57); writechar(56);  { #4098 }
  EmitNL;

  { mov x4, #-1  - fd = -1 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(110); writechar(32);  { movn }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { mov x5, #0  - offset = 0 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { mmap syscall: 0x20000C5 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);  { movz }
  writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);  { x16, }
  writechar(35); writechar(48); writechar(120); writechar(67); writechar(53);  { #0xC5 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);  { movk }
  writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);  { x16, }
  writechar(35); writechar(48); writechar(120); writechar(50); writechar(48); writechar(48);  { #0x200 }
  writechar(44); writechar(32);
  writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
  writechar(35); writechar(49); writechar(54);  { #16 }
  EmitNL;

  EmitSvc;

  { mov x21, x0  - store heap base in x21 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(49); writechar(44); writechar(32);  { x21, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  EmitAddSP(16);
  EmitLdp;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  { stur x1, [x29, #-16] - source }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { Load length from source [x1] into x2 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;
  { Store length to dest [x0] }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  { Save length to [x29, #-24] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

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
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  { cmp x0, x2 - if index >= length, done }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  { b.ge done }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;

  { x3 = index + 1 (offset into string) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load source char: ldrb w4, [x1, x3] where x1 = source }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(51); writechar(93);  { x3] }
  EmitNL;

  { Store to dest char: strb w4, [x5, x3] where x5 = dest }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(53); writechar(44); writechar(32);  { [x5, }
  writechar(120); writechar(51); writechar(93);  { x3] }
  EmitNL;

  { Increment index }
  EmitLdurX0(-32);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { Load lengths }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;

  { Compare lengths first }
  not_equal_lbl := NewLabel;
  done_lbl := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  { b.ne not_equal }
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne }
  writechar(76); write(not_equal_lbl);
  EmitNL;

  { Save length to [x29, #-24] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

  { Initialize index to 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  { Loop: compare each byte }
  loop_lbl := NewLabel;
  EmitLabel(loop_lbl);

  { Load index }
  EmitLdurX0(-32);
  { Load length }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  { cmp x0, x2 - if index >= length, all equal }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  { b.ge equal (return 1) }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;

  { x3 = index + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load chars from both strings }
  EmitLdurX0(-8);  { string 1 addr }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(51); writechar(93);  { x3] }
  EmitNL;

  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(51); writechar(93);  { x3] }
  EmitNL;

  { Compare chars }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(53);  { x5 }
  EmitNL;
  { b.ne not_equal }
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne }
  writechar(76); write(not_equal_lbl);
  EmitNL;

  { Increment index }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  { stur x2, [x29, #-24] - string 2 }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

  { Load lengths }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] - len1 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(50); writechar(93);  { [x2] - len2 }
  EmitNL;

  { Calculate total length (capped at 255) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(52);  { x4 }
  EmitNL;
  { cmp x5, #255 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(50); writechar(53); writechar(53);  { #255 }
  EmitNL;
  { csel x5, x5, x6, le - if x5 <= 255, keep it; else use 255 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(35); writechar(50); writechar(53); writechar(53);  { #255 }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(115); writechar(101); writechar(108); writechar(32);  { csel }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(108); writechar(101);  { le }
  EmitNL;

  { Store total length to dest }
  EmitLdurX0(-8);  { dest }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;

  { Save len1 to [x29, #-32] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;

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
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  { cmp x0, x3 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  { b.ge loop2 }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(loop2_lbl);
  EmitNL;

  { x7 = index + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load source1 char }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(56); writechar(44); writechar(32);  { w8, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(55); writechar(93);  { x7] }
  EmitNL;

  { Store to dest }
  EmitLdurX0(-8);
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(56); writechar(44); writechar(32);  { w8, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(55); writechar(93);  { x7] }
  EmitNL;

  { Increment index }
  EmitLdurX0(-40);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitSturX0(-40);
  EmitBranchLabel(loop1_lbl);

  { Loop 2: copy string2 chars }
  EmitLabel(loop2_lbl);
  { Reset index to 0 for string2 }
  EmitMovX0(0);
  EmitSturX0(-40);

  { Load len2 into x4 }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] - string2 addr }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(50); writechar(93);  { [x2] }
  EmitNL;
  { Save len2 to [x29, #-48] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(56); writechar(93);  { #-48] }
  EmitNL;

  { Loop 2 body label }
  EmitLabel(label_count);
  label_count := label_count + 1;

  { Load index and len2 }
  EmitLdurX0(-40);
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(56); writechar(93);  { #-48] }
  EmitNL;
  { cmp x0, x4 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(52);  { x4 }
  EmitNL;
  { b.ge done }
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;

  { x7 = index + 1 (source offset) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load source2 char }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(56); writechar(44); writechar(32);  { w8, }
  writechar(91); writechar(120); writechar(50); writechar(44); writechar(32);  { [x2, }
  writechar(120); writechar(55); writechar(93);  { x7] }
  EmitNL;

  { Calculate dest offset: len1 + index + 1 }
  EmitLdurX0(-40);  { index }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] - len1 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Store to dest }
  EmitLdurX0(-8);
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(56); writechar(44); writechar(32);  { w8, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(55); writechar(93);  { x7] }
  EmitNL;

  { Increment index }
  EmitLdurX0(-40);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

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
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;            { x2 = len1 }

  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;            { x3 = len2 }

  { Load index }
  EmitLdurX0(-24);   { x0 = index }

  { if index >= min(len1, len2), check length }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(check_len_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(check_len_lbl);
  EmitNL;

  { Compare chars at index+1 }
  { x4 = index + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load char from s1 }
  EmitLdurX0(-8);
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(52); writechar(93);  { x4] }
  EmitNL;

  { Load char from s2 }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(54); writechar(44); writechar(32);  { w6, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(52); writechar(93);  { x4] }
  EmitNL;

  { Compare chars }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(54);  { x6 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(less_lbl);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
  writechar(76); write(greater_lbl);
  EmitNL;

  { Chars equal, increment index }
  EmitLdurX0(-24);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitSturX0(-24);
  EmitBranchLabel(loop_lbl);

  { Check lengths when all compared chars are equal }
  EmitLabel(check_len_lbl);
  EmitLdurX0(-8);
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(less_lbl);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
  writechar(76); write(greater_lbl);
  EmitNL;
  { Equal }
  EmitMovX0(0);
  EmitBranchLabel(done_lbl);

  EmitLabel(less_lbl);
  EmitMovX0(0);
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;  { x0 = -1 }
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  outer_lbl := NewLabel;
  inner_lbl := NewLabel;
  done_lbl := NewLabel;
  found_lbl := NewLabel;
  not_found_lbl := NewLabel;
  next_pos_lbl := NewLabel;

  { If substr is empty, return 1 }
  EmitLdurX0(-8);
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(98); writechar(122); writechar(32);  { cbz }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(76); write(found_lbl);
  EmitNL;
  { x2 = substr len }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);  { #-40] }
  EmitNL;

  { Load string length }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;
  { x3 = string len }

  { outer_idx = 0 (will be 1-based position - 1) }
  EmitMovX0(0);
  EmitSturX0(-24);

  EmitLabel(outer_lbl);
  { Check if outer_idx + substr_len > string_len }
  EmitLdurX0(-24);  { outer_idx }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);  { #-40] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  { x4 = outer_idx + substr_len }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
  writechar(76); write(not_found_lbl);
  EmitNL;

  { inner_idx = 0 }
  EmitMovX0(0);
  EmitSturX0(-32);

  EmitLabel(inner_lbl);
  { if inner_idx >= substr_len, found! }
  EmitLdurX0(-32);
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);  { #-40] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(found_lbl);
  EmitNL;

  { Compare substr[inner_idx+1] with string[outer_idx+inner_idx+1] }
  EmitLdurX0(-8);  { substr addr }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(52); writechar(93);  { x4] }
  EmitNL;
  { x5 = substr[inner_idx+1] }

  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitLdurX0(-24);  { outer_idx }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(52);  { x4 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(54); writechar(44); writechar(32);  { w6, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(52); writechar(93);  { x4] }
  EmitNL;
  { x6 = string[outer_idx+inner_idx+1] }

  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(54);  { x6 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne }
  writechar(76); write(next_pos_lbl);
  EmitNL;

  { Chars match, increment inner_idx }
  EmitLdurX0(-32);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitSturX0(-32);
  EmitBranchLabel(inner_lbl);

  EmitLabel(next_pos_lbl);
  { Chars don't match, try next outer position }
  EmitLdurX0(-24);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitSturX0(-24);
  EmitBranchLabel(outer_lbl);

  EmitLabel(found_lbl);
  { Return outer_idx + 1 (1-based position) }
  EmitLdurX0(-24);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

  loop_lbl := NewLabel;
  done_lbl := NewLabel;

  { Load string length }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  { x3 = len }

  { idx = start }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitSturX0(-40);

  EmitLabel(loop_lbl);
  { if idx + count > len, done }
  EmitLdurX0(-40);  { idx }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
  writechar(76); write(done_lbl);
  EmitNL;

  { Copy char from idx+count to idx }
  EmitLdurX0(-8);  { string addr }
  EmitLdurX0(-40);  { idx }
  { RELOAD string addr since we overwrote x0 }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  { x3 = idx + count (source position) }
  { Load char from string[idx+count] }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(51); writechar(93);  { x3] }
  EmitNL;
  { Store to string[idx] }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(48); writechar(93);  { x0] }
  EmitNL;

  { idx++ }
  EmitLdurX0(-40);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitSturX0(-40);
  EmitBranchLabel(loop_lbl);

  EmitLabel(done_lbl);
  { Update string length: new_len = old_len - count }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitLdurX0(-8);
  { RELOAD x0 for store since we overwrote it }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;

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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;

  loop1_lbl := NewLabel;
  loop2_lbl := NewLabel;
  copy_lbl := NewLabel;
  done_lbl := NewLabel;

  { Load lengths }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;  { src_len }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);  { #-40] }
  EmitNL;  { dst_len }

  { Move existing chars from pos to end, shifting right by src_len }
  { idx = dst_len }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(56); writechar(93);  { #-48] }
  EmitNL;

  EmitLabel(loop1_lbl);
  { if idx < pos, done moving }
  EmitLdurX0(-48);  { idx }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(loop2_lbl);
  EmitNL;

  { Copy dest[idx] to dest[idx+src_len] }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(48); writechar(93);  { x0] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(54); writechar(93);  { x6] }
  EmitNL;

  { idx-- }
  EmitLdurX0(-48);
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
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
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;

  { Copy source[idx+1] to dest[pos+idx] }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(55); writechar(44); writechar(32);  { w7, }
  writechar(91); writechar(120); writechar(53); writechar(44); writechar(32);  { [x5, }
  writechar(120); writechar(54); writechar(93);  { x6] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(50); writechar(52); writechar(93);  { #-24] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(55); writechar(44); writechar(32);  { w7, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(54); writechar(93);  { x6] }
  EmitNL;

  { idx++ }
  EmitLdurX0(-48);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitSturX0(-48);
  EmitBranchLabel(copy_lbl);

  EmitLabel(done_lbl);
  { Update dest length: new_len = dst_len + src_len }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(51); writechar(50); writechar(93);  { #-32] }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(52); writechar(48); writechar(93);  { #-40] }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(52);  { x4 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;

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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { x2 = digit count, x3 = is_negative }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { Check if value is negative }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(neg_lbl);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(pos_lbl);
  EmitNL;

  { Handle negative }
  EmitLabel(neg_lbl);
  EmitIndent;
  writechar(110); writechar(101); writechar(103); writechar(32);  { neg }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Loop to extract digits (stored in reverse on stack) }
  EmitLabel(pos_lbl);
  EmitLabel(loop_lbl);
  { x4 = x0 / 10, x5 = x0 mod 10 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(117); writechar(100); writechar(105); writechar(118); writechar(32);  { udiv }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(54);  { x6 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(115); writechar(117); writechar(98); writechar(32);  { msub }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  { Convert digit to ASCII and store on stack }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;

  { Store digit at sp + x2 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);  { [sp, }
  writechar(120); writechar(50); writechar(93);  { x2] }
  EmitNL;

  { Increment digit count }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { x0 = x4 (quotient), continue if x0 > 0 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(52);  { x4 }
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne }
  writechar(76); write(loop_lbl);
  EmitNL;

  { Now reverse digits into destination string }
  { x1 = dest string, x4 = write position (starts at 1 if no sign, 2 if negative) }
  EmitIndent;
  writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(49); writechar(54); writechar(93);  { #-16] }
  EmitNL;

  { If negative, store '-' at position 1, start digits at 2 }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(rev_lbl);
  EmitNL;

  { Store '-' sign }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(35); writechar(52); writechar(53);  { #45 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(35); writechar(49); writechar(93);  { #1] }
  EmitNL;

  { x4 = write position (1 for positive, 2 for negative start) }
  EmitLabel(rev_lbl);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { x5 = read position (digit count - 1, reading backwards) }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Reverse copy loop }
  EmitLabel(rev_done_lbl);
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(done_lbl);
  EmitNL;

  { Load digit from stack at position x5 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(54); writechar(44); writechar(32);  { w6, }
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);  { [sp, }
  writechar(120); writechar(53); writechar(93);  { x5] }
  EmitNL;

  { Store at dest position x4 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(54); writechar(44); writechar(32);  { w6, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(52); writechar(93);  { x4] }
  EmitNL;

  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(rev_done_lbl);
  EmitNL;

  { Store length (x2 + x3 = digit count + sign) }
  EmitLabel(done_lbl);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;

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
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(49); writechar(44); writechar(32);  { w1, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { Check if empty string }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(error_lbl);
  EmitNL;

  { Check first char for sign }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(35); writechar(49); writechar(93);  { #1] }
  EmitNL;

  { Check for '-' (45) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(52); writechar(53);  { #45 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(neg_lbl);
  EmitNL;

  { Check for '+' (43) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(52); writechar(51);  { #43 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(pos_lbl);
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(loop_lbl);
  EmitNL;

  EmitLabel(neg_lbl);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitLabel(pos_lbl);
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Main loop }
  EmitLabel(loop_lbl);
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(49);  { x1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
  writechar(76); write(done_lbl);
  EmitNL;

  { Load char at position x2 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(50); writechar(93);  { x2] }
  EmitNL;

  { Check if digit (48-57) }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76); write(error_lbl);
  EmitNL;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(53); writechar(55);  { #57 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(116); writechar(32);  { b.gt }
  writechar(76); write(error_lbl);
  EmitNL;

  { result = result * 10 + digit }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(54);  { x6 }
  EmitNL;
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(53);  { x5 }
  EmitNL;

  { x2 := x2 + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(loop_lbl);
  EmitNL;

  { Success }
  EmitLabel(done_lbl);
  { Apply sign if negative }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(skip_sign_lbl);
  EmitNL;
  EmitIndent;
  writechar(110); writechar(101); writechar(103); writechar(32);  { neg }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;

  EmitLabel(skip_sign_lbl);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(exit_lbl);
  EmitNL;

  { Error - return position in x1 }
  EmitLabel(error_lbl);
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;

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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  { x2 = source length }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;

  { x3 = 0 (skip count) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { Loop: skip leading whitespace }
  EmitLabel(loop_lbl);
  { if x3 >= x2, done }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(done_lbl);
  EmitNL;

  { x4 = char at [x1 + x3 + 1] }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(52); writechar(93);  { x4] }
  EmitNL;

  { if char = 32 (space), skip }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(51); writechar(50);  { #32 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(label_count);
  EmitNL;

  { if char = 9 (tab), skip }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(57);  { #9 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne }
  writechar(76); write(done_lbl);
  EmitNL;

  EmitLabel(label_count);
  label_count := label_count + 1;

  { x3 := x3 + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(loop_lbl);
  EmitNL;

  { Done scanning: x3 = number to skip, x2 = length }
  EmitLabel(done_lbl);

  { Allocate new string from heap: x0 = x21 }
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

  { x5 = new length = x2 - x3 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;

  { Store new length }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(53); writechar(44); writechar(32);  { w5, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;

  { x6 = copy index (0 to x5-1) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { Copy loop }
  EmitLabel(copy_lbl);
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(53);  { x5 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(copy_done_lbl);
  EmitNL;

  { x7 = x3 + x6 + 1 (source index) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(54);  { x6 }
  EmitNL;
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load char }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(56); writechar(44); writechar(32);  { w8, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(55); writechar(93);  { x7] }
  EmitNL;

  { x7 = x6 + 1 (dest index) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(55); writechar(44); writechar(32);  { x7, }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Store char }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(56); writechar(44); writechar(32);  { w8, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(55); writechar(93);  { x7] }
  EmitNL;

  { x6 := x6 + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(copy_lbl);
  EmitNL;

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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  { x2 = source length }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(50); writechar(44); writechar(32);  { w2, }
  writechar(91); writechar(120); writechar(49); writechar(93);  { [x1] }
  EmitNL;

  { x3 = x2 (scan from end) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(50);  { x2 }
  EmitNL;

  { Loop: find last non-whitespace }
  EmitLabel(loop_lbl);
  { if x3 <= 0, done }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(101); writechar(32);  { b.le }
  writechar(76); write(done_lbl);
  EmitNL;

  { x4 = char at [x1 + x3] }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(52); writechar(44); writechar(32);  { w4, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(51); writechar(93);  { x3] }
  EmitNL;

  { if char = 32 (space), continue }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(51); writechar(50);  { #32 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(101); writechar(113); writechar(32);  { b.eq }
  writechar(76); write(label_count);
  EmitNL;

  { if char = 9 (tab), continue }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(35); writechar(57);  { #9 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(110); writechar(101); writechar(32);  { b.ne }
  writechar(76); write(done_lbl);
  EmitNL;

  EmitLabel(label_count);
  label_count := label_count + 1;

  { x3 := x3 - 1 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(loop_lbl);
  EmitNL;

  { Done: x3 = new length }
  EmitLabel(done_lbl);

  { Allocate new string from heap }
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

  { Store new length }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(51); writechar(44); writechar(32);  { w3, }
  writechar(91); writechar(120); writechar(48); writechar(93);  { [x0] }
  EmitNL;

  { x5 = copy index (0 to x3-1) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { Copy loop }
  EmitLabel(copy_lbl);
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(51);  { x3 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(103); writechar(101); writechar(32);  { b.ge }
  writechar(76); write(copy_done_lbl);
  EmitNL;

  { x6 = x5 + 1 (index) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(54); writechar(44); writechar(32);  { x6, }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;

  { Load char from source }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(98); writechar(32);  { ldrb }
  writechar(119); writechar(55); writechar(44); writechar(32);  { w7, }
  writechar(91); writechar(120); writechar(49); writechar(44); writechar(32);  { [x1, }
  writechar(120); writechar(54); writechar(93);  { x6] }
  EmitNL;

  { Store char to dest }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(55); writechar(44); writechar(32);  { w7, }
  writechar(91); writechar(120); writechar(48); writechar(44); writechar(32);  { [x0, }
  writechar(120); writechar(54); writechar(93);  { x6] }
  EmitNL;

  { x5 := x5 + 1 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(120); writechar(53); writechar(44); writechar(32);  { x5, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(32);  { b }
  writechar(76); write(copy_lbl);
  EmitNL;

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
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114);  { stur }
  writechar(98); writechar(32);  { b }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(50); writechar(57);  { [x29 }
  writechar(44); writechar(32); writechar(35);  { , # }
  write(offset);
  writechar(93);  { ] }
  EmitNL
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(53);  { #15 }
  EmitNL;
  { mov x2, #7 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(55);  { #7 }
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(49); writechar(44); writechar(32);  { x11, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(50); writechar(44); writechar(32);  { x12, }
  writechar(120); writechar(49);  { x1 }
  EmitNL;
  { Build escape sequence at stack offset -20 onwards (leave room for digits) }
  { Format: ESC [ y ; x H }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-20);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-19);
  { x8 = pointer to next byte (start at -18) }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(56);  { #18 }
  EmitNL;
  { Convert y to decimal digits (y is in x11) }
  { mov x0, x11 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(49);  { x11 }
  EmitNL;
  { If y >= 10, output tens digit }
  loop1 := NewLabel;
  done1 := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76);
  write(done1);
  EmitNL;
  { Divide by 10: x1 = x0 / 10, x2 = x0 mod 10 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(117); writechar(100); writechar(105); writechar(118); writechar(32);  { udiv }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(57);  { x9 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(115); writechar(117); writechar(98); writechar(32);  { msub }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  { x1=tens, x0=units - store tens digit }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  { strb w1, [x8], #1 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(49); writechar(44); writechar(32);  { w1, }
  writechar(91); writechar(120); writechar(56); writechar(93); writechar(44); writechar(32);  { [x8], }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitLabel(done1);
  { Store units digit }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  { strb w0, [x8], #1 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(56); writechar(93); writechar(44); writechar(32);  { [x8], }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  { Store semicolon }
  EmitMovX0(59);  { ; }
  { strb w0, [x8], #1 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(56); writechar(93); writechar(44); writechar(32);  { [x8], }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  { Convert x to decimal digits (x is in x12) }
  { mov x0, x12 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(50);  { x12 }
  EmitNL;
  done2 := NewLabel;
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);  { cmp }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(98); writechar(46); writechar(108); writechar(116); writechar(32);  { b.lt }
  writechar(76);
  write(done2);
  EmitNL;
  { Divide by 10 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(35); writechar(49); writechar(48);  { #10 }
  EmitNL;
  EmitIndent;
  writechar(117); writechar(100); writechar(105); writechar(118); writechar(32);  { udiv }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(57);  { x9 }
  EmitNL;
  EmitIndent;
  writechar(109); writechar(115); writechar(117); writechar(98); writechar(32);  { msub }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  { Store tens digit }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  { strb w1, [x8], #1 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(49); writechar(44); writechar(32);  { w1, }
  writechar(91); writechar(120); writechar(56); writechar(93); writechar(44); writechar(32);  { [x8], }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  EmitLabel(done2);
  { Store units digit }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  { strb w0, [x8], #1 }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(56); writechar(93); writechar(44); writechar(32);  { [x8], }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  { Store H terminator }
  EmitMovX0(72);  { H }
  { strb w0, [x8] }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(98); writechar(32);  { strb }
  writechar(119); writechar(48); writechar(44); writechar(32);  { w0, }
  writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
  EmitNL;
  { Calculate length: x8 points past last byte, buffer starts at x29-20 }
  { add x8, x8, #1 (point past H) }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(35); writechar(49);  { #1 }
  EmitNL;
  { x2 = x8 - (x29-20) = x8 - x29 + 20 }
  { sub x2, x8, x29 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { add x2, x2, #20 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(50); writechar(48);  { #20 }
  EmitNL;
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #20 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(50); writechar(48);  { #20 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(49);  { #11 }
  EmitNL;
  { mov x2, #3 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(51);  { #3 }
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  { Store ESC[3Xm = 27,91,51,X,109 (5 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-13);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-12);
  EmitMovX0(51);  { 3 }
  EmitStrbAtOffset(-11);
  { Store color digit: x9 + 48 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  EmitStrbAtOffset(-10);
  { Store 'm' }
  EmitMovX0(109);  { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #13 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(51);  { #13 }
  EmitNL;
  { mov x2, #5 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(53);  { #5 }
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;
  { Store ESC[4Xm = 27,91,52,X,109 (5 bytes) }
  EmitMovX0(27);  { ESC }
  EmitStrbAtOffset(-13);
  EmitMovX0(91);  { [ }
  EmitStrbAtOffset(-12);
  EmitMovX0(52);  { 4 }
  EmitStrbAtOffset(-11);
  { Store color digit: x9 + 48 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);  { add }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(35); writechar(52); writechar(56);  { #48 }
  EmitNL;
  EmitStrbAtOffset(-10);
  { Store 'm' }
  EmitMovX0(109);  { m }
  EmitStrbAtOffset(-9);
  { Write syscall }
  EmitMovX16(33554436);  { 0x2000004 = write }
  EmitMovX0X20;          { fd from x20 }
  { sub x1, x29, #13 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(51);  { #13 }
  EmitNL;
  { mov x2, #5 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(53);  { #5 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(50);  { #12 }
  EmitNL;
  { mov x2, #4 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(52);  { #4 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(50);  { #12 }
  EmitNL;
  { mov x2, #4 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(52);  { #4 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(50);  { #12 }
  EmitNL;
  { mov x2, #4 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(52);  { #4 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(52);  { #14 }
  EmitNL;
  { mov x2, #6 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(54);  { #6 }
  EmitNL;
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
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
  writechar(35); writechar(49); writechar(52);  { #14 }
  EmitNL;
  { mov x2, #6 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(54);  { #6 }
  EmitNL;
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
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(48); writechar(44); writechar(32);  { x10, }
  writechar(120); writechar(48);  { x0 }
  EmitNL;

  { x11 = 1000 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(49); writechar(44); writechar(32);  { x11, }
  writechar(35); writechar(49); writechar(48); writechar(48); writechar(48);  { #1000 }
  EmitNL;

  { x0 = ms / 1000 (seconds) }
  EmitIndent;
  writechar(117); writechar(100); writechar(105); writechar(118); writechar(32);  { udiv }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(48); writechar(44); writechar(32);  { x10, }
  writechar(120); writechar(49); writechar(49);  { x11 }
  EmitNL;

  { x1 = ms mod 1000 (remainder): msub x1, x0, x11, x10 = x10 - (x0 * x11) }
  EmitIndent;
  writechar(109); writechar(115); writechar(117); writechar(98); writechar(32);  { msub }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(120); writechar(49); writechar(49); writechar(44); writechar(32);  { x11, }
  writechar(120); writechar(49); writechar(48);  { x10 }
  EmitNL;

  { x1 = x1 * 1000 (microseconds, not nanoseconds for timeval) }
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);  { mul }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(120); writechar(49); writechar(49);  { x11 (x11 is still 1000) }
  EmitNL;

  { Store timeval at sp: [sp] = tv_sec, [sp+8] = tv_usec }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(32);  { str }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(91); writechar(115); writechar(112); writechar(93);  { [sp] }
  EmitNL;

  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(32);  { str }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);  { [sp, }
  writechar(35); writechar(56); writechar(93);  { #8] }
  EmitNL;

  { select(nfds=0, readfds=NULL, writefds=NULL, exceptfds=NULL, timeout=sp) }
  { x0 = 0 (nfds) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { x1 = NULL }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(49); writechar(44); writechar(32);  { x1, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { x2 = NULL }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(50); writechar(44); writechar(32);  { x2, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { x3 = NULL }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(51); writechar(44); writechar(32);  { x3, }
  writechar(35); writechar(48);  { #0 }
  EmitNL;

  { x4 = pointer to timeval (sp) }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(52); writechar(44); writechar(32);  { x4, }
  writechar(115); writechar(112);  { sp }
  EmitNL;

  { x16 = syscall number for select: 0x2000000 + 93 = 33554525 }
  EmitMovX16(33554525);
  EmitSvc;

  EmitAddSP(32);
  EmitLdp;
  EmitRet
end;

