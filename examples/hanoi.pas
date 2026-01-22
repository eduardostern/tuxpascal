program TowersOfHanoi;
{ Classic Towers of Hanoi with smooth animated disks! }
{ Like a ZX Spectrum or C64 game }

const
  NumDisks = 5;
  BaseRow = 18;
  Peg1Col = 15;
  Peg2Col = 40;
  Peg3Col = 65;
  PegHeight = 10;
  TopRow = 5;       { Row where disks travel horizontally }
  AnimDelay = 12;   { ms between animation frames }

var
  { Tower arrays: tower[peg, position] = disk size (0 = empty) }
  tower1: array[0..9] of integer;
  tower2: array[0..9] of integer;
  tower3: array[0..9] of integer;
  top1, top2, top3: integer;  { top position for each tower }
  moveCount: integer;
  i: integer;

procedure EraseDisk(col, row, size: integer);
var j, startCol: integer;
begin
  startCol := col - size;
  GotoXY(startCol, row);
  NormVideo;
  for j := 1 to size * 2 + 1 do
    write(' ')
end;

procedure DrawDisk(col, row, size, clr: integer);
var j, startCol: integer;
begin
  if size = 0 then
  begin
    { Draw empty peg segment }
    GotoXY(col, row);
    NormVideo;
    write('|')
  end
  else
  begin
    { Draw colored disk }
    startCol := col - size;
    GotoXY(startCol, row);
    TextBackground(clr);
    TextColor(0);  { black text }
    for j := 1 to size do
      write(' ');
    write('=');
    for j := 1 to size do
      write(' ');
    NormVideo
  end
end;

procedure DrawPeg(col: integer);
var row: integer;
begin
  NormVideo;
  for row := TopRow to BaseRow - 1 do
  begin
    GotoXY(col, row);
    write('|')
  end;
  { Draw base }
  GotoXY(col - 6, BaseRow);
  write('=============')
end;

procedure DrawTower(pegNum, col: integer);
var row, idx, size, clr: integer;
begin
  { Draw peg }
  DrawPeg(col);

  { Draw disks }
  for idx := 0 to NumDisks - 1 do
  begin
    if pegNum = 1 then
      size := tower1[idx]
    else if pegNum = 2 then
      size := tower2[idx]
    else
      size := tower3[idx];

    if size > 0 then
    begin
      clr := size;  { color based on disk size }
      if clr > 7 then clr := clr - 7;
      row := BaseRow - 1 - idx;
      DrawDisk(col, row, size, clr)
    end
  end
end;

procedure DrawAllTowers;
begin
  DrawTower(1, Peg1Col);
  DrawTower(2, Peg2Col);
  DrawTower(3, Peg3Col)
end;

procedure ShowStatus;
begin
  GotoXY(1, 20);
  NormVideo;
  write('Moves: ');
  write(moveCount);
  write('    ')
end;

procedure AnimateDisk(fromCol, fromRow, toCol, toRow, size, clr: integer);
var curCol, curRow, dir: integer;
begin
  curCol := fromCol;
  curRow := fromRow;

  { Phase 1: Move UP to top row }
  while curRow > TopRow do
  begin
    EraseDisk(curCol, curRow, size);
    GotoXY(curCol, curRow);
    write('|');  { restore peg }
    curRow := curRow - 1;
    DrawDisk(curCol, curRow, size, clr);
    Sleep(AnimDelay)
  end;

  { Phase 2: Move HORIZONTALLY to destination column }
  if toCol > fromCol then
    dir := 1
  else
    dir := -1;

  while curCol <> toCol do
  begin
    EraseDisk(curCol, curRow, size);
    curCol := curCol + dir;
    DrawDisk(curCol, curRow, size, clr);
    { Restore pegs only if NOT covered by current disk }
    if (Peg1Col < curCol - size) or (Peg1Col > curCol + size) then
    begin
      GotoXY(Peg1Col, curRow);
      write('|')
    end;
    if (Peg2Col < curCol - size) or (Peg2Col > curCol + size) then
    begin
      GotoXY(Peg2Col, curRow);
      write('|')
    end;
    if (Peg3Col < curCol - size) or (Peg3Col > curCol + size) then
    begin
      GotoXY(Peg3Col, curRow);
      write('|')
    end;
    Sleep(AnimDelay)
  end;

  { Phase 3: Move DOWN to destination row }
  while curRow < toRow do
  begin
    EraseDisk(curCol, curRow, size);
    GotoXY(curCol, curRow);
    write('|');  { restore peg }
    curRow := curRow + 1;
    DrawDisk(curCol, curRow, size, clr);
    Sleep(AnimDelay)
  end
