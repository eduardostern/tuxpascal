program GraphTetris;
{ Graphical Tetris for TuxPascal using TuxGraph library }
{ Controls: Arrow keys or A/D/W/S, Space = drop, Q = quit }

const
  { Board dimensions }
  BoardWidth = 10;
  BoardHeight = 20;
  BlockSize = 28;
  BoardLeft = 40;
  BoardTop = 40;

  { Window size }
  WinWidth = 400;
  WinHeight = 640;

  { Colors (0xRRGGBB) }
  ColorBlack = 0;
  ColorWhite = 16777215;
  ColorCyan = 65535;
  ColorYellow = 16776960;
  ColorMagenta = 16711935;
  ColorGreen = 65280;
  ColorRed = 16711680;
  ColorBlue = 255;
  ColorOrange = 16744448;
  ColorGray = 4210752;
  ColorDarkGray = 2105376;
  ColorBorder = 8421504;

  { Arrow key codes from macOS }
  KeyUp = 63232;
  KeyDown = 63233;
  KeyLeft = 63234;
  KeyRight = 63235;

var
  board: array[0..199] of integer;  { 10 x 20 board, stores color }

  { Current piece: 4 blocks with x,y offsets }
  curPiece: array[0..7] of integer;
  curX, curY: integer;
  curType: integer;
  curColor: integer;

  { Next piece preview }
  nextType: integer;

  score, linesCleared, level: integer;
  gameOver: integer;
  dropTimer, dropSpeed: integer;
  rngState: integer;
  ix: integer;

{ External graphics functions }
function gfx_init(width, height: integer): integer; external;
procedure gfx_close; external;
procedure gfx_set_pixel(x, y, color: integer); external;
procedure gfx_clear(color: integer); external;
procedure gfx_line(x1, y1, x2, y2, color: integer); external;
procedure gfx_rect(x, y, w, h, color: integer); external;
procedure gfx_fill_rect(x, y, w, h, color: integer); external;
procedure gfx_present; external;
function gfx_running: integer; external;
procedure gfx_sleep(ms: integer); external;
function gfx_read_key: integer; external;
procedure gfx_poll_events; external;

{ External sound functions }
procedure snd_beep(frequency, duration: integer); external;
procedure snd_tone(frequency, duration: integer); external;
procedure snd_noise(duration: integer); external;
procedure snd_volume(vol: integer); external;

function GetPieceColor(t: integer): integer;
begin
  if t = 0 then GetPieceColor := ColorCyan
  else if t = 1 then GetPieceColor := ColorYellow
  else if t = 2 then GetPieceColor := ColorMagenta
  else if t = 3 then GetPieceColor := ColorGreen
  else if t = 4 then GetPieceColor := ColorRed
  else if t = 5 then GetPieceColor := ColorBlue
  else GetPieceColor := ColorOrange
end;

procedure SetPiece(t: integer);
begin
  curType := t;
  curColor := GetPieceColor(t);

  if t = 0 then begin { I - cyan }
    curPiece[0] := 0; curPiece[1] := -1;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 0; curPiece[5] := 1;
    curPiece[6] := 0; curPiece[7] := 2
  end
  else if t = 1 then begin { O - yellow }
    curPiece[0] := 0; curPiece[1] := 0;
    curPiece[2] := 1; curPiece[3] := 0;
    curPiece[4] := 0; curPiece[5] := 1;
    curPiece[6] := 1; curPiece[7] := 1
  end
  else if t = 2 then begin { T - magenta }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 1; curPiece[5] := 0;
    curPiece[6] := 0; curPiece[7] := 1
  end
  else if t = 3 then begin { S - green }
    curPiece[0] := 0; curPiece[1] := 0;
    curPiece[2] := 1; curPiece[3] := 0;
    curPiece[4] := -1; curPiece[5] := 1;
    curPiece[6] := 0; curPiece[7] := 1
  end
  else if t = 4 then begin { Z - red }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 0; curPiece[5] := 1;
    curPiece[6] := 1; curPiece[7] := 1
  end
  else if t = 5 then begin { J - blue }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 1; curPiece[5] := 0;
    curPiece[6] := -1; curPiece[7] := 1
  end
  else begin { L - orange }
    curPiece[0] := -1; curPiece[1] := 0;
    curPiece[2] := 0; curPiece[3] := 0;
    curPiece[4] := 1; curPiece[5] := 0;
    curPiece[6] := 1; curPiece[7] := 1
  end
end;

function Random7: integer;
begin
  rngState := rngState * 1103515245 + 12345;
  if rngState < 0 then rngState := 0 - rngState;
  Random7 := rngState mod 7
end;

procedure DrawBlock(x, y, c: integer);
var
  px, py: integer;
