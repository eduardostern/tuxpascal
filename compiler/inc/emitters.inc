{ ----- Output Helpers ----- }

Procedure EmitIndent;
Begin
  Write('    ')
End;

Procedure EmitNL;
Begin
  WriteLn
End;

Procedure EmitGlobl;
Begin
  WriteLn('.globl _main')
End;

Procedure EmitAlign4;
Begin
  WriteLn('.align 4')
End;

Procedure EmitMain;
Begin
  WriteLn('_main:')
End;

Procedure EmitLabel(n: Integer);
Begin
  Write('L'); Write(n); WriteLn(':')
End;

Function NewLabel: Integer;
Begin
  NewLabel := label_count;
  label_count := label_count + 1
End;

Procedure EmitStp;
Begin
  WriteLn('    stp x29, x30, [sp, #-16]!')
End;

Procedure EmitMovFP;
Begin
  WriteLn('    mov x29, sp')
End;

Procedure EmitLdp;
Begin
  WriteLn('    ldp x29, x30, [sp], #16')
End;

Procedure EmitRet;
Begin
  WriteLn('    ret')
End;

Procedure EmitStoreStaticLink;
Begin
  WriteLn('    stur x9, [x29, #-8]')
End;

Procedure EmitStaticLink(sym_level, cur_level: Integer);
Var
  i: Integer;
Begin
  WriteLn('    mov x9, x29');
  For i := cur_level DownTo sym_level + 1 Do
    WriteLn('    ldur x9, [x9, #-8]')
End;

Procedure EmitMovX0(val: Integer);
Var
  lo, hi: Integer;
  neg: Integer;
Begin
  neg := 0;
  If val < 0 Then
  Begin
    neg := 1;
    val := 0 - val
  End;
  If val > 65535 Then
  Begin
    lo := val Mod 65536;
    hi := val Div 65536;
    Write('    movz x0, #'); WriteLn(lo);
    Write('    movk x0, #'); Write(hi); WriteLn(', lsl #16')
  End
  Else
  Begin
    Write('    mov x0, #'); WriteLn(val)
  End;
  If neg = 1 Then
    WriteLn('    neg x0, x0')
End;

Procedure EmitMovX16(val: Integer);
Var
  lo, hi: Integer;
Begin
  If val > 65535 Then
  Begin
    lo := val Mod 65536;
    hi := val Div 65536;
    Write('    movz x16, #'); WriteLn(lo);
    Write('    movk x16, #'); Write(hi); WriteLn(', lsl #16')
  End
  Else
  Begin
    Write('    mov x16, #'); WriteLn(val)
  End
End;

Procedure EmitMovX8(val: Integer);
Var
  lo, hi: Integer;
  neg: Integer;
Begin
  neg := 0;
  If val < 0 Then
  Begin
    neg := 1;
    val := 0 - val
  End;
  If val > 65535 Then
  Begin
    lo := val Mod 65536;
    hi := val Div 65536;
    Write('    movz x8, #'); WriteLn(lo);
    Write('    movk x8, #'); Write(hi); WriteLn(', lsl #16')
  End
  Else
  Begin
    Write('    mov x8, #'); WriteLn(val)
  End;
  If neg = 1 Then
    WriteLn('    neg x8, x8')
End;

Procedure EmitSubLargeOffset(dest, src, offset: Integer);
Var
  lo, hi: Integer;
Begin
  If offset <= 4095 Then
  Begin
    Write('    sub x'); Write(dest); Write(', x'); Write(src); Write(', #'); WriteLn(offset)
  End
  Else
  Begin
    lo := offset Mod 65536;
    hi := offset Div 65536;
    Write('    movz x10, #'); WriteLn(lo);
    If hi > 0 Then
    Begin
      Write('    movk x10, #'); Write(hi); WriteLn(', lsl #16')
    End;
    Write('    sub x'); Write(dest); Write(', x'); Write(src); WriteLn(', x10')
  End
