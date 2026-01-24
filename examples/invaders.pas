program Invaders;
{ Space Invaders clone for TuxPascal }
{ Controls: Arrow keys or A/D to move, Space to shoot, Q to quit }

const
  ScreenWidth = 60;
  ScreenHeight = 24;
  PlayerY = 22;
  AlienRows = 4;
  AlienCols = 8;
  MaxBullets = 3;
  MaxAlienBullets = 3;

var
  { Player }
  playerX: integer;
  playerLives: integer;

  { Aliens: 1 = alive, 0 = dead - stored in 32-element array }
  aliens: array[0..31] of integer;  { 4 x 8 }
  alienBaseX, alienBaseY: integer;
  alienDir: integer;  { 1 = right, -1 = left }
  alienMoveTimer: integer;
  alienMoveDelay: integer;
  aliensRemaining: integer;

  { Player bullets }
  bulletX: array[0..2] of integer;
  bulletY: array[0..2] of integer;
  bulletActive: array[0..2] of integer;

  { Alien bullets }
  aBulletX: array[0..2] of integer;
  aBulletY: array[0..2] of integer;
  aBulletActive: array[0..2] of integer;
  alienShootTimer: integer;

  score: integer;
  level: integer;
  gameOver: integer;

  rngState: integer;
  i: integer;

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
  TextColor(2);  { Green }
  GotoXY(playerX - 1, PlayerY);
  write('/');
  TextColor(10);
  writechar('A');
  TextColor(2);
  writechar('\')
end;

procedure ClearPlayer;
begin
  GotoXY(playerX - 1, PlayerY);
  write('   ')
end;

procedure DrawAlien(row, col: integer; show: integer);
var ax, ay: integer;
begin
  ax := alienBaseX + col * 5;
  ay := alienBaseY + row * 2;

  if (ax >= 1) and (ax + 2 <= ScreenWidth) and (ay >= 1) and (ay <= ScreenHeight) then
  begin
    GotoXY(ax, ay);
    if show = 1 then
    begin
      { Different alien types by row }
      if row = 0 then
      begin
        TextColor(5);  { Magenta - top row worth most }
        write('{@}')
      end
      else if row = 1 then
      begin
        TextColor(6);  { Cyan }
        write('<O>')
      end
      else
      begin
        TextColor(3);  { Yellow }
        write('[=]')
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

procedure DrawHUD;
begin
  NormVideo;
  GotoXY(1, 1);
  TextColor(7);
  write('SCORE: ');
  TextColor(15);
  write(score);
  write('    ');

  GotoXY(25, 1);
  TextColor(7);
  write('LEVEL: ');
  TextColor(15);
  write(level);
  write('  ');

  GotoXY(42, 1);
  TextColor(7);
  write('LIVES: ');
  TextColor(1);
  write(playerLives);
  write(' ')
end;

procedure InitAliens;
var r, c: integer;
begin
  aliensRemaining := AlienRows * AlienCols;
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      aliens[r * AlienCols + c] := 1;

  alienBaseX := 5;
  alienBaseY := 4;
  alienDir := 1;
  alienMoveTimer := 0;
  alienMoveDelay := 25 - level * 2;
  if alienMoveDelay < 5 then alienMoveDelay := 5
end;

procedure InitGame;
var k: integer;
begin
  playerX := ScreenWidth div 2;

  for k := 0 to MaxBullets - 1 do
    bulletActive[k] := 0;

  for k := 0 to MaxAlienBullets - 1 do
    aBulletActive[k] := 0;

  alienShootTimer := 0;

  InitAliens
end;

procedure FireBullet;
var k: integer;
begin
  for k := 0 to MaxBullets - 1 do
  begin
    if bulletActive[k] = 0 then
    begin
      bulletActive[k] := 1;
      bulletX[k] := playerX;
      bulletY[k] := PlayerY - 1;
      exit
    end
  end
end;

procedure FireAlienBullet(ax, ay: integer);
var k: integer;
begin
  for k := 0 to MaxAlienBullets - 1 do
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

procedure UpdateBullets;
var k, r, c, ax, ay, pts: integer;
begin
  for k := 0 to MaxBullets - 1 do
  begin
    if bulletActive[k] = 1 then
    begin
      { Clear old position }
      ClearAt(bulletX[k], bulletY[k]);

      { Move bullet up }
      bulletY[k] := bulletY[k] - 1;

      { Check if off screen }
      if bulletY[k] < 2 then
        bulletActive[k] := 0
      else
      begin
        { Check alien collision }
        for r := 0 to AlienRows - 1 do
        begin
          for c := 0 to AlienCols - 1 do
          begin
            if aliens[r * AlienCols + c] = 1 then
            begin
              ax := alienBaseX + c * 5;
              ay := alienBaseY + r * 2;
              if (bulletY[k] = ay) and (bulletX[k] >= ax) and (bulletX[k] <= ax + 2) then
              begin
                { Hit alien! }
                aliens[r * AlienCols + c] := 0;
                aliensRemaining := aliensRemaining - 1;
                bulletActive[k] := 0;
                DrawAlien(r, c, 0);

                { Score based on row }
                if r = 0 then pts := 30
                else if r = 1 then pts := 20
                else pts := 10;
                score := score + pts;
                DrawHUD;

                { Speed up remaining aliens }
                if aliensRemaining > 0 then
                begin
                  alienMoveDelay := 25 - level * 2 - (32 - aliensRemaining) div 2;
                  if alienMoveDelay < 2 then alienMoveDelay := 2
                end
              end
            end
          end
        end;

        { Draw bullet if still active }
        if bulletActive[k] = 1 then
          DrawChar(bulletX[k], bulletY[k], '|', 15)
      end
    end
  end
end;

procedure UpdateAlienBullets;
var k: integer;
begin
  for k := 0 to MaxAlienBullets - 1 do
  begin
    if aBulletActive[k] = 1 then
    begin
      { Clear old position }
      ClearAt(aBulletX[k], aBulletY[k]);

      { Move bullet down }
      aBulletY[k] := aBulletY[k] + 1;

      { Check if off screen }
      if aBulletY[k] > ScreenHeight then
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
            DrawHUD;

            if playerLives <= 0 then
              gameOver := 1
            else
            begin
              { Flash player }
              TextColor(1);
              GotoXY(playerX - 1, PlayerY);
              write('XXX');
              Sleep(200);
              DrawPlayer
            end
          end
        end;

        { Draw bullet if still active }
        if aBulletActive[k] = 1 then
          DrawChar(aBulletX[k], aBulletY[k], '*', 1)
      end
    end
  end
end;

procedure UpdateAliens;
var r, c, ax, ay, needDrop, leftmost, rightmost, bottomY: integer;
    shooterCol, foundShooter: integer;
begin
  alienMoveTimer := alienMoveTimer + 1;
  if alienMoveTimer < alienMoveDelay then exit;
  alienMoveTimer := 0;

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

  { Check if aliens reached player level }
  if bottomY >= PlayerY - 2 then
  begin
    gameOver := 1;
    exit
  end;

  { Check if need to drop and reverse }
  needDrop := 0;
  if alienDir = 1 then
  begin
    ax := alienBaseX + rightmost * 5 + 2;
    if ax >= ScreenWidth - 2 then needDrop := 1
  end
  else
  begin
    ax := alienBaseX + leftmost * 5;
    if ax <= 3 then needDrop := 1
  end;

  if needDrop = 1 then
  begin
    alienDir := 0 - alienDir;
    alienBaseY := alienBaseY + 1
  end
  else
    alienBaseX := alienBaseX + alienDir;

  { Redraw all aliens }
  DrawAllAliens;

  { Random alien shoots }
  alienShootTimer := alienShootTimer + 1;
  if alienShootTimer >= 3 then
  begin
    alienShootTimer := 0;
    if RandomN(100) < 25 then
    begin
      { Find bottom-most alien in a random column to shoot }
      foundShooter := 0;
      shooterCol := RandomN(AlienCols);
      for r := AlienRows - 1 downto 0 do
      begin
        if foundShooter = 0 then
        begin
          if aliens[r * AlienCols + shooterCol] = 1 then
          begin
            ax := alienBaseX + shooterCol * 5 + 1;
            ay := alienBaseY + r * 2 + 1;
            FireAlienBullet(ax, ay);
            foundShooter := 1
          end
        end
      end
    end
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

procedure NextLevel;
var k: integer;
begin
  level := level + 1;

  { Clear bullets }
  for k := 0 to MaxBullets - 1 do
    bulletActive[k] := 0;
  for k := 0 to MaxAlienBullets - 1 do
    aBulletActive[k] := 0;

  InitAliens;

  { Clear play area }
  ClrScr;

  { Redraw everything }
  DrawHUD;
  DrawAllAliens;
  DrawPlayer;

  { Brief pause }
  GotoXY(22, 12);
  TextColor(15);
  write('LEVEL ');
  write(level);
  Sleep(1000);
  GotoXY(22, 12);
  write('         ')
end;

procedure ShowTitle;
begin
  ClrScr;

  GotoXY(18, 4);
  TextColor(2);
  write(' ___  ___   __    ___  ____');
  GotoXY(18, 5);
  write('/ __)| _ \\ / _\\  / __|| ___)');
  GotoXY(18, 6);
  write('\\__ \\|  _//    \\( (__ | _)');
  GotoXY(18, 7);
  write('(___/|_|  \\_/\\_/ \\___)|____)');

  GotoXY(14, 9);
  TextColor(6);
  write(' __  __ _  _  _   __   ___   ____  ___   ___');
  GotoXY(14, 10);
  write('(  )(  ( \\/ )( \\ / _\\ |   \\ | ___|| _ \\ / __)');
  GotoXY(14, 11);
  write(' )( |     |\\ \\/ //    \\| |) || _)  |   / \\__ \\');
  GotoXY(14, 12);
  write('(__)|_|\\_| \\__/ \\_/\\_/|___/ |____||_\\_\\ (___/');

  GotoXY(18, 15);
  TextColor(7);
  write('Controls: A/D or Arrow Keys to Move');
  GotoXY(23, 16);
  write('Space to Shoot, Q to Quit');

  GotoXY(17, 18);
  TextColor(5);
  write('{@}');
  TextColor(7);
  write('=30  ');
  TextColor(6);
  write('<O>');
  TextColor(7);
  write('=20  ');
  TextColor(3);
  write('[=]');
  TextColor(7);
  write('=10 points');

  GotoXY(20, 21);
  TextColor(15);
  write('Press any key to start...');

  { Wait for keypress }
  while not KeyPressed do
    Sleep(50);
  i := readchar
end;

procedure ShowGameOver;
begin
  GotoXY(22, 10);
  TextColor(1);
  HighVideo;
  write('  GAME OVER!  ');
  NormVideo;

  GotoXY(22, 12);
  TextColor(7);
  write('Final Score: ');
  TextColor(15);
  write(score);

  GotoXY(22, 14);
  TextColor(7);
  write('Level Reached: ');
  TextColor(15);
  write(level);

  GotoXY(18, 17);
  TextColor(7);
  write('Press any key to exit...');

  while not KeyPressed do
    Sleep(50);
  i := readchar
end;

{ Main program }
begin
  ClrScr;
  HideCursor;
  InitKeyboard;

  rngState := 31337;

  ShowTitle;

  { Initialize game state }
  score := 0;
  level := 1;
  playerLives := 3;
  gameOver := 0;

  InitGame;

  ClrScr;
  DrawHUD;
  DrawAllAliens;
  DrawPlayer;

  { Main game loop }
  while gameOver = 0 do
  begin
    HandleInput;
    UpdateBullets;
    UpdateAlienBullets;
    UpdateAliens;

    { Check for level complete }
    if aliensRemaining = 0 then
      NextLevel;

    Sleep(16)  { ~60 FPS }
  end;

  ShowGameOver;

  DoneKeyboard;
  ShowCursor;
  GotoXY(1, ScreenHeight);
  NormVideo;
  writeln
end.