begin
  px := BoardLeft + x * BlockSize;
  py := BoardTop + y * BlockSize;
  if c = 0 then
  begin
    { Empty cell - full black background with subtle grid }
    gfx_fill_rect(px, py, BlockSize, BlockSize, ColorBlack);
    gfx_rect(px, py, BlockSize, BlockSize, ColorDarkGray)
  end
  else
  begin
    { Filled block with 3D effect }
    gfx_fill_rect(px, py, BlockSize, BlockSize, c);
    { Highlight (top-left) }
    gfx_line(px, py, px + BlockSize - 1, py, ColorWhite);
    gfx_line(px, py, px, py + BlockSize - 1, ColorWhite);
    { Shadow (bottom-right) }
    gfx_line(px + BlockSize - 1, py, px + BlockSize - 1, py + BlockSize - 1, ColorBlack);
    gfx_line(px, py + BlockSize - 1, px + BlockSize - 1, py + BlockSize - 1, ColorBlack);
    { Inner highlight }
    gfx_fill_rect(px + 4, py + 4, BlockSize - 12, BlockSize - 12, c + 2105376)
  end
end;

procedure DrawBoardBlock(x, y: integer);
begin
  if (x >= 0) and (x < BoardWidth) and (y >= 0) and (y < BoardHeight) then
    DrawBlock(x, y, board[y * BoardWidth + x])
end;

procedure DrawBoard;
var
  x, y: integer;
begin
  for y := 0 to BoardHeight - 1 do
    for x := 0 to BoardWidth - 1 do
      DrawBlock(x, y, board[y * BoardWidth + x])
end;

procedure DrawPiece(c: integer);
var
  k, bx, by: integer;
begin
  for k := 0 to 3 do
  begin
    bx := curX + curPiece[k * 2];
    by := curY + curPiece[k * 2 + 1];
    if (bx >= 0) and (bx < BoardWidth) and (by >= 0) and (by < BoardHeight) then
      DrawBlock(bx, by, c)
  end
end;

procedure ShowPiece;
begin
  DrawPiece(curColor)
end;

procedure HidePiece;
var
  k, bx, by: integer;
begin
  for k := 0 to 3 do
  begin
    bx := curX + curPiece[k * 2];
    by := curY + curPiece[k * 2 + 1];
    if (bx >= 0) and (bx < BoardWidth) and (by >= 0) and (by < BoardHeight) then
      DrawBlock(bx, by, board[by * BoardWidth + bx])
  end
end;

procedure DrawBorder;
var
  bw, bh: integer;
begin
  bw := BoardWidth * BlockSize;
  bh := BoardHeight * BlockSize;

  { Outer border }
  gfx_rect(BoardLeft - 4, BoardTop - 4, bw + 8, bh + 8, ColorBorder);
  gfx_rect(BoardLeft - 3, BoardTop - 3, bw + 6, bh + 6, ColorBorder);
  gfx_rect(BoardLeft - 2, BoardTop - 2, bw + 4, bh + 4, ColorWhite)
end;

procedure DrawNextPiece;
var
  nx, ny, k, ox, oy, c: integer;
  tempPiece: array[0..7] of integer;
begin
  { Clear next piece area inside the box }
  gfx_fill_rect(322, 87, 66, 76, ColorBlack);

  { Get piece shape }
  c := GetPieceColor(nextType);

  if nextType = 0 then begin { I }
    tempPiece[0] := 0; tempPiece[1] := -1;
    tempPiece[2] := 0; tempPiece[3] := 0;
    tempPiece[4] := 0; tempPiece[5] := 1;
    tempPiece[6] := 0; tempPiece[7] := 2
  end
  else if nextType = 1 then begin { O }
    tempPiece[0] := 0; tempPiece[1] := 0;
    tempPiece[2] := 1; tempPiece[3] := 0;
    tempPiece[4] := 0; tempPiece[5] := 1;
    tempPiece[6] := 1; tempPiece[7] := 1
  end
  else if nextType = 2 then begin { T }
    tempPiece[0] := -1; tempPiece[1] := 0;
    tempPiece[2] := 0; tempPiece[3] := 0;
    tempPiece[4] := 1; tempPiece[5] := 0;
    tempPiece[6] := 0; tempPiece[7] := 1
  end
  else if nextType = 3 then begin { S }
    tempPiece[0] := 0; tempPiece[1] := 0;
    tempPiece[2] := 1; tempPiece[3] := 0;
    tempPiece[4] := -1; tempPiece[5] := 1;
    tempPiece[6] := 0; tempPiece[7] := 1
  end
  else if nextType = 4 then begin { Z }
    tempPiece[0] := -1; tempPiece[1] := 0;
    tempPiece[2] := 0; tempPiece[3] := 0;
    tempPiece[4] := 0; tempPiece[5] := 1;
    tempPiece[6] := 1; tempPiece[7] := 1
  end
  else if nextType = 5 then begin { J }
    tempPiece[0] := -1; tempPiece[1] := 0;
    tempPiece[2] := 0; tempPiece[3] := 0;
    tempPiece[4] := 1; tempPiece[5] := 0;
    tempPiece[6] := -1; tempPiece[7] := 1
  end
  else begin { L }
    tempPiece[0] := -1; tempPiece[1] := 0;
    tempPiece[2] := 0; tempPiece[3] := 0;
    tempPiece[4] := 1; tempPiece[5] := 0;
    tempPiece[6] := 1; tempPiece[7] := 1
  end;

  { Draw next piece centered in preview box }
  nx := 355;
  ny := 120;
  for k := 0 to 3 do
  begin
    ox := tempPiece[k * 2];
    oy := tempPiece[k * 2 + 1];
    gfx_fill_rect(nx + ox * 14, ny + oy * 14, 12, 12, c)
  end
