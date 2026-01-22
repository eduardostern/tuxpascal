program Tetris;
{ Tetris demo for TuxPascal }
{ Controls: Arrow keys or A/D/W/S, Space = drop, Q = quit }

const
  BoardWidth = 10;
  BoardHeight = 20;
  BoardLeft = 25;
  BoardTop = 2;

var
  board: array[0..199] of integer;  { 10 x 20 board }

  { Current piece: 4 blocks with x,y offsets from piece center }
  curPiece: array[0..7] of integer;  { 4 pairs of x,y }
  curX, curY: integer;  { piece position }
  curType: integer;     { 0-6 }
  curColor: integer;

  score, linesCleared: integer;
  gameOver: integer;
  dropTimer: integer;
  rngState: integer;  { for pseudo-random }
  ix: integer;  { loop variable for main }

procedure SetPiece(t: integer);
{ Set curPiece blocks based on piece type t }
begin
  curType := t;
  if t = 0 then begin { I - cyan }
    curPiece[0] := 0; curPiece[1] := -1;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 0; curPiece[5] := 1;
    curPiece[6] := 0; curPiece[7] := 2;
    curColor := 6
  end
  else if t = 1 then begin { O - yellow }
    curPiece[0] := 0; curPiece[1] := 0;
    curPiece[2] := 1; curPiece[3] := 0;
    curPiece[4] := 0; curPiece[5] := 1;
    curPiece[6] := 1; curPiece[7] := 1;
    curColor := 3
  end
  else if t = 2 then begin { T - magenta }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 1; curPiece[5] := 0;
    curPiece[6] := 0; curPiece[7] := 1;
    curColor := 5
  end
  else if t = 3 then begin { S - green }
    curPiece[0] := 0; curPiece[1] := 0;
    curPiece[2] := 1; curPiece[3] := 0;
    curPiece[4] := -1; curPiece[5] := 1;
    curPiece[6] := 0; curPiece[7] := 1;
    curColor := 2
  end
  else if t = 4 then begin { Z - red }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 0; curPiece[5] := 1;
    curPiece[6] := 1; curPiece[7] := 1;
    curColor := 1
  end
  else if t = 5 then begin { J - blue }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 1; curPiece[5] := 0;
    curPiece[6] := -1; curPiece[7] := 1;
    curColor := 4
  end
  else begin { L - orange/white }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 1; curPiece[5] := 0;
    curPiece[6] := 1; curPiece[7] := 1;
    curColor := 7
  end
end;

function Random7: integer;
{ Simple pseudo-random 0-6 }
begin
  rngState := rngState * 1103515245 + 12345;
  if rngState < 0 then rngState := 0 - rngState;
  Random7 := rngState mod 7
end;

procedure DrawBlock(x, y, c: integer);
begin
  if (x >= 0) and (x < BoardWidth) and (y >= 0) and (y < BoardHeight) then
  begin
    GotoXY(BoardLeft + x * 2, BoardTop + y);
    if c = 0 then
    begin
      NormVideo;
      write('. ')
    end
    else
    begin
      TextBackground(c);
      write('  ');
      NormVideo
    end
  end
end;

procedure DrawBoard;
var x, y: integer;
begin
  for y := 0 to BoardHeight - 1 do
    for x := 0 to BoardWidth - 1 do
      DrawBlock(x, y, board[y * BoardWidth + x])
end;

procedure DrawPiece(c: integer);
var k, bx, by: integer;
begin
  for k := 0 to 3 do
  begin
    bx := curX + curPiece[k * 2];
    by := curY + curPiece[k * 2 + 1];
    DrawBlock(bx, by, c)
  end
end;

procedure ShowPiece;
begin
  DrawPiece(curColor)
end;

procedure HidePiece;
begin
  DrawPiece(0)
end;

procedure DrawBorder;
var y: integer;
begin
  NormVideo;
  for y := 0 to BoardHeight - 1 do
  begin
    GotoXY(BoardLeft - 1, BoardTop + y);
    write('|');
    GotoXY(BoardLeft + BoardWidth * 2, BoardTop + y);
    write('|')
  end;
  GotoXY(BoardLeft - 1, BoardTop + BoardHeight);
  write('+--------------------+')
end;

procedure DrawInfo;
begin
  NormVideo;
  GotoXY(BoardLeft + 24, BoardTop);
  write('Score: '); write(score); write('   ');
  GotoXY(BoardLeft + 24, BoardTop + 1);
  write('Lines: '); write(linesCleared); write('   ');
  GotoXY(BoardLeft + 24, BoardTop + 3);
  write('Controls:');
  GotoXY(BoardLeft + 24, BoardTop + 4);
  write('Arrows/WASD');
  GotoXY(BoardLeft + 24, BoardTop + 5);
  write('Space=Drop');
  GotoXY(BoardLeft + 24, BoardTop + 6);
  write('Q=Quit')
end;

function Collides: integer;
var k, bx, by: integer;
begin
  Collides := 0;
  for k := 0 to 3 do
  begin
    bx := curX + curPiece[k * 2];
    by := curY + curPiece[k * 2 + 1];
    if bx < 0 then Collides := 1;
    if bx >= BoardWidth then Collides := 1;
    if by >= BoardHeight then Collides := 1;
    if by >= 0 then
      if board[by * BoardWidth + bx] > 0 then
        Collides := 1
  end
end;

procedure RotateCW;
{ Rotate piece 90 degrees clockwise: (x,y) -> (y,-x) }
var k, ox, oy: integer;
begin
  for k := 0 to 3 do
  begin
    ox := curPiece[k * 2];
    oy := curPiece[k * 2 + 1];
    curPiece[k * 2] := oy;
    curPiece[k * 2 + 1] := 0 - ox
  end