end;

procedure MoveDisk(fromPeg, toPeg: integer);
var disk, clr: integer;
    fromCol, toCol: integer;
    fromRow, toRow: integer;
    fromTop, toTop: integer;
begin
  { Get the disk from source peg }
  if fromPeg = 1 then
  begin
    fromTop := top1;
    disk := tower1[fromTop];
    tower1[fromTop] := 0;
    top1 := top1 - 1;
    fromCol := Peg1Col
  end
  else if fromPeg = 2 then
  begin
    fromTop := top2;
    disk := tower2[fromTop];
    tower2[fromTop] := 0;
    top2 := top2 - 1;
    fromCol := Peg2Col
  end
  else
  begin
    fromTop := top3;
    disk := tower3[fromTop];
    tower3[fromTop] := 0;
    top3 := top3 - 1;
    fromCol := Peg3Col
  end;

  { Calculate source row }
  fromRow := BaseRow - 1 - fromTop;

  { Get destination info }
  if toPeg = 1 then
  begin
    toTop := top1 + 1;
    top1 := toTop;
    tower1[toTop] := disk;
    toCol := Peg1Col
  end
  else if toPeg = 2 then
  begin
    toTop := top2 + 1;
    top2 := toTop;
    tower2[toTop] := disk;
    toCol := Peg2Col
  end
  else
  begin
    toTop := top3 + 1;
    top3 := toTop;
    tower3[toTop] := disk;
    toCol := Peg3Col
  end;

  { Calculate destination row }
  toRow := BaseRow - 1 - toTop;

  { Get color }
  clr := disk;
  if clr > 7 then clr := clr - 7;

  { Animate the disk movement }
  AnimateDisk(fromCol, fromRow, toCol, toRow, disk, clr);

  { Redraw destination tower with all its disks }
  DrawTower(toPeg, toCol);

  moveCount := moveCount + 1;
  ShowStatus
end;

procedure Hanoi(n, fromPeg, toPeg, auxPeg: integer);
begin
  if n > 0 then
  begin
    Hanoi(n - 1, fromPeg, auxPeg, toPeg);
    MoveDisk(fromPeg, toPeg);
    Hanoi(n - 1, auxPeg, toPeg, fromPeg)
  end
end;

begin
  { Initialize }
  ClrScr;
  HideCursor;
  moveCount := 0;

  { Clear tower arrays }
  for i := 0 to 9 do
  begin
    tower1[i] := 0;
    tower2[i] := 0;
    tower3[i] := 0
  end;

  { Put all disks on tower 1 (largest at bottom) }
  for i := 0 to NumDisks - 1 do
    tower1[i] := NumDisks - i;
  top1 := NumDisks - 1;
  top2 := -1;
  top3 := -1;

  { Draw title }
  GotoXY(25, 1);
  HighVideo;
  TextColor(3);  { cyan }
  write('*** TOWERS OF HANOI ***');
  NormVideo;

  GotoXY(20, 3);
  write('Move all disks from left to right');

  GotoXY(10, BaseRow + 1);
  write('Peg A');
  GotoXY(35, BaseRow + 1);
  write('Peg B');
  GotoXY(60, BaseRow + 1);
  write('Peg C');

  { Draw initial state }
  DrawAllTowers;
  ShowStatus;

  GotoXY(1, 22);
  write('Press ENTER to start...');
  readln;

  GotoXY(1, 22);
  write('                        ');

  { Solve it! }
  Hanoi(NumDisks, 1, 3, 2);

  { Done! }
  GotoXY(25, 22);
  HighVideo;
  TextColor(2);  { green }
  write('*** SOLVED! ***');
  NormVideo;
  ShowCursor;

  GotoXY(1, 24)
end.