end;

procedure DrawInfo;
var
  y, barWidth: integer;
begin
  { Clear stats area inside the box }
  gfx_fill_rect(322, 182, 66, 116, ColorBlack);

  { Display level bar }
  y := 190;
  gfx_fill_rect(325, y, 60, 14, ColorDarkGray);
  barWidth := level * 5;
  if barWidth > 56 then barWidth := 56;
  gfx_fill_rect(327, y + 2, barWidth, 10, ColorGreen);

  { Display lines bar }
  y := 220;
  gfx_fill_rect(325, y, 60, 14, ColorDarkGray);
  barWidth := linesCleared * 4;
  if barWidth > 56 then barWidth := 56;
  gfx_fill_rect(327, y + 2, barWidth, 10, ColorCyan);

  { Display score bar }
  y := 250;
  gfx_fill_rect(325, y, 60, 14, ColorDarkGray);
  barWidth := score div 100;
  if barWidth > 56 then barWidth := 56;
  gfx_fill_rect(327, y + 2, barWidth, 10, ColorYellow)
end;

function Collides: integer;
var
  k, bx, by: integer;
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
var
  k, ox, oy: integer;
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
var
  k, ox, oy: integer;
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
var
  k, bx, by: integer;
begin
  snd_beep(150, 30);  { Thud sound when piece locks }
  for k := 0 to 3 do
  begin
    bx := curX + curPiece[k * 2];
    by := curY + curPiece[k * 2 + 1];
    if (bx >= 0) and (bx < BoardWidth) and (by >= 0) and (by < BoardHeight) then
      board[by * BoardWidth + bx] := curColor
  end
end;

procedure FlashLine(row: integer);
var
  x, i: integer;
begin
  { Flash effect with ascending tone }
  for i := 0 to 2 do
  begin
    snd_beep(400 + i * 200, 40);
    for x := 0 to BoardWidth - 1 do
      DrawBlock(x, row, ColorWhite);
    gfx_present;
    gfx_sleep(50);
    for x := 0 to BoardWidth - 1 do
      DrawBlock(x, row, board[row * BoardWidth + x]);
    gfx_present;
    gfx_sleep(50)
  end
end;

procedure ClearLines;
var
  y, x, full, dy, cleared: integer;
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
      FlashLine(y);
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
    { Increase speed every 10 lines }
    level := linesCleared div 10 + 1;
    if level > 10 then level := 10;
    dropSpeed := 25 - level * 2;
    if dropSpeed < 5 then dropSpeed := 5;

    DrawBoard;
    DrawInfo
  end
end;

procedure NewPiece;
begin
  SetPiece(nextType);
  nextType := Random7;
  curX := 4;
  curY := 0;

  if Collides = 1 then
    gameOver := 1
  else
  begin
    ShowPiece;
    DrawNextPiece
  end
end;