end;

procedure RotateCCW;
{ Rotate piece 90 degrees counter-clockwise: (x,y) -> (-y,x) }
var k, ox, oy: integer;
begin
  for k := 0 to 3 do
  begin
    ox := curPiece[k * 2];
    oy := curPiece[k * 2 + 1];
    curPiece[k * 2] := 0 - oy;
    curPiece[k * 2 + 1] := ox
  end
end;

procedure LockPiece;
var k, bx, by: integer;
begin
  for k := 0 to 3 do
  begin
    bx := curX + curPiece[k * 2];
    by := curY + curPiece[k * 2 + 1];
    if (bx >= 0) and (bx < BoardWidth) and (by >= 0) and (by < BoardHeight) then
      board[by * BoardWidth + bx] := curColor
  end
end;

procedure ClearLines;
var y, x, full, dy, cleared: integer;
begin
  cleared := 0;
  y := BoardHeight - 1;
  while y >= 0 do
  begin
    full := 1;
    for x := 0 to BoardWidth - 1 do
      if board[y * BoardWidth + x] = 0 then full := 0;

    if full = 1 then
    begin
      cleared := cleared + 1;
      { Move everything above down }
      for dy := y downto 1 do
        for x := 0 to BoardWidth - 1 do
          board[dy * BoardWidth + x] := board[(dy - 1) * BoardWidth + x];
      { Clear top row }
      for x := 0 to BoardWidth - 1 do
        board[x] := 0
      { Don't decrement y - check same row again }
    end
    else
      y := y - 1
  end;

  if cleared > 0 then
  begin
    linesCleared := linesCleared + cleared;
    score := score + cleared * cleared * 100;
    DrawBoard;
    DrawInfo
  end
end;

procedure NewPiece;
var t: integer;
begin
  t := Random7;
  SetPiece(t);
  curX := 4;
  curY := 0;

  if Collides = 1 then
    gameOver := 1
  else
    ShowPiece
end;

procedure TryMove(dx, dy: integer);
var oldX, oldY: integer;
begin
  HidePiece;
  oldX := curX;
  oldY := curY;
  curX := curX + dx;
  curY := curY + dy;

  if Collides = 1 then
  begin
    curX := oldX;
    curY := oldY;
    ShowPiece;
    if dy > 0 then
    begin
      { Couldn't move down - lock piece }
      LockPiece;
      ClearLines;
      NewPiece
    end
  end
  else
    ShowPiece
end;

procedure TryRotate;
var oldPiece: array[0..7] of integer;
    k: integer;
begin
  HidePiece;
  { Save current piece }
  for k := 0 to 7 do
    oldPiece[k] := curPiece[k];

  RotateCW;

  if Collides = 1 then
  begin
    { Restore }
    for k := 0 to 7 do
      curPiece[k] := oldPiece[k]
  end;
  ShowPiece
end;

procedure Drop;
begin
  HidePiece;
  while Collides = 0 do
    curY := curY + 1;
  curY := curY - 1;
  ShowPiece;
  LockPiece;
  ClearLines;
  NewPiece
end;

procedure HandleInput;
var ch, ch2: integer;
begin
  if KeyPressed then
  begin
    ch := readchar;

    if ch = 27 then  { ESC sequence for arrow keys }
    begin
      if KeyPressed then
      begin
        ch := readchar;
        if ch = 91 then
        begin
          if KeyPressed then
          begin
            ch2 := readchar;
            if ch2 = 65 then TryRotate        { Up }
            else if ch2 = 66 then TryMove(0, 1)   { Down }
            else if ch2 = 67 then TryMove(1, 0)   { Right }
            else if ch2 = 68 then TryMove(-1, 0)  { Left }
          end
        end
      end
    end
    else if (ch = 119) or (ch = 87) then TryRotate       { W }
    else if (ch = 115) or (ch = 83) then TryMove(0, 1)   { S }
    else if (ch = 100) or (ch = 68) then TryMove(1, 0)   { D }
    else if (ch = 97) or (ch = 65) then TryMove(-1, 0)   { A }
    else if ch = 32 then Drop                            { Space }
    else if (ch = 113) or (ch = 81) then gameOver := 1   { Q }
  end
end;

begin
  ClrScr;
  HideCursor;
  InitKeyboard;

  { Initialize }
  score := 0;
  linesCleared := 0;
  gameOver := 0;
  dropTimer := 0;
  rngState := 12345;

  { Clear board }
  for ix := 0 to 199 do
    board[ix] := 0;

  { Draw UI }
  GotoXY(BoardLeft + 4, 1);
  HighVideo;
  TextColor(6);
  write('T E T R I S');
  NormVideo;

  DrawBorder;
  DrawBoard;
  DrawInfo;

  NewPiece;

  { Game loop }
  while gameOver = 0 do
  begin
    HandleInput;

    dropTimer := dropTimer + 1;
    if dropTimer >= 20 then
    begin
      TryMove(0, 1);
      dropTimer := 0
    end;

    Sleep(25)
  end;

  { Game over }
  DoneKeyboard;
  ShowCursor;

  GotoXY(BoardLeft + 4, BoardTop + 9);
  HighVideo;
  TextColor(1);
  write('GAME OVER!');
  NormVideo;

  GotoXY(1, BoardTop + BoardHeight + 3);
  write('Final Score: ');
  writeln(score);
  write('Lines: ');
  writeln(linesCleared)
end.
