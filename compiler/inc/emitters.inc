{ ----- Output Helpers ----- }

procedure EmitIndent;
begin
  write('    ')
end;

procedure EmitNL;
begin
  writeln
end;

procedure EmitGlobl;
begin
  writeln('.globl _main')
end;

procedure EmitAlign4;
begin
  writeln('.align 4')
end;

procedure EmitMain;
begin
  writeln('_main:')
end;

procedure EmitLabel(n: integer);
begin
  write('L'); write(n); writeln(':')
end;

function NewLabel: integer;
begin
  NewLabel := label_count;
  label_count := label_count + 1
end;

procedure EmitStp;
begin
  writeln('    stp x29, x30, [sp, #-16]!')
end;

procedure EmitMovFP;
begin
  writeln('    mov x29, sp')
end;

procedure EmitLdp;
begin
  writeln('    ldp x29, x30, [sp], #16')
end;

procedure EmitRet;
begin
  writeln('    ret')
end;

procedure EmitStoreStaticLink;
begin
  writeln('    stur x9, [x29, #-8]')
end;

procedure EmitStaticLink(sym_level, cur_level: integer);
var
  i: integer;
begin
  writeln('    mov x9, x29');
  for i := cur_level downto sym_level + 1 do
    writeln('    ldur x9, [x9, #-8]')
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
    write('    movz x0, #'); writeln(lo);
    write('    movk x0, #'); write(hi); writeln(', lsl #16')
  end
  else
  begin
    write('    mov x0, #'); writeln(val)
  end;
  if neg = 1 then
    writeln('    neg x0, x0')
end;

procedure EmitMovX16(val: integer);
var
  lo, hi: integer;
begin
  if val > 65535 then
  begin
    lo := val mod 65536;
    hi := val div 65536;
    write('    movz x16, #'); writeln(lo);
    write('    movk x16, #'); write(hi); writeln(', lsl #16')
  end
  else
  begin
    write('    mov x16, #'); writeln(val)
  end
end;

procedure EmitMovX8(val: integer);
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
    write('    movz x8, #'); writeln(lo);
    write('    movk x8, #'); write(hi); writeln(', lsl #16')
  end
  else
  begin
    write('    mov x8, #'); writeln(val)
  end;
  if neg = 1 then
    writeln('    neg x8, x8')
end;

procedure EmitSubLargeOffset(dest, src, offset: integer);
var
  lo, hi: integer;
begin
  if offset <= 4095 then
  begin
    write('    sub x'); write(dest); write(', x'); write(src); write(', #'); writeln(offset)
  end
  else
  begin
    lo := offset mod 65536;
    hi := offset div 65536;
    write('    movz x10, #'); writeln(lo);
    if hi > 0 then
    begin
      write('    movk x10, #'); write(hi); writeln(', lsl #16')
    end;
    write('    sub x'); write(dest); write(', x'); write(src); writeln(', x10')
  end
end;

procedure EmitSvc;
begin
  writeln('    svc #0x80')
end;

procedure EmitPushX0;
begin
  writeln('    str x0, [sp, #-16]!')
end;

procedure EmitPopX0;
begin
  writeln('    ldr x0, [sp], #16')
end;

procedure EmitPopX1;
begin
  writeln('    ldr x1, [sp], #16')
end;

procedure EmitPushX1;
begin
  writeln('    str x1, [sp, #-16]!')
end;

procedure EmitAdd;
begin
  writeln('    add x0, x1, x0')
end;

procedure EmitSub;
begin
  writeln('    sub x0, x1, x0')
end;

procedure EmitMul;
begin
  writeln('    mul x0, x1, x0')
end;

procedure EmitSDiv;
begin
  writeln('    sdiv x0, x1, x0')
end;

procedure EmitMovX0X20;
begin
  writeln('    mov x0, x20')
end;

procedure EmitBranchLabel(lbl: integer);
begin
  write('    b L'); writeln(lbl)
end;

procedure EmitBranchLabelZ(lbl: integer);
begin
  write('    cbz x0, L'); writeln(lbl)
end;

procedure EmitBranchLabelNZ(lbl: integer);
begin
  write('    cbnz x0, L'); writeln(lbl)
end;

procedure EmitBL(lbl: integer);
begin
  write('    bl L'); writeln(lbl)
end;

procedure EmitCmpX0X1;
begin
  writeln('    cmp x1, x0')
end;

