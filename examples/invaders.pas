program Invaders;
{ Space Invaders - Classic Atari 2600 Style }
{ Controls: Arrow keys or A/D to move, Space to shoot, Q to quit }

const
  ScreenWidth = 60;
  ScreenHeight = 24;
  PlayerY = 22;
  AlienRows = 5;
  AlienCols = 11;
  BunkerCount = 4;
  BunkerWidth = 6;
  BunkerHeight = 3;

var
  { Player }
  playerX: integer;
  playerLives: integer;

  { Aliens: 1 = alive, 0 = dead }
  aliens: array[0..54] of integer;  { 5 x 11 = 55 }
  alienBaseX, alienBaseY: integer;
  alienDir: integer;
  alienMoveTimer: integer;
  alienMoveDelay: integer;
  aliensRemaining: integer;
  alienFrame: integer;  { For animation }

  { Player bullet - only ONE at a time like original }
  bulletX: integer;
  bulletY: integer;
  bulletActive: integer;

  { Alien bullets - max 3 like original }
  aBulletX: array[0..2] of integer;
  aBulletY: array[0..2] of integer;
  aBulletActive: array[0..2] of integer;
  alienShootTimer: integer;

  { Mystery ship (UFO) }
  ufoX: integer;
  ufoActive: integer;
  ufoDir: integer;
  ufoTimer: integer;

  { Bunkers - 4 bunkers, each is a 6x3 grid of blocks }
  bunkers: array[0..71] of integer;  { 4 * 6 * 3 = 72 }

  score: integer;
  gameOver: integer;
  hiScore: integer;
  wave: integer;

  rngState: integer;
  i, j: integer;

function RandomN(n: integer): integer;
begin
  rngState := rngState * 1103515245 + 12345;
  if rngState < 0 then rngState := 0 - rngState;
  RandomN := rngState mod n
end;

procedure DrawChar(x, y: integer; ch: char; color: integer);
begin
  if (x >= 1) and (x <= ScreenWidth) and (y >= 1) and (y <= ScreenHeight) then
  begin
    GotoXY(x, y);
    TextColor(color);
    writechar(ch)
  end
end;

procedure ClearAt(x, y: integer);
begin
  if (x >= 1) and (x <= ScreenWidth) and (y >= 1) and (y <= ScreenHeight) then
  begin
    GotoXY(x, y);
    writechar(' ')
  end
end;

procedure DrawPlayer;
begin
  GotoXY(playerX - 1, PlayerY);
  TextColor(10);
  writechar('/');
  writechar('^');
  writechar(92)  { backslash }
end;

procedure ClearPlayer;
begin
  GotoXY(playerX - 1, PlayerY);
  write('   ')
end;

procedure DrawAlien(row, col: integer; show: integer);
var ax, ay: integer;
begin
  ax := alienBaseX + col * 4;
  ay := alienBaseY + row * 2;

  if (ax >= 1) and (ax + 2 <= ScreenWidth) and (ay >= 1) and (ay <= ScreenHeight) then
  begin
    GotoXY(ax, ay);
    if show = 1 then
    begin
      { Classic alien shapes - animate between frames }
      if row = 0 then
      begin
        TextColor(15);  { White - top row squid worth most }
        if alienFrame = 0 then
        begin
          writechar('/');
          writechar('O');
          writechar(92)  { backslash }
        end
        else
        begin
          writechar(92);  { backslash }
          writechar('O');
          writechar('/')
        end
      end
      else if (row = 1) or (row = 2) then
      begin
        TextColor(14);  { Yellow - middle rows crabs }
        if alienFrame = 0 then
          write('{#}')
        else
          write('[#]')
      end
      else
      begin
        TextColor(2);  { Green - bottom rows octopi }
        if alienFrame = 0 then
          write('<@>')
        else
          write('>@<')
      end
    end
    else
      write('   ')
  end
end;

procedure DrawAllAliens;
var r, c: integer;
begin
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      if aliens[r * AlienCols + c] = 1 then
        DrawAlien(r, c, 1)
end;

procedure DrawUFO;
begin
  if ufoActive = 1 then
  begin
    if (ufoX >= 1) and (ufoX + 4 <= ScreenWidth) then
    begin
      GotoXY(ufoX, 2);
      TextColor(5);  { Magenta }
      write('<==>')
    end
  end
