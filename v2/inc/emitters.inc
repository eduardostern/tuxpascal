{ ----- Output Helpers ----- }

procedure EmitIndent;
begin
  writechar(32);
  writechar(32);
  writechar(32);
  writechar(32)
end;

procedure EmitNL;
begin
  writeln
end;

{ Emit specific strings character by character }
procedure EmitGlobl;
begin
  { .globl _main }
  writechar(46); writechar(103); writechar(108); writechar(111);
  writechar(98); writechar(108); writechar(32);
  writechar(95); writechar(109); writechar(97); writechar(105); writechar(110);
  EmitNL
end;

procedure EmitAlign4;
begin
  { .align 4 }
  writechar(46); writechar(97); writechar(108); writechar(105);
  writechar(103); writechar(110); writechar(32); writechar(52);
  EmitNL
end;

procedure EmitMain;
begin
  { _main: }
  writechar(95); writechar(109); writechar(97); writechar(105); writechar(110);
  writechar(58);
  EmitNL
end;

procedure EmitLabel(n: integer);
begin
  writechar(76);  { L }
  write(n);
  writechar(58);  { : }
  EmitNL
end;

function NewLabel: integer;
begin
  NewLabel := label_count;
  label_count := label_count + 1
end;

procedure EmitStp;
begin
  { stp x29, x30, [sp, #-16]! }
  EmitIndent;
  writechar(115); writechar(116); writechar(112); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(120); writechar(51); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49); writechar(54);
  writechar(93); writechar(33);
  EmitNL
end;

procedure EmitMovFP;
begin
  { mov x29, sp }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(115); writechar(112);
  EmitNL
end;

procedure EmitLdp;
begin
  { ldp x29, x30, [sp], #16 }
  EmitIndent;
  writechar(108); writechar(100); writechar(112); writechar(32);
  writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);
  writechar(120); writechar(51); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);
  EmitNL
end;

procedure EmitRet;
begin
  { ret }
  EmitIndent;
  writechar(114); writechar(101); writechar(116);
  EmitNL
end;