procedure EmitCset(cond: integer);
begin
  write('    cset x0, ');
  if cond = 0 then writeln('eq')
  else if cond = 1 then writeln('ne')
  else if cond = 2 then writeln('lt')
  else if cond = 3 then writeln('le')
  else if cond = 4 then writeln('gt')
  else writeln('ge')
end;

procedure EmitLdurX0(offset: integer);
begin
  if (offset >= -255) and (offset <= 255) then
  begin
    write('    ldur x0, [x29, #'); write(offset); writeln(']')
  end
  else
  begin
    EmitMovX8(offset);
    writeln('    add x8, x29, x8');
    writeln('    ldr x0, [x8]')
  end
end;

procedure EmitLdurX0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  writeln('    mov x8, x29');
  for i := cur_level downto sym_level + 1 do
    writeln('    ldur x8, [x8, #-8]');
  if (offset >= -255) and (offset <= 255) then
  begin
    write('    ldur x0, [x8, #'); write(offset); writeln(']')
  end
  else
  begin
    writeln('    mov x9, x8');
    EmitMovX8(offset);
    writeln('    add x8, x9, x8');
    writeln('    ldr x0, [x8]')
  end
end;

procedure EmitFollowChain(sym_level, cur_level: integer);
var
  i: integer;
begin
  writeln('    mov x8, x29');
  for i := cur_level downto sym_level + 1 do
    writeln('    ldur x8, [x8, #-8]')
end;

procedure EmitVarAddr(var_idx, cur_scope: integer);
var
  offset, var_level, i: integer;
begin
  offset := sym_offset[var_idx];
  var_level := sym_level[var_idx];

  if var_level < cur_scope then
  begin
    writeln('    mov x8, x29');
    for i := cur_scope downto var_level + 1 do
      writeln('    ldur x8, [x8, #-8]');
    if offset < 0 then
      EmitSubLargeOffset(0, 8, 0 - offset)
    else
    begin
      if offset <= 4095 then
      begin
        write('    add x0, x8, #'); writeln(offset)
      end
      else
      begin
        writeln('    mov x9, x8');
        EmitMovX8(offset);
        writeln('    add x0, x9, x8')
      end
    end
  end
  else
  begin
    if offset < 0 then
      EmitSubLargeOffset(0, 29, 0 - offset)
    else
    begin
      if offset <= 4095 then
      begin
        write('    add x0, x29, #'); writeln(offset)
      end
      else
      begin
        EmitMovX8(offset);
        writeln('    add x0, x29, x8')
      end
    end
  end
end;

procedure EmitSturX0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  EmitPushX0;
  writeln('    mov x8, x29');
  for i := cur_level downto sym_level + 1 do
    writeln('    ldur x8, [x8, #-8]');
  EmitPopX0;
  if (offset >= -255) and (offset <= 255) then
  begin
    write('    stur x0, [x8, #'); write(offset); writeln(']')
  end
  else
  begin
    writeln('    mov x9, x8');
    EmitPushX0;
    EmitMovX8(offset);
    writeln('    add x8, x9, x8');
    EmitPopX0;
    writeln('    str x0, [x8]')
  end
end;

procedure EmitSturX0(offset: integer);
begin
  if (offset >= -255) and (offset <= 255) then
  begin
    write('    stur x0, [x29, #'); write(offset); writeln(']')
  end
  else
  begin
    EmitMovX8(offset);
    writeln('    add x8, x29, x8');
    writeln('    str x0, [x8]')
  end
end;

procedure EmitSubSP(n: integer);
begin
  if n <= 4095 then
  begin
    write('    sub sp, sp, #'); writeln(n)
  end
  else
  begin
    EmitMovX8(n);
    writeln('    sub sp, sp, x8')
  end
end;

procedure EmitAddSP(n: integer);
begin
  if n <= 4095 then
  begin
    write('    add sp, sp, #'); writeln(n)
  end
  else
  begin
    EmitMovX8(n);
    writeln('    add sp, sp, x8')
  end
end;

procedure EmitNeg;
begin
  writeln('    neg x0, x0')
end;

procedure EmitMsub;
begin
  writeln('    msub x0, x0, x2, x1')
end;

procedure EmitMovX2X0;
begin
  writeln('    mov x2, x0')
end;

procedure EmitAndX0X1;
begin
  writeln('    and x0, x1, x0')
end;

procedure EmitOrrX0X1;
begin
  writeln('    orr x0, x1, x0')