End;

Procedure EmitSvc;
Begin
  WriteLn('    svc #0x80')
End;

Procedure EmitPushX0;
Begin
  WriteLn('    str x0, [sp, #-16]!')
End;

Procedure EmitPopX0;
Begin
  WriteLn('    ldr x0, [sp], #16')
End;

Procedure EmitPopX1;
Begin
  WriteLn('    ldr x1, [sp], #16')
End;

Procedure EmitPushX1;
Begin
  WriteLn('    str x1, [sp, #-16]!')
End;

Procedure EmitAdd;
Begin
  WriteLn('    add x0, x1, x0')
End;

Procedure EmitSub;
Begin
  WriteLn('    sub x0, x1, x0')
End;

Procedure EmitMul;
Begin
  WriteLn('    mul x0, x1, x0')
End;

Procedure EmitSDiv;
Begin
  WriteLn('    sdiv x0, x1, x0')
End;

Procedure EmitMovX0X20;
Begin
  WriteLn('    mov x0, x20')
End;

Procedure EmitBranchLabel(lbl: Integer);
Begin
  Write('    b L'); WriteLn(lbl)
End;

Procedure EmitBranchLabelZ(lbl: Integer);
Begin
  Write('    cbz x0, L'); WriteLn(lbl)
End;

Procedure EmitBranchLabelNZ(lbl: Integer);
Begin
  Write('    cbnz x0, L'); WriteLn(lbl)
End;

Procedure EmitBL(lbl: Integer);
Begin
  Write('    bl L'); WriteLn(lbl)
End;

Procedure EmitCmpX0X1;
Begin
  WriteLn('    cmp x1, x0')
End;

Procedure EmitCset(cond: Integer);
Begin
  Write('    cset x0, ');
  If cond = 0 Then WriteLn('eq')
  Else If cond = 1 Then WriteLn('ne')
  Else If cond = 2 Then WriteLn('lt')
  Else If cond = 3 Then WriteLn('le')
  Else If cond = 4 Then WriteLn('gt')
  Else WriteLn('ge')
End;

Procedure EmitLdurX0(offset: Integer);
Begin
  If (offset >= -255) And (offset <= 255) Then
  Begin
    Write('    ldur x0, [x29, #'); Write(offset); WriteLn(']')
  End
  Else
  Begin
    EmitMovX8(offset);
    WriteLn('    add x8, x29, x8');
    WriteLn('    ldr x0, [x8]')
  End
End;

Procedure EmitLdurX0Outer(offset, sym_level, cur_level: Integer);
Var
  i: Integer;
Begin
  WriteLn('    mov x8, x29');
  For i := cur_level DownTo sym_level + 1 Do
    WriteLn('    ldur x8, [x8, #-8]');
  If (offset >= -255) And (offset <= 255) Then
  Begin
    Write('    ldur x0, [x8, #'); Write(offset); WriteLn(']')
  End
  Else
  Begin
    WriteLn('    mov x9, x8');
    EmitMovX8(offset);
    WriteLn('    add x8, x9, x8');
    WriteLn('    ldr x0, [x8]')
  End
End;

Procedure EmitFollowChain(sym_level, cur_level: Integer);
Var
  i: Integer;
Begin
  WriteLn('    mov x8, x29');
  For i := cur_level DownTo sym_level + 1 Do
    WriteLn('    ldur x8, [x8, #-8]')
End;

Procedure EmitVarAddr(var_idx, cur_scope: Integer);
Var
  offset, var_level, i: Integer;