end;

procedure ClearUFO;
begin
  if (ufoX >= 1) and (ufoX + 4 <= ScreenWidth) then
  begin
    GotoXY(ufoX, 2);
    write('    ')
  end
end;

procedure DrawBunker(idx: integer);
var bx, by, x, y, bi: integer;
begin
  bx := 6 + idx * 14;
  by := 18;

  for y := 0 to BunkerHeight - 1 do
  begin
    for x := 0 to BunkerWidth - 1 do
    begin
      bi := idx * BunkerWidth * BunkerHeight + y * BunkerWidth + x;
      GotoXY(bx + x, by + y);
      if bunkers[bi] = 1 then
      begin
        TextColor(2);
        writechar('#')
      end
      else
        writechar(' ')
    end
  end
end;

procedure DrawAllBunkers;
var b: integer;
begin
  for b := 0 to BunkerCount - 1 do
    DrawBunker(b)
end;

procedure DrawHUD;
var k: integer;
begin
  { Score - left side like original }
  GotoXY(1, 1);
  TextColor(15);
  write('SCORE<1>');
  GotoXY(1, 2);
  if score < 10 then write('000');
  if (score >= 10) and (score < 100) then write('00');
  if (score >= 100) and (score < 1000) then write('0');
  write(score);
  write('  ');

  { Hi-Score - center }
  GotoXY(24, 1);
  TextColor(15);
  write('HI-SCORE');
  GotoXY(24, 2);
  if hiScore < 10 then write('000');
  if (hiScore >= 10) and (hiScore < 100) then write('00');
  if (hiScore >= 100) and (hiScore < 1000) then write('0');
  write(hiScore);
  write('  ');

  { Lives - bottom left with ship icons }
  GotoXY(1, ScreenHeight);
  TextColor(10);
  write(playerLives);
  write(' ');
  for k := 1 to playerLives - 1 do
  begin
writechar('/');
    writechar('^');
    writechar(92);  { backslash }
    write(' ')
  end;
  write('         ');

  { Credit counter - bottom right like original }
  GotoXY(ScreenWidth - 10, ScreenHeight);
  TextColor(15);
  write('CREDIT 00')
end;

procedure InitBunkers;
var b, x, y, bi: integer;
begin
  for b := 0 to BunkerCount - 1 do
  begin
    for y := 0 to BunkerHeight - 1 do
    begin
      for x := 0 to BunkerWidth - 1 do
      begin
        bi := b * BunkerWidth * BunkerHeight + y * BunkerWidth + x;
        { Classic bunker shape - notch in bottom middle }
        if (y = BunkerHeight - 1) and (x >= 2) and (x <= 3) then
          bunkers[bi] := 0  { Notch }
        else
          bunkers[bi] := 1
      end
    end
  end
end;

procedure InitAliens;
var r, c: integer;
begin
  aliensRemaining := AlienRows * AlienCols;
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      aliens[r * AlienCols + c] := 1;

  alienBaseX := 3;
  alienBaseY := 4;
  alienDir := 1;
  alienMoveTimer := 0;
  alienFrame := 0;

  { Speed based on aliens remaining - classic mechanic }
  alienMoveDelay := 12
end;

procedure InitGame;
var k: integer;
begin
  playerX := ScreenWidth div 2;
  bulletActive := 0;

  for k := 0 to 2 do
    aBulletActive[k] := 0;

  alienShootTimer := 0;

  ufoActive := 0;
  ufoTimer := 0;

  InitBunkers;
  InitAliens
end;

procedure FireBullet;
begin
  { Only one bullet at a time - classic style }
  if bulletActive = 0 then
  begin
    bulletActive := 1;
    bulletX := playerX;
    bulletY := PlayerY - 1
  end
end;

function HitBunker(x, y: integer): integer;
var b, bx, by, lx, ly, bi: integer;
begin
  HitBunker := 0;

  for b := 0 to BunkerCount - 1 do
  begin
    bx := 6 + b * 14;
    by := 18;

    if (x >= bx) and (x < bx + BunkerWidth) and (y >= by) and (y < by + BunkerHeight) then
    begin
      lx := x - bx;
      ly := y - by;
      bi := b * BunkerWidth * BunkerHeight + ly * BunkerWidth + lx;

      if bunkers[bi] = 1 then
      begin
        bunkers[bi] := 0;
        DrawBunker(b);
        HitBunker := 1
      end
    end
  end