end;

procedure EmitAndImm(val: integer);
begin
  write('    and x0, x0, #'); writeln(val)
end;

procedure EmitEorX0(val: integer);
begin
  write('    eor x0, x0, #'); writeln(val)
end;

{ ----- Floating Point Emitters ----- }

procedure EmitPushD0;
begin
  writeln('    str d0, [sp, #-16]!')
end;

procedure EmitPopD0;
begin
  writeln('    ldr d0, [sp], #16')
end;

procedure EmitPopD1;
begin
  writeln('    ldr d1, [sp], #16')
end;

procedure EmitFAdd;
begin
  writeln('    fadd d0, d1, d0')
end;

procedure EmitFSub;
begin
  writeln('    fsub d0, d1, d0')
end;

procedure EmitFMul;
begin
  writeln('    fmul d0, d1, d0')
end;

procedure EmitFDiv;
begin
  writeln('    fdiv d0, d1, d0')
end;

procedure EmitFNeg;
begin
  writeln('    fneg d0, d0')
end;

procedure EmitFCmp;
begin
  writeln('    fcmp d1, d0')
end;

procedure EmitScvtfD0X0;
begin
  writeln('    scvtf d0, x0')
end;

procedure EmitScvtfD1X1;
begin
  writeln('    scvtf d1, x1')
end;

procedure EmitFcvtzsX0D0;
begin
  writeln('    fcvtzs x0, d0')
end;

procedure EmitFmovD0X0;
begin
  writeln('    fmov d0, x0')
end;

procedure EmitFmovX0D0;
begin
  writeln('    fmov x0, d0')
end;

procedure EmitLdurD0(offset: integer);
begin
  write('    ldur d0, [x29, #'); write(offset); writeln(']')
end;

procedure EmitSturD0(offset: integer);
begin
  write('    stur d0, [x29, #'); write(offset); writeln(']')
end;

procedure EmitLdurD0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  writeln('    mov x8, x29');
  for i := cur_level downto sym_level + 1 do
    writeln('    ldur x8, [x8, #-8]');
  write('    ldur d0, [x8, #'); write(offset); writeln(']')
end;

procedure EmitSturD0Outer(offset, sym_level, cur_level: integer);
var
  i: integer;
begin
  EmitPushD0;
  writeln('    mov x8, x29');
  for i := cur_level downto sym_level + 1 do
    writeln('    ldur x8, [x8, #-8]');
  EmitPopD0;
  write('    stur d0, [x8, #'); write(offset); writeln(']')
end;

{ ----- Unit Symbol Emitters ----- }

procedure WriteSymName(sym_idx: integer);
var
  base, i, c: integer;
begin
  base := sym_idx * 32;
  i := 0;
  c := sym_name[base];
  while c <> 0 do
  begin
    writechar(c);
    i := i + 1;
    c := sym_name[base + i]
  end
end;

procedure WriteCurrentUnitName;
var
  i: integer;
begin
  for i := 0 to current_unit_len - 1 do
    writechar(current_unit_name[i])
end;

procedure WriteLoadedUnitName(unit_idx: integer);
var
  base, i, c: integer;
begin
  base := unit_idx * 32;
  i := 0;
  c := loaded_units[base];
  while c <> 0 do
  begin
    writechar(c);
    i := i + 1;
    c := loaded_units[base + i]
  end
end;

procedure EmitGloblCurrentUnit(sym_idx: integer);
begin
  write('.globl _');
  WriteCurrentUnitName;
  write('_');
  WriteSymName(sym_idx);
  writeln
end;

procedure EmitUnitLabel(sym_idx: integer);
begin
  write('_');
  WriteCurrentUnitName;
  write('_');
  WriteSymName(sym_idx);
  writeln(':')
end;

procedure EmitBLUnitProc(unit_idx, sym_idx: integer);
begin
  write('    bl _');
  WriteLoadedUnitName(unit_idx);
  write('_');
  WriteSymName(sym_idx);
  writeln
end;

procedure EmitGloblUnitInit;
begin
  write('.globl _');
  WriteCurrentUnitName;
  writeln('_init')
end;

procedure EmitUnitInitLabel;
begin
  write('_');
  WriteCurrentUnitName;
  writeln('_init:')
end;

procedure EmitBLUnitInit(unit_idx: integer);
begin
  write('    bl _');
  WriteLoadedUnitName(unit_idx);
  writeln('_init')
end;