Begin
  offset := sym_offset[var_idx];
  var_level := sym_level[var_idx];

  If var_level < cur_scope Then
  Begin
    WriteLn('    mov x8, x29');
    For i := cur_scope DownTo var_level + 1 Do
      WriteLn('    ldur x8, [x8, #-8]');
    If offset < 0 Then
      EmitSubLargeOffset(0, 8, 0 - offset)
    Else
    Begin
      If offset <= 4095 Then
      Begin
        Write('    add x0, x8, #'); WriteLn(offset)
      End
      Else
      Begin
        WriteLn('    mov x9, x8');
        EmitMovX8(offset);
        WriteLn('    add x0, x9, x8')
      End
    End
  End
  Else
  Begin
    If offset < 0 Then
      EmitSubLargeOffset(0, 29, 0 - offset)
    Else
    Begin
      If offset <= 4095 Then
      Begin
        Write('    add x0, x29, #'); WriteLn(offset)
      End
      Else
      Begin
        EmitMovX8(offset);
        WriteLn('    add x0, x29, x8')
      End
    End
  End
End;

Procedure EmitSturX0Outer(offset, sym_level, cur_level: Integer);
Var
  i: Integer;
Begin
  EmitPushX0;
  WriteLn('    mov x8, x29');
  For i := cur_level DownTo sym_level + 1 Do
    WriteLn('    ldur x8, [x8, #-8]');
  EmitPopX0;
  If (offset >= -255) And (offset <= 255) Then
  Begin
    Write('    stur x0, [x8, #'); Write(offset); WriteLn(']')
  End
  Else
  Begin
    WriteLn('    mov x9, x8');
    EmitPushX0;
    EmitMovX8(offset);
    WriteLn('    add x8, x9, x8');
    EmitPopX0;
    WriteLn('    str x0, [x8]')
  End
End;

Procedure EmitSturX0(offset: Integer);
Begin
  If (offset >= -255) And (offset <= 255) Then
  Begin
    Write('    stur x0, [x29, #'); Write(offset); WriteLn(']')
  End
  Else
  Begin
    EmitMovX8(offset);
    WriteLn('    add x8, x29, x8');
    WriteLn('    str x0, [x8]')
  End
End;

Procedure EmitSubSP(n: Integer);
Begin
  If n <= 4095 Then
  Begin
    Write('    sub sp, sp, #'); WriteLn(n)
  End
  Else
  Begin
    EmitMovX8(n);
    WriteLn('    sub sp, sp, x8')
  End
End;

Procedure EmitAddSP(n: Integer);
Begin
  If n <= 4095 Then
  Begin
    Write('    add sp, sp, #'); WriteLn(n)
  End
  Else
  Begin
    EmitMovX8(n);
    WriteLn('    add sp, sp, x8')
  End
End;

Procedure EmitNeg;
Begin
  WriteLn('    neg x0, x0')
End;

Procedure EmitMsub;
Begin
  WriteLn('    msub x0, x0, x2, x1')
End;

Procedure EmitMovX2X0;
Begin
  WriteLn('    mov x2, x0')
End;

Procedure EmitAndX0X1;
Begin
  WriteLn('    And x0, x1, x0')
End;

Procedure EmitOrrX0X1;
Begin
  WriteLn('    orr x0, x1, x0')
End;

Procedure EmitAndImm(val: Integer);
Begin
  Write('    And x0, x0, #'); WriteLn(val)
End;

Procedure EmitEorX0(val: Integer);
Begin
  Write('    eor x0, x0, #'); WriteLn(val)
End;

{ ----- Floating Point Emitters ----- }

Procedure EmitPushD0;
Begin
  WriteLn('    str d0, [sp, #-16]!')
End;

Procedure EmitPopD0;
Begin
  WriteLn('    ldr d0, [sp], #16')
End;

Procedure EmitPopD1;
Begin
  WriteLn('    ldr d1, [sp], #16')
End;

Procedure EmitFAdd;
Begin
  WriteLn('    fadd d0, d1, d0')
End;

Procedure EmitFSub;
Begin
  WriteLn('    fsub d0, d1, d0')
End;

Procedure EmitFMul;
Begin
  WriteLn('    fmul d0, d1, d0')
End;

Procedure EmitFDiv;
Begin
  WriteLn('    fdiv d0, d1, d0')