end;

procedure UpdateBullets;
var r, c, ax, ay, pts: integer;
begin
  if bulletActive = 1 then
  begin
    { Clear old position }
    ClearAt(bulletX, bulletY);

    { Move bullet up }
    bulletY := bulletY - 1;

    { Check if off screen }
    if bulletY < 2 then
      bulletActive := 0
    else if HitBunker(bulletX, bulletY) = 1 then
      bulletActive := 0
    else
    begin
      { Check UFO collision }
      if ufoActive = 1 then
      begin
        if (bulletY = 2) and (bulletX >= ufoX) and (bulletX <= ufoX + 3) then
        begin
          { Hit UFO! Mystery score }
          ClearUFO;
          ufoActive := 0;
          pts := (RandomN(3) + 1) * 50;  { 50, 100, or 150 }
          score := score + pts;
          if score > hiScore then hiScore := score;

          { Show score at UFO position }
          GotoXY(ufoX, 2);
          TextColor(5);
          write(pts);
          bulletActive := 0;
          DrawHUD
        end
      end;

      { Check alien collision }
      if bulletActive = 1 then
      begin
        for r := 0 to AlienRows - 1 do
        begin
          for c := 0 to AlienCols - 1 do
          begin
            if aliens[r * AlienCols + c] = 1 then
            begin
              ax := alienBaseX + c * 4;
              ay := alienBaseY + r * 2;
              if (bulletY = ay) and (bulletX >= ax) and (bulletX <= ax + 2) then
              begin
                { Hit alien! }
                aliens[r * AlienCols + c] := 0;
                aliensRemaining := aliensRemaining - 1;
                bulletActive := 0;

                { Quick explosion }
                GotoXY(ax, ay);
                TextColor(1);
                writechar(92);  { backslash }
                writechar('|');
                writechar('/');

                { Classic scoring: top=30, middle=20, bottom=10 }
                if r = 0 then pts := 30
                else if (r = 1) or (r = 2) then pts := 20
                else pts := 10;
                score := score + pts;
                if score > hiScore then hiScore := score;
                DrawHUD;

                Sleep(30);
                DrawAlien(r, c, 0);

                { Speed up remaining aliens - key classic mechanic }
                if aliensRemaining > 0 then
                begin
                  alienMoveDelay := 2 + (aliensRemaining div 5);
                  if alienMoveDelay > 12 then alienMoveDelay := 12
                end
              end
            end
          end
        end
      end;

      { Draw bullet if still active }
      if bulletActive = 1 then
        DrawChar(bulletX, bulletY, '|', 15)
    end
  end
end;

procedure UpdateAlienBullets;
var k: integer;
begin
  for k := 0 to 2 do
  begin
    if aBulletActive[k] = 1 then
    begin
      { Clear old position }
      ClearAt(aBulletX[k], aBulletY[k]);

      { Move bullet down }
      aBulletY[k] := aBulletY[k] + 1;

      { Check if off screen }
      if aBulletY[k] > ScreenHeight - 1 then
        aBulletActive[k] := 0
      else if HitBunker(aBulletX[k], aBulletY[k]) = 1 then
        aBulletActive[k] := 0
      else
      begin
        { Check player collision }
        if aBulletY[k] = PlayerY then
        begin
          if (aBulletX[k] >= playerX - 1) and (aBulletX[k] <= playerX + 1) then
          begin
            { Player hit! }
            aBulletActive[k] := 0;
            playerLives := playerLives - 1;

            if playerLives <= 0 then
              gameOver := 1
            else
            begin
              { Death animation - simple flash }
              GotoXY(playerX - 1, PlayerY);
              TextColor(1);
              write('XXX');
              Sleep(200);
              ClearPlayer;
              Sleep(300);
              DrawPlayer
            end;
            DrawHUD
          end
        end;

        { Draw bullet if still active - zigzag pattern like original }
        if aBulletActive[k] = 1 then
        begin
          if (aBulletY[k] mod 2) = 0 then
            DrawChar(aBulletX[k], aBulletY[k], ':', 15)
          else
            DrawChar(aBulletX[k], aBulletY[k], ';', 15)
        end
      end
    end
  end