procedure TryMove(dx, dy: integer);
var
  oldX, oldY: integer;
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
      LockPiece;
      ClearLines;
      NewPiece
    end
    else if dx <> 0 then
      snd_beep(100, 20)  { Bump sound when can't move horizontally }
  end
  else
  begin
    if dx <> 0 then
      snd_beep(300, 15);  { Move sound }
    ShowPiece
  end
end;

procedure TryRotate;
var
  oldPiece: array[0..7] of integer;
  k: integer;
  rotated: integer;
begin
  { O piece doesn't rotate }
  if curType = 1 then exit;

  HidePiece;
  for k := 0 to 7 do
    oldPiece[k] := curPiece[k];

  RotateCW;
  rotated := 1;

  if Collides = 1 then
  begin
    for k := 0 to 7 do
      curPiece[k] := oldPiece[k];
    rotated := 0;
    snd_beep(100, 20)  { Can't rotate sound }
  end
  else
    snd_beep(500, 20);  { Rotate sound }

  ShowPiece
end;

procedure Drop;
begin
  snd_beep(800, 30);  { Whoosh sound for hard drop }
  HidePiece;
  while Collides = 0 do
    curY := curY + 1;
  curY := curY - 1;
  ShowPiece;
  LockPiece;
  score := score + 10;  { Bonus for dropping }
  ClearLines;
  NewPiece
end;

procedure HandleInput;
var
  ch: integer;
begin
  ch := gfx_read_key;
  if ch >= 0 then
  begin
    if ch = KeyUp then TryRotate
    else if ch = KeyDown then TryMove(0, 1)
    else if ch = KeyRight then TryMove(1, 0)
    else if ch = KeyLeft then TryMove(-1, 0)
    else if (ch = 119) or (ch = 87) then TryRotate       { W }
    else if (ch = 115) or (ch = 83) then TryMove(0, 1)   { S }
    else if (ch = 100) or (ch = 68) then TryMove(1, 0)   { D }
    else if (ch = 97) or (ch = 65) then TryMove(-1, 0)   { A }
    else if ch = 32 then Drop                            { Space }
    else if (ch = 113) or (ch = 81) then gameOver := 1   { Q }
  end
end;

procedure DrawGameOver;
var
  x, y, bx, by, i: integer;
begin
  { Play descending game over sound }
  for i := 0 to 4 do
  begin
    snd_beep(400 - i * 60, 100);
    gfx_sleep(120)
  end;

  { Draw "GAME OVER" pattern in center of board using blocks }
  { Draw a big X pattern }
  for y := 0 to 4 do
  begin
    bx := BoardLeft + (2 + y) * BlockSize;
    by := BoardTop + (7 + y) * BlockSize;
    gfx_fill_rect(bx, by, BlockSize, BlockSize, ColorRed);
    bx := BoardLeft + (7 - y) * BlockSize;
    gfx_fill_rect(bx, by, BlockSize, BlockSize, ColorRed)
  end;
  gfx_present
end;

procedure DrawSidebar;
begin
  { Clear entire sidebar area }
  gfx_fill_rect(BoardLeft + BoardWidth * BlockSize + 5, BoardTop,
                WinWidth - BoardLeft - BoardWidth * BlockSize - 10,
                BoardHeight * BlockSize, ColorBlack);

  { Next piece label area }
  gfx_rect(320, 60, 70, 20, ColorGray);

  { Next piece box }
  gfx_rect(320, 85, 70, 80, ColorGray);

  { Stats area }
  gfx_rect(320, 180, 70, 120, ColorGray)
end;

procedure DrawTitle;
var
  x, c: integer;
begin
  { Draw TETRIS title at top using colored blocks }
  for x := 0 to 6 do
  begin
    c := GetPieceColor(x);
    gfx_fill_rect(BoardLeft + 40 + x * 30, 8, 25, 25, c)
  end
end;

begin
  writeln('Starting Graphical Tetris...');

  if gfx_init(WinWidth, WinHeight) = 0 then
  begin
    writeln('Error: Could not initialize graphics');
    halt(1)
  end;

  { Initialize }
  score := 0;
  linesCleared := 0;
  level := 1;
  gameOver := 0;
  dropTimer := 0;
  dropSpeed := 20;
  rngState := 54321;  { Seed }
  nextType := Random7;

  { Clear board }
  for ix := 0 to 199 do
    board[ix] := 0;

  { Draw initial screen }
  gfx_clear(ColorBlack);
  DrawTitle;
  DrawBorder;
  DrawSidebar;
  DrawBoard;
  DrawInfo;
  DrawNextPiece;
  gfx_present;
  gfx_poll_events;

  { Startup jingle }
  snd_beep(262, 100); gfx_sleep(120);
  snd_beep(330, 100); gfx_sleep(120);
  snd_beep(392, 100); gfx_sleep(120);
  snd_beep(523, 200); gfx_sleep(250);

  NewPiece;
  gfx_present;

  { Game loop }
  while gameOver = 0 do
  begin
    HandleInput;

    dropTimer := dropTimer + 1;
    if dropTimer >= dropSpeed then
    begin
      TryMove(0, 1);
      dropTimer := 0
    end;

    gfx_present;
    gfx_sleep(25);

    if gfx_running = 0 then
      gameOver := 1
  end;

  { Game over }
  DrawGameOver;
  gfx_sleep(2000);

  gfx_close;

  writeln('Game Over!');
  write('Final Score: '); writeln(score);
  write('Lines: '); writeln(linesCleared);
  write('Level: '); writeln(level)
end.