End;

Procedure EmitFNeg;
Begin
  WriteLn('    fneg d0, d0')
End;

Procedure EmitFCmp;
Begin
  WriteLn('    fcmp d1, d0')
End;

Procedure EmitScvtfD0X0;
Begin
  WriteLn('    scvtf d0, x0')
End;

Procedure EmitScvtfD1X1;
Begin
  WriteLn('    scvtf d1, x1')
End;

Procedure EmitFcvtzsX0D0;
Begin
  WriteLn('    fcvtzs x0, d0')
End;

Procedure EmitFmovD0X0;
Begin
  WriteLn('    fmov d0, x0')
End;

Procedure EmitFmovX0D0;
Begin
  WriteLn('    fmov x0, d0')
End;

Procedure EmitLdurD0(offset: Integer);
Begin
  Write('    ldur d0, [x29, #'); Write(offset); WriteLn(']')
End;

Procedure EmitSturD0(offset: Integer);
Begin
  Write('    stur d0, [x29, #'); Write(offset); WriteLn(']')
End;

Procedure EmitLdurD0Outer(offset, sym_level, cur_level: Integer);
Var
  i: Integer;
Begin
  WriteLn('    mov x8, x29');
  For i := cur_level DownTo sym_level + 1 Do
    WriteLn('    ldur x8, [x8, #-8]');
  Write('    ldur d0, [x8, #'); Write(offset); WriteLn(']')
End;

Procedure EmitSturD0Outer(offset, sym_level, cur_level: Integer);
Var
  i: Integer;
Begin
  EmitPushD0;
  WriteLn('    mov x8, x29');
  For i := cur_level DownTo sym_level + 1 Do
    WriteLn('    ldur x8, [x8, #-8]');
  EmitPopD0;
  Write('    stur d0, [x8, #'); Write(offset); WriteLn(']')
End;

{ ----- Unit Symbol Emitters ----- }

Procedure WriteSymName(sym_idx: Integer);
Var
  base, i, c: Integer;
Begin
  base := sym_idx * 32;
  i := 0;
  c := sym_name[base];
  While c <> 0 Do
  Begin
    WriteChar(c);
    i := i + 1;
    c := sym_name[base + i]
  End
End;

Procedure WriteCurrentUnitName;
Var
  i: Integer;
Begin
  For i := 0 To current_unit_len - 1 Do
    WriteChar(current_unit_name[i])
End;

Procedure WriteLoadedUnitName(unit_idx: Integer);
Var
  base, i, c: Integer;
Begin
  base := unit_idx * 32;
  i := 0;
  c := loaded_units[base];
  While c <> 0 Do
  Begin
    WriteChar(c);
    i := i + 1;
    c := loaded_units[base + i]
  End
End;

Procedure EmitGloblCurrentUnit(sym_idx: Integer);
Begin
  Write('.globl _');
  WriteCurrentUnitName;
  Write('_');
  WriteSymName(sym_idx);
  WriteLn
End;

Procedure EmitUnitLabel(sym_idx: Integer);
Begin
  Write('_');
  WriteCurrentUnitName;
  Write('_');
  WriteSymName(sym_idx);
  WriteLn(':')
End;

Procedure EmitBLUnitProc(unit_idx, sym_idx: Integer);
Begin
  Write('    bl _');
  WriteLoadedUnitName(unit_idx);
  Write('_');
  WriteSymName(sym_idx);
  WriteLn
End;

Procedure EmitGloblUnitInit;
Begin
  Write('.globl _');
  WriteCurrentUnitName;
  WriteLn('_init')
End;

Procedure EmitUnitInitLabel;
Begin
  Write('_');
  WriteCurrentUnitName;
  WriteLn('_init:')
End;

Procedure EmitBLUnitInit(unit_idx: Integer);
Begin
  Write('    bl _');
  WriteLoadedUnitName(unit_idx);
  WriteLn('_init')
End;