end;

procedure FireAlienBullet(ax, ay: integer);
var k: integer;
begin
  for k := 0 to 2 do
  begin
    if aBulletActive[k] = 0 then
    begin
      aBulletActive[k] := 1;
      aBulletX[k] := ax;
      aBulletY[k] := ay;
      exit
    end
  end
end;

procedure AlienShoot;
var r, c, ax, ay, col: integer;
begin
  { Pick a random column with living aliens }
  col := RandomN(AlienCols);

  { Find bottom-most alien in that column }
  for r := AlienRows - 1 downto 0 do
  begin
    if aliens[r * AlienCols + col] = 1 then
    begin
      ax := alienBaseX + col * 4 + 1;
      ay := alienBaseY + r * 2 + 1;
      FireAlienBullet(ax, ay);
      exit
    end
  end
end;

procedure UpdateUFO;
begin
  { Spawn UFO periodically }
  if ufoActive = 0 then
  begin
    ufoTimer := ufoTimer + 1;
    if ufoTimer > 600 then  { Every ~10 seconds }
    begin
      ufoTimer := 0;
      if RandomN(100) < 30 then  { 30% chance }
      begin
        ufoActive := 1;
        if RandomN(2) = 0 then
        begin
          ufoX := 1;
          ufoDir := 1
        end
        else
        begin
          ufoX := ScreenWidth - 4;
          ufoDir := -1
        end
      end
    end
  end
  else
  begin
    { Move UFO }
    ClearUFO;
    ufoX := ufoX + ufoDir;

    if (ufoX < 1) or (ufoX > ScreenWidth - 4) then
      ufoActive := 0
    else
      DrawUFO
  end
end;

procedure UpdateAliens;
var r, c, ax, ay, needDrop, leftmost, rightmost, bottomY: integer;
begin
  alienMoveTimer := alienMoveTimer + 1;
  if alienMoveTimer < alienMoveDelay then exit;
  alienMoveTimer := 0;

  { Toggle animation frame }
  if alienFrame = 0 then alienFrame := 1 else alienFrame := 0;

  { Clear all aliens at current positions }
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      if aliens[r * AlienCols + c] = 1 then
        DrawAlien(r, c, 0);

  { Find leftmost and rightmost living aliens }
  leftmost := AlienCols;
  rightmost := -1;
  bottomY := 0;
  for r := 0 to AlienRows - 1 do
  begin
    for c := 0 to AlienCols - 1 do
    begin
      if aliens[r * AlienCols + c] = 1 then
      begin
        if c < leftmost then leftmost := c;
        if c > rightmost then rightmost := c;
        ay := alienBaseY + r * 2;
        if ay > bottomY then bottomY := ay
      end
    end
  end;

  { Check if aliens reached bunkers/player - game over }
  if bottomY >= 17 then
  begin
    gameOver := 1;
    exit
  end;

  { Check if need to drop and reverse - classic movement }
  needDrop := 0;
  if alienDir = 1 then
  begin
    ax := alienBaseX + rightmost * 4 + 2;
    if ax >= ScreenWidth - 2 then needDrop := 1
  end
  else
  begin
    ax := alienBaseX + leftmost * 4;
    if ax <= 2 then needDrop := 1
  end;

  if needDrop = 1 then
  begin
    alienDir := 0 - alienDir;
    alienBaseY := alienBaseY + 1
  end
  else
    alienBaseX := alienBaseX + alienDir * 2;

  { Redraw all aliens }
  DrawAllAliens;

  { Alien shooting }
  alienShootTimer := alienShootTimer + 1;
  if alienShootTimer >= 8 then
  begin
    alienShootTimer := 0;
    if RandomN(100) < 15 then  { Simple random shooting }
      AlienShoot
  end
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
            if ch2 = 67 then  { Right }
            begin
              if playerX < ScreenWidth - 2 then
              begin
                ClearPlayer;
                playerX := playerX + 1;
                DrawPlayer
              end
            end
            else if ch2 = 68 then  { Left }
            begin
              if playerX > 3 then
              begin
                ClearPlayer;
                playerX := playerX - 1;
                DrawPlayer
              end
            end
          end
        end
      end
    end
    else if (ch = 100) or (ch = 68) then  { D }
    begin
      if playerX < ScreenWidth - 2 then
      begin
        ClearPlayer;
        playerX := playerX + 1;
        DrawPlayer
      end
    end
    else if (ch = 97) or (ch = 65) then  { A }
    begin
      if playerX > 3 then
      begin
        ClearPlayer;
        playerX := playerX - 1;
        DrawPlayer
      end
    end
    else if ch = 32 then  { Space }
      FireBullet
    else if (ch = 113) or (ch = 81) then  { Q }
      gameOver := 1
  end