procedure EmitStoreStaticLink;
begin
  { stur x9, [x29, #-8] }
  EmitIndent;
  writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(91); writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { [x29, }
  writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
  EmitNL
end;

{ Emit code to set up static link in x9 before a call }
{ The callee was declared at sym_level, so its static link should point to frame at sym_level }
procedure EmitStaticLink(sym_level, cur_level: integer);
var
  i: integer;
begin
  { mov x9, x29 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { Follow static link chain to reach sym_level }
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(91); writechar(120); writechar(57); writechar(44); writechar(32);  { [x9, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end
end;

procedure EmitMovX0(val: integer);
var
  lo, hi: integer;
  neg: integer;
begin
  neg := 0;
  if val < 0 then
  begin
    neg := 1;
    val := 0 - val
  end;
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    { movz x0, #lo }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(35);
    write(lo);
    EmitNL;
    { movk x0, #hi, lsl #16 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(35);
    write(hi);
    writechar(44); writechar(32);
    writechar(108); writechar(115); writechar(108); writechar(32);
    writechar(35); writechar(49); writechar(54);
    EmitNL
  end
  else
  begin
    { mov x0, #val }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(35);
    write(val);
    EmitNL
  end;
  if neg = 1 then
  begin
    { neg x0, x0 }
    EmitIndent;
    writechar(110); writechar(101); writechar(103); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(120); writechar(48);
    EmitNL
  end
end;

procedure EmitMovX16(val: integer);
var
  lo, hi: integer;
begin
  { For large values like syscall numbers (0x2000001, etc), use movz+movk }
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    { movz x16, #lo }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);
    writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);
    writechar(35);
    write(lo);
    EmitNL;
    { movk x16, #hi, lsl #16 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);
    writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);
    writechar(35);
    write(hi);
    writechar(44); writechar(32);
    writechar(108); writechar(115); writechar(108); writechar(32);
    writechar(35); writechar(49); writechar(54);
    EmitNL
  end
  else
  begin
    { mov x16, #val }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);
    writechar(120); writechar(49); writechar(54); writechar(44); writechar(32);
    writechar(35);
    write(val);
    EmitNL
  end
end;

procedure EmitMovX8(val: integer);
var
  lo, hi: integer;
  neg: integer;
begin
  { Handle negative values: negate, emit, then negate result }
  neg := 0;
  if val < 0 then
  begin
    neg := 1;
    val := 0 - val
  end;
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    { movz x8, #lo }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(35);
    write(lo);
    EmitNL;
    { movk x8, #hi, lsl #16 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(35);
    write(hi);
    writechar(44); writechar(32);
    writechar(108); writechar(115); writechar(108); writechar(32);
    writechar(35); writechar(49); writechar(54);
    EmitNL
  end
  else
  begin
    { mov x8, #val }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(35);
    write(val);
    EmitNL
  end;
  if neg = 1 then
  begin
    { neg x8, x8 }
    EmitIndent;
    writechar(110); writechar(101); writechar(103); writechar(32);
    writechar(120); writechar(56); writechar(44); writechar(32);
    writechar(120); writechar(56);
    EmitNL
  end
end;

{ Emit sub xD, xS, #offset where offset can be large }
{ If offset > 4095, loads into x10 first }
procedure EmitSubLargeOffset(dest, src, offset: integer);
var
  lo, hi: integer;
begin
  if offset <= 4095 then
  begin
    { sub xD, xS, #offset }
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
    writechar(120);
    if dest < 10 then
      writechar(48 + dest)
    else
    begin
      writechar(49);
      writechar(48 + dest - 10)
    end;
    writechar(44); writechar(32);
    writechar(120);
    if src < 10 then
      writechar(48 + src)
    else
    begin
      if src = 29 then
      begin
        writechar(50); writechar(57)
      end
      else
      begin
        writechar(49);
        writechar(48 + src - 10)
      end
    end;
    writechar(44); writechar(32);
    writechar(35);
    write(offset);
    EmitNL
  end
  else
  begin
    { Load offset into x10 }
    lo := offset mod 65536;
    hi := offset div 65536;
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(122); writechar(32);  { movz }
    writechar(120); writechar(49); writechar(48); writechar(44); writechar(32);  { x10, }
    writechar(35);
    write(lo);
    EmitNL;
    if hi > 0 then
    begin
      EmitIndent;
      writechar(109); writechar(111); writechar(118); writechar(107); writechar(32);  { movk }
      writechar(120); writechar(49); writechar(48); writechar(44); writechar(32);  { x10, }
      writechar(35);
      write(hi);
      writechar(44); writechar(32);
      writechar(108); writechar(115); writechar(108); writechar(32);  { lsl }
      writechar(35); writechar(49); writechar(54);  { #16 }
      EmitNL
    end;
    { sub xD, xS, x10 }
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
    writechar(120);
    if dest < 10 then
      writechar(48 + dest)
    else
    begin
      writechar(49);
      writechar(48 + dest - 10)
    end;
    writechar(44); writechar(32);
    writechar(120);
    if src < 10 then
      writechar(48 + src)
    else
    begin
      if src = 29 then
      begin
        writechar(50); writechar(57)
      end
      else
      begin
        writechar(49);
        writechar(48 + src - 10)
      end
    end;
    writechar(44); writechar(32);
    writechar(120); writechar(49); writechar(48);  { x10 }
    EmitNL
  end
end;

procedure EmitSvc;
begin
  { svc #0x80 }
  EmitIndent;
  writechar(115); writechar(118); writechar(99); writechar(32);
  writechar(35); writechar(48); writechar(120); writechar(56); writechar(48);
  EmitNL
end;

procedure EmitPushX0;
begin
  { str x0, [sp, #-16]! }
  EmitIndent;
  writechar(115); writechar(116); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(44); writechar(32);
  writechar(35); writechar(45); writechar(49); writechar(54);
  writechar(93); writechar(33);
  EmitNL
end;

procedure EmitPopX0;
begin
  { ldr x0, [sp], #16 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);
  EmitNL
end;

procedure EmitPopX1;
begin
  { ldr x1, [sp], #16 }
  EmitIndent;
  writechar(108); writechar(100); writechar(114); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(91); writechar(115); writechar(112); writechar(93);
  writechar(44); writechar(32); writechar(35); writechar(49); writechar(54);
  EmitNL
end;

procedure EmitAdd;
begin
  { add x0, x1, x0 }
  EmitIndent;
  writechar(97); writechar(100); writechar(100); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitSub;
begin
  { sub x0, x1, x0 }
  EmitIndent;
  writechar(115); writechar(117); writechar(98); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitMul;
begin
  { mul x0, x1, x0 }
  EmitIndent;
  writechar(109); writechar(117); writechar(108); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitSDiv;
begin
  { sdiv x0, x1, x0 }
  EmitIndent;
  writechar(115); writechar(100); writechar(105); writechar(118); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitMovX0X20;
begin
  { mov x0, x20 - use output file descriptor }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(48); writechar(44); writechar(32);     { x0, }
  writechar(120); writechar(50); writechar(48);                    { x20 }
  EmitNL
end;

procedure EmitBranchLabel(lbl: integer);
begin
  { b Lxx }
  EmitIndent;
  writechar(98); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitBranchLabelZ(lbl: integer);
begin
  { cbz x0, Lxx }
  EmitIndent;
  writechar(99); writechar(98); writechar(122); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitBranchLabelNZ(lbl: integer);
begin
  { cbnz x0, Lxx }
  EmitIndent;
  writechar(99); writechar(98); writechar(110); writechar(122); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitBL(lbl: integer);
begin
  { bl Lxx }
  EmitIndent;
  writechar(98); writechar(108); writechar(32);
  writechar(76);
  write(lbl);
  EmitNL
end;

procedure EmitCmpX0X1;
begin
  { cmp x0, x1 - actually cmp x1, x0 for our stack order }
  EmitIndent;
  writechar(99); writechar(109); writechar(112); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitCset(cond: integer);
begin
  { cset x0, <cond> }
  { cond: 0=eq, 1=ne, 2=lt, 3=le, 4=gt, 5=ge }
  EmitIndent;
  writechar(99); writechar(115); writechar(101); writechar(116); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  if cond = 0 then
  begin writechar(101); writechar(113) end  { eq }
  else if cond = 1 then
  begin writechar(110); writechar(101) end  { ne }
  else if cond = 2 then
  begin writechar(108); writechar(116) end  { lt }
  else if cond = 3 then
  begin writechar(108); writechar(101) end  { le }
  else if cond = 4 then
  begin writechar(103); writechar(116) end  { gt }
  else
  begin writechar(103); writechar(101) end; { ge }
  EmitNL
end;

procedure EmitLdurX0(offset: integer);
begin
  if (offset >= -255) and (offset <= 255) then
  begin
    { ldur x0, [x29, #offset] }
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(91); writechar(120); writechar(50); writechar(57);
    writechar(44); writechar(32); writechar(35);
    write(offset);
    writechar(93);
    EmitNL
  end
  else
  begin
    { Large offset: mov x8, #offset; add x8, x29, x8; ldr x0, [x8] }
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitIndent;
    writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

{ Load from outer scope - follow static link chain }
{ Static link is stored at [frame, #-8] }
procedure EmitLdurX0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  { Start with current frame pointer }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { Follow static link chain }
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end;
  { Now x8 points to the target frame }
  if (offset >= -255) and (offset <= 255) then
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56);  { [x8 }
    writechar(44); writechar(32); writechar(35);  { , # }
    write(offset);
    writechar(93);  { ] }
    EmitNL
  end
  else
  begin
    { Large offset: save x8 to x9, load offset to x8, add them }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitIndent;
    writechar(108); writechar(100); writechar(114); writechar(32);  { ldr }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

{ Follow static link chain, leave target frame in x8 }
{ Static link is stored at [frame, #-8] }
procedure EmitFollowChain(sym_level, cur_level: integer);
var
  i: integer;
begin
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end
end;

{ Emit code to compute address of a variable into x0 }
{ For use with var parameters - caller passes address }
procedure EmitVarAddr(var_idx, cur_scope: integer);
var
  offset, var_level, i: integer;
begin
  offset := sym_offset[var_idx];
  var_level := sym_level[var_idx];

  if var_level < cur_scope then
  begin
    { Variable is in outer scope - follow static chain to x8 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(50); writechar(57);  { x29 }
    EmitNL;
    for i := cur_scope downto var_level + 1 do
    begin
      EmitIndent;
      writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
      writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
      writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
      writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
      EmitNL
    end;
    { Now x8 has target frame, compute address: add x0, x8, #offset }
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(35);
    write(offset);
    EmitNL
  end
  else
  begin
    { Variable is in current scope - add x0, x29, #offset }
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
    writechar(35);
    write(offset);
    EmitNL
  end
end;

{ Store to outer scope - follow saved frame pointer chain }
procedure EmitSturX0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  { Save x0 temporarily }
  EmitPushX0;
  { Start with current frame pointer }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
  writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
  writechar(120); writechar(50); writechar(57);  { x29 }
  EmitNL;
  { Follow static link chain }
  for i := cur_level downto sym_level + 1 do
  begin
    EmitIndent;
    writechar(108); writechar(100); writechar(117); writechar(114); writechar(32);  { ldur }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(91); writechar(120); writechar(56); writechar(44); writechar(32);  { [x8, }
    writechar(35); writechar(45); writechar(56); writechar(93);  { #-8] }
    EmitNL
  end;
  { Restore x0 }
  EmitPopX0;
  { Now x8 points to the target frame }
  if (offset >= -255) and (offset <= 255) then
  begin
    EmitIndent;
    writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);  { stur }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56);  { [x8 }
    writechar(44); writechar(32); writechar(35);  { , # }
    write(offset);
    writechar(93);  { ] }
    EmitNL
  end
  else
  begin
    { Large offset: save x8 to x9, x0 to stack, load offset to x8, add, restore x0 }
    EmitIndent;
    writechar(109); writechar(111); writechar(118); writechar(32);  { mov }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitPushX0;
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(57); writechar(44); writechar(32);  { x9, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitPopX0;
    EmitIndent;
    writechar(115); writechar(116); writechar(114); writechar(32);  { str }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

procedure EmitSturX0(offset: integer);
begin
  if (offset >= -255) and (offset <= 255) then
  begin
    { stur x0, [x29, #offset] }
    EmitIndent;
    writechar(115); writechar(116); writechar(117); writechar(114); writechar(32);
    writechar(120); writechar(48); writechar(44); writechar(32);
    writechar(91); writechar(120); writechar(50); writechar(57);
    writechar(44); writechar(32); writechar(35);
    write(offset);
    writechar(93);
    EmitNL
  end
  else
  begin
    { Large offset: mov x8, #offset; add x8, x29, x8; str x0, [x8] }
    EmitMovX8(offset);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(120); writechar(56); writechar(44); writechar(32);  { x8, }
    writechar(120); writechar(50); writechar(57); writechar(44); writechar(32);  { x29, }
    writechar(120); writechar(56);  { x8 }
    EmitNL;
    EmitIndent;
    writechar(115); writechar(116); writechar(114); writechar(32);  { str }
    writechar(120); writechar(48); writechar(44); writechar(32);  { x0, }
    writechar(91); writechar(120); writechar(56); writechar(93);  { [x8] }
    EmitNL
  end
end;

procedure EmitSubSP(n: integer);
begin
  if n <= 4095 then
  begin
    { sub sp, sp, #n }
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(35);
    write(n);
    EmitNL
  end
  else
  begin
    { Large value: mov x8, #n; sub sp, sp, x8 }
    EmitMovX8(n);
    EmitIndent;
    writechar(115); writechar(117); writechar(98); writechar(32);  { sub }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(120); writechar(56);  { x8 }
    EmitNL
  end
end;

procedure EmitAddSP(n: integer);
begin
  if n <= 4095 then
  begin
    { add sp, sp, #n }
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(115); writechar(112); writechar(44); writechar(32);
    writechar(35);
    write(n);
    EmitNL
  end
  else
  begin
    { Large value: mov x8, #n; add sp, sp, x8 }
    EmitMovX8(n);
    EmitIndent;
    writechar(97); writechar(100); writechar(100); writechar(32);  { add }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(115); writechar(112); writechar(44); writechar(32);  { sp, }
    writechar(120); writechar(56);  { x8 }
    EmitNL
  end
end;

procedure EmitNeg;
begin
  { neg x0, x0 }
  EmitIndent;
  writechar(110); writechar(101); writechar(103); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitMsub;
begin
  { msub x0, x0, x2, x1   (x0 = x1 - x0 * x2) for mod }
  EmitIndent;
  writechar(109); writechar(115); writechar(117); writechar(98); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(49);
  EmitNL
end;

procedure EmitMovX2X0;
begin
  { mov x2, x0 }
  EmitIndent;
  writechar(109); writechar(111); writechar(118); writechar(32);
  writechar(120); writechar(50); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitAndX0X1;
begin
  { and x0, x1, x0 }
  EmitIndent;
  writechar(97); writechar(110); writechar(100); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitOrrX0X1;
begin
  { orr x0, x1, x0 }
  EmitIndent;
  writechar(111); writechar(114); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(49); writechar(44); writechar(32);
  writechar(120); writechar(48);
  EmitNL
end;

procedure EmitEorX0(val: integer);
begin
  { eor x0, x0, #val }
  EmitIndent;
  writechar(101); writechar(111); writechar(114); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(120); writechar(48); writechar(44); writechar(32);
  writechar(35);
  write(val);
  EmitNL
end;