end;

procedure NextWave;
var k: integer;
begin
  wave := wave + 1;

  { Clear bullets }
  bulletActive := 0;
  for k := 0 to 2 do
    aBulletActive[k] := 0;

  { Clear UFO score display if any }
  GotoXY(1, 2);
  write('                    ');

  { Keep bunkers damaged - no restore in original }

  InitAliens;

  { Aliens start slightly lower each wave }
  alienBaseY := 4 + (wave - 1);
  if alienBaseY > 7 then alienBaseY := 7;

  DrawAllAliens
end;

procedure ShowTitle;
begin
  ClrScr;

  GotoXY(18, 4);
  TextColor(15);
  write('S P A C E');
  GotoXY(16, 6);
  write('I N V A D E R S');

  GotoXY(12, 9);
  TextColor(14);
  write('*SCORE ADVANCE TABLE*');

  GotoXY(18, 11);
  TextColor(5);
  write('<==>');
  TextColor(7);
  write(' = ? MYSTERY');

  GotoXY(18, 13);
  TextColor(15);
  writechar('/');
  writechar('O');
  writechar(92);  { backslash }
  TextColor(7);
  write(' = 30 POINTS');

  GotoXY(18, 15);
  TextColor(14);
  write('{#}');
  TextColor(7);
  write(' = 20 POINTS');

  GotoXY(18, 17);
  TextColor(2);
  write('<@>');
  TextColor(7);
  write(' = 10 POINTS');

  GotoXY(14, 20);
  TextColor(15);
  write('PRESS SPACE TO PLAY');

  GotoXY(12, 22);
  TextColor(7);
  write('A/D or Arrows = Move');
  GotoXY(16, 23);
  write('Space = Fire  Q = Quit');

  { Wait for space }
  repeat
    Sleep(50)
  until KeyPressed;
  i := readchar
end;

procedure ShowGameOver;
begin
  GotoXY(22, 10);
  TextColor(1);
  write('G A M E  O V E R');

  GotoXY(20, 13);
  TextColor(7);
  write('FINAL SCORE: ');
  TextColor(15);
  write(score);

  GotoXY(18, 16);
  TextColor(7);
  write('PRESS ANY KEY TO EXIT');

  while not KeyPressed do
    Sleep(50);
  i := readchar
end;

{ Main program }
begin
  ClrScr;
  HideCursor;
  InitKeyboard;

  rngState := 12345;
  hiScore := 0;

  ShowTitle;

  { Initialize game state }
  score := 0;
  wave := 1;
  playerLives := 3;
  gameOver := 0;

  InitGame;

  ClrScr;

  { Draw divider line like original }
  GotoXY(1, ScreenHeight - 1);
  TextColor(2);
  for i := 1 to ScreenWidth do
    writechar('_');

  DrawHUD;
  DrawAllBunkers;
  DrawAllAliens;
  DrawPlayer;

  { Main game loop }
  while gameOver = 0 do
  begin
    HandleInput;
    UpdateBullets;
    UpdateAlienBullets;
    UpdateAliens;
    UpdateUFO;

    { Check for wave complete }
    if aliensRemaining = 0 then
      NextWave;

    Sleep(16)  { ~60 FPS }
  end;

  ShowGameOver;

  DoneKeyboard;
  ShowCursor;
  GotoXY(1, ScreenHeight);
  NormVideo;
  writeln
end.
