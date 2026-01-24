program GraphInvaders;
{ Space Invaders - Atari 2600 Authentic Recreation }
{ Faithful to the original 1980 Atari 2600 port }
{ Controls: Arrow keys or A/D to move, Space to shoot, Q to quit }

const
  { Atari 2600 had ~160x192 effective resolution, we scale 3x }
  WinWidth = 480;
  WinHeight = 576;
  Scale = 3;

  { Play area - matching Atari 2600 proportions }
  PlayLeft = 0;
  PlayTop = 48;
  PlayWidth = 480;
  PlayHeight = 528;

  { Atari 2600 had 6 rows x 6 columns = 36 aliens }
  AlienRows = 6;
  AlienCols = 6;
  AlienWidth = 24;   { 8 pixels * 3 }
  AlienHeight = 24;  { 8 pixels * 3 }
  AlienSpacingX = 48;
  AlienSpacingY = 30;

  { Player dimensions }
  PlayerY = 510;
  PlayerWidth = 39;   { 13 pixels * 3 }
  PlayerHeight = 24;  { 8 pixels * 3 }

  { Shields - Atari 2600 style blocky shields }
  ShieldCount = 4;
  ShieldWidth = 66;   { 22 pixels * 3 }
  ShieldHeight = 48;  { 16 pixels * 3 }

  { Atari 2600 color palette (approximate RGB) }
  ColorBlack = 0;
  ColorWhite = 16777215;
  ColorGreen = 4521796;     { Atari green #44E444 }
  ColorOrange = 16744448;   { For shields #FFA500 }
  ColorYellow = 13421568;   { Atari yellow #CCCC00 }
  ColorRed = 13369344;      { Atari red #CC0000 }
  ColorBlue = 6724044;      { Atari blue #6666CC }
  ColorCyan = 6750156;      { #66CCCC }

  { Key codes }
  KeyUp = 63232;
  KeyDown = 63233;
  KeyLeft = 63234;
  KeyRight = 63235;

var
  { Player }
  playerX: integer;
  playerLives: integer;
  playerDead: integer;
  deathTimer: integer;

  { Aliens: 1 = alive, 0 = dead - 6x6 grid }
  aliens: array[0..35] of integer;
  alienBaseX, alienBaseY: integer;
  alienDir: integer;
  alienMoveTimer: integer;
  alienMoveDelay: integer;
  aliensRemaining: integer;
  alienFrame: integer;
  alienStepSound: integer;

  { Player bullet - ONE at a time like original }
  bulletX, bulletY: integer;
  bulletActive: integer;

  { Alien bullets - max 1 like original Atari 2600 }
  aBulletX, aBulletY: integer;
  aBulletActive: integer;
  aBulletFrame: integer;
  alienShootTimer: integer;

  { UFO - mystery ship }
  ufoX: integer;
  ufoActive: integer;
  ufoDir: integer;
  ufoTimer: integer;
  ufoScoreTimer: integer;
  ufoScoreX: integer;
  ufoScoreVal: integer;

  { Shields - 4 shields, each 11x8 blocks (scaled) }
  shields: array[0..351] of integer;  { 4 * 11 * 8 = 352 }

  score: integer;
  gameOver: integer;
  wave: integer;

  rngState: integer;
  i, j: integer;

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

function RandomN(n: integer): integer;
begin
  rngState := rngState * 1103515245 + 12345;
  if rngState < 0 then rngState := 0 - rngState;
  RandomN := rngState mod n
end;

procedure DrawDigit(x, y, d, color: integer);
{ Atari 2600 style blocky digits - 4 pixels wide, 5 tall, scaled 3x }
begin
  if d = 0 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x, y, 3, 15, color);
    gfx_fill_rect(x+9, y, 3, 15, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
  else if d = 1 then
    gfx_fill_rect(x+9, y, 3, 15, color)
  else if d = 2 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x+9, y, 3, 9, color);
    gfx_fill_rect(x, y+6, 12, 3, color);
    gfx_fill_rect(x, y+6, 3, 9, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
  else if d = 3 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x+9, y, 3, 15, color);
    gfx_fill_rect(x, y+6, 12, 3, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
  else if d = 4 then
  begin
    gfx_fill_rect(x, y, 3, 9, color);
    gfx_fill_rect(x+9, y, 3, 15, color);
    gfx_fill_rect(x, y+6, 12, 3, color)
  end
  else if d = 5 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x, y, 3, 9, color);
    gfx_fill_rect(x, y+6, 12, 3, color);
    gfx_fill_rect(x+9, y+6, 3, 9, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
  else if d = 6 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x, y, 3, 15, color);
    gfx_fill_rect(x, y+6, 12, 3, color);
    gfx_fill_rect(x+9, y+6, 3, 9, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
  else if d = 7 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x+9, y, 3, 15, color)
  end
  else if d = 8 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x, y, 3, 15, color);
    gfx_fill_rect(x+9, y, 3, 15, color);
    gfx_fill_rect(x, y+6, 12, 3, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
  else if d = 9 then
  begin
    gfx_fill_rect(x, y, 12, 3, color);
    gfx_fill_rect(x, y, 3, 9, color);
    gfx_fill_rect(x+9, y, 3, 15, color);
    gfx_fill_rect(x, y+6, 12, 3, color);
    gfx_fill_rect(x, y+12, 12, 3, color)
  end
end;

procedure DrawNumber(x, y, num, digits, color: integer);
var
  n, d, px: integer;
begin
  n := num;
  px := x + (digits - 1) * 15;
  for d := 1 to digits do
  begin
    DrawDigit(px, y, n mod 10, color);
    n := n div 10;
    px := px - 15
  end
end;

procedure ClearNumber(x, y, digits: integer);
begin
  gfx_fill_rect(x, y, digits * 15, 15, ColorBlack)
end;

procedure DrawPlayer;
var
  px, py: integer;
begin
  if playerDead = 1 then exit;
  px := playerX;
  py := PlayerY;
  { Atari 2600 cannon shape - blocky }
  gfx_fill_rect(px + 15, py - 12, 9, 12, ColorGreen);  { Barrel }
  gfx_fill_rect(px + 6, py, 27, 6, ColorGreen);        { Top }
  gfx_fill_rect(px, py + 6, 39, 6, ColorGreen)         { Base }
end;

procedure ClearPlayer;
begin
  gfx_fill_rect(playerX, PlayerY - 12, PlayerWidth, PlayerHeight + 12, ColorBlack)
end;

procedure DrawPlayerExplosion(frame: integer);
var
  px, py, k: integer;
begin
  px := playerX;
  py := PlayerY;
  gfx_fill_rect(px, py - 12, 39, 24, ColorBlack);

  if frame mod 2 = 0 then
  begin
    { Explosion frame 1 }
    gfx_fill_rect(px + 3, py - 6, 6, 6, ColorGreen);
    gfx_fill_rect(px + 30, py - 6, 6, 6, ColorGreen);
    gfx_fill_rect(px + 15, py, 9, 6, ColorGreen);
    gfx_fill_rect(px + 6, py + 6, 6, 6, ColorGreen);
    gfx_fill_rect(px + 27, py + 6, 6, 6, ColorGreen)
  end
  else
  begin
    { Explosion frame 2 }
    gfx_fill_rect(px + 6, py - 9, 6, 6, ColorGreen);
    gfx_fill_rect(px + 27, py - 9, 6, 6, ColorGreen);
    gfx_fill_rect(px + 15, py - 3, 9, 6, ColorGreen);
    gfx_fill_rect(px + 3, py + 9, 6, 6, ColorGreen);
    gfx_fill_rect(px + 30, py + 9, 6, 6, ColorGreen)
  end
end;

procedure DrawAlien(row, col, show: integer);
var
  ax, ay: integer;
begin
  ax := alienBaseX + col * AlienSpacingX;
  ay := alienBaseY + row * AlienSpacingY;

  if show = 0 then
  begin
    gfx_fill_rect(ax, ay, AlienWidth, AlienHeight, ColorBlack);
    exit
  end;

  { Atari 2600 aliens were simple - all same color per row }
  { All aliens are white/light colored on 2600 }

  if alienFrame = 0 then
  begin
    { Frame 1 - arms down }
    gfx_fill_rect(ax + 6, ay, 12, 3, ColorWhite);
    gfx_fill_rect(ax + 3, ay + 3, 18, 3, ColorWhite);
    gfx_fill_rect(ax, ay + 6, 24, 3, ColorWhite);
    gfx_fill_rect(ax, ay + 9, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 9, ay + 9, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 18, ay + 9, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 3, ay + 12, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 15, ay + 12, 6, 3, ColorWhite);
    { Eyes }
    gfx_fill_rect(ax + 6, ay + 6, 3, 3, ColorBlack);
    gfx_fill_rect(ax + 15, ay + 6, 3, 3, ColorBlack)
  end
  else
  begin
    { Frame 2 - arms up }
    gfx_fill_rect(ax + 6, ay, 12, 3, ColorWhite);
    gfx_fill_rect(ax + 3, ay + 3, 18, 3, ColorWhite);
    gfx_fill_rect(ax, ay + 6, 24, 3, ColorWhite);
    gfx_fill_rect(ax, ay + 9, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 9, ay + 9, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 18, ay + 9, 6, 3, ColorWhite);
    gfx_fill_rect(ax, ay + 12, 6, 3, ColorWhite);
    gfx_fill_rect(ax + 18, ay + 12, 6, 3, ColorWhite);
    { Eyes }
    gfx_fill_rect(ax + 6, ay + 6, 3, 3, ColorBlack);
    gfx_fill_rect(ax + 15, ay + 6, 3, 3, ColorBlack)
  end
end;

procedure DrawAllAliens;
var
  r, c: integer;
begin
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      if aliens[r * AlienCols + c] = 1 then
        DrawAlien(r, c, 1)
end;

procedure DrawAlienExplosion(ax, ay: integer);
begin
  gfx_fill_rect(ax, ay, 24, 24, ColorBlack);
  { Explosion sprite }
  gfx_fill_rect(ax + 9, ay, 6, 3, ColorWhite);
  gfx_fill_rect(ax + 3, ay + 6, 6, 3, ColorWhite);
  gfx_fill_rect(ax + 15, ay + 6, 6, 3, ColorWhite);
  gfx_fill_rect(ax + 9, ay + 9, 6, 3, ColorWhite);
  gfx_fill_rect(ax, ay + 12, 6, 3, ColorWhite);
  gfx_fill_rect(ax + 18, ay + 12, 6, 3, ColorWhite);
  gfx_fill_rect(ax + 6, ay + 18, 6, 3, ColorWhite);
  gfx_fill_rect(ax + 12, ay + 18, 6, 3, ColorWhite)
end;

procedure DrawUFO;
begin
  if ufoActive = 1 then
  begin
    { Atari 2600 UFO - simple saucer shape }
    gfx_fill_rect(ufoX + 9, 60, 18, 6, ColorRed);
    gfx_fill_rect(ufoX + 3, 66, 30, 6, ColorRed);
    gfx_fill_rect(ufoX, 72, 36, 6, ColorRed);
    gfx_fill_rect(ufoX + 6, 78, 24, 3, ColorRed)
  end
end;

procedure ClearUFO;
begin
  gfx_fill_rect(ufoX, 60, 36, 24, ColorBlack)
end;

procedure DrawUFOScore;
begin
  if ufoScoreTimer > 0 then
    DrawNumber(ufoScoreX, 66, ufoScoreVal, 3, ColorRed)
end;

procedure InitShields;
var
  s, x, y, bi: integer;
begin
  for s := 0 to ShieldCount - 1 do
  begin
    for y := 0 to 7 do
    begin
      for x := 0 to 10 do
      begin
        bi := s * 88 + y * 11 + x;
        { Classic shield shape - rounded top, notch at bottom }
        if y = 0 then
        begin
          if (x >= 2) and (x <= 8) then
            shields[bi] := 1
          else
            shields[bi] := 0
        end
        else if y = 1 then
        begin
          if (x >= 1) and (x <= 9) then
            shields[bi] := 1
          else
            shields[bi] := 0
        end
        else if (y >= 6) and (x >= 4) and (x <= 6) then
          shields[bi] := 0  { Bottom notch }
        else
          shields[bi] := 1
      end
    end
  end
end;

procedure DrawShield(idx: integer);
var
  sx, sy, x, y, bi: integer;
begin
  sx := 30 + idx * 114;
  sy := 432;

  for y := 0 to 7 do
  begin
    for x := 0 to 10 do
    begin
      bi := idx * 88 + y * 11 + x;
      if shields[bi] = 1 then
        gfx_fill_rect(sx + x * 6, sy + y * 6, 6, 6, ColorOrange)
      else
        gfx_fill_rect(sx + x * 6, sy + y * 6, 6, 6, ColorBlack)
    end
  end
end;

procedure DrawAllShields;
var
  s: integer;
begin
  for s := 0 to ShieldCount - 1 do
    DrawShield(s)
end;

procedure DrawHUD;
var
  k, lx: integer;
begin
  { Clear score area }
  gfx_fill_rect(0, 0, WinWidth, 48, ColorBlack);

  { Score - Atari 2600 style at top left }
  DrawNumber(30, 15, score, 4, ColorWhite);

  { Lives - number and icons on right }
  DrawNumber(390, 15, playerLives, 1, ColorGreen);

  { Mini player icons }
  lx := 420;
  for k := 1 to playerLives - 1 do
  begin
    if k <= 3 then
    begin
      gfx_fill_rect(lx + 5, 15, 3, 4, ColorGreen);
      gfx_fill_rect(lx + 2, 19, 9, 2, ColorGreen);
      gfx_fill_rect(lx, 21, 13, 2, ColorGreen);
      lx := lx + 18
    end
  end;

  { Bottom line - Atari 2600 had a green line at bottom }
  gfx_fill_rect(0, 540, WinWidth, 3, ColorGreen)
end;

procedure InitAliens;
var
  r, c: integer;
begin
  aliensRemaining := AlienRows * AlienCols;
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      aliens[r * AlienCols + c] := 1;

  alienBaseX := 60;
  alienBaseY := 96;
  alienDir := 1;
  alienMoveTimer := 0;
  alienFrame := 0;
  alienStepSound := 0;

  { Speed based on wave }
  alienMoveDelay := 20 - wave * 2;
  if alienMoveDelay < 4 then alienMoveDelay := 4
end;

procedure InitGame;
begin
  playerX := WinWidth div 2 - PlayerWidth div 2;
  playerDead := 0;
  deathTimer := 0;
  bulletActive := 0;
  aBulletActive := 0;
  alienShootTimer := 0;
  ufoActive := 0;
  ufoTimer := 0;
  ufoScoreTimer := 0;

  InitShields;
  InitAliens
end;

procedure FireBullet;
begin
  { Only ONE bullet at a time - authentic Atari 2600 }
  if bulletActive = 0 then
  begin
    bulletActive := 1;
    bulletX := playerX + 18;
    bulletY := PlayerY - 15;
    snd_beep(1760, 30)  { High pitched shot sound }
  end
end;

function HitShield(x, y, fromAbove: integer): integer;
var
  s, sx, sy, lx, ly, bi: integer;
begin
  HitShield := 0;

  for s := 0 to ShieldCount - 1 do
  begin
    sx := 30 + s * 114;
    sy := 432;

    if (x >= sx) and (x < sx + 66) and (y >= sy) and (y < sy + 48) then
    begin
      lx := (x - sx) div 6;
      ly := (y - sy) div 6;

      if (lx >= 0) and (lx < 11) and (ly >= 0) and (ly < 8) then
      begin
        bi := s * 88 + ly * 11 + lx;
        if shields[bi] = 1 then
        begin
          shields[bi] := 0;
          { Damage adjacent blocks too for realistic erosion }
          if lx > 0 then
            if shields[s * 88 + ly * 11 + lx - 1] = 1 then
              if RandomN(2) = 0 then
                shields[s * 88 + ly * 11 + lx - 1] := 0;
          if lx < 10 then
            if shields[s * 88 + ly * 11 + lx + 1] = 1 then
              if RandomN(2) = 0 then
                shields[s * 88 + ly * 11 + lx + 1] := 0;
          DrawShield(s);
          HitShield := 1
        end
      end
    end
  end
end;

procedure UpdateBullet;
var
  r, c, ax, ay, pts: integer;
begin
  if bulletActive = 0 then exit;

  { Clear old }
  gfx_fill_rect(bulletX, bulletY, 3, 12, ColorBlack);

  bulletY := bulletY - 12;

  if bulletY < 60 then
  begin
    bulletActive := 0;
    exit
  end;

  { Check shield hit }
  if HitShield(bulletX + 1, bulletY, 0) = 1 then
  begin
    bulletActive := 0;
    snd_beep(110, 20);
    exit
  end;

  { Check UFO hit }
  if ufoActive = 1 then
  begin
    if (bulletY < 84) and (bulletY > 54) and
       (bulletX + 1 >= ufoX) and (bulletX + 1 <= ufoX + 36) then
    begin
      ClearUFO;
      ufoActive := 0;
      bulletActive := 0;

      { Atari 2600 UFO scores: 50, 100, or 200 }
      pts := (RandomN(3) + 1) * 50;
      if pts = 150 then pts := 200;
      score := score + pts;
      ufoScoreTimer := 40;
      ufoScoreX := ufoX;
      ufoScoreVal := pts;

      snd_beep(880, 50);
      snd_beep(440, 50);
      DrawHUD;
      exit
    end
  end;

  { Check alien hit }
  for r := 0 to AlienRows - 1 do
  begin
    for c := 0 to AlienCols - 1 do
    begin
      if aliens[r * AlienCols + c] = 1 then
      begin
        ax := alienBaseX + c * AlienSpacingX;
        ay := alienBaseY + r * AlienSpacingY;

        if (bulletX + 1 >= ax) and (bulletX + 1 <= ax + 24) and
           (bulletY >= ay) and (bulletY <= ay + 24) then
        begin
          aliens[r * AlienCols + c] := 0;
          aliensRemaining := aliensRemaining - 1;
          bulletActive := 0;

          { Show explosion }
          DrawAlienExplosion(ax, ay);
          snd_beep(220, 30);
          snd_noise(50);
          gfx_present;
          gfx_sleep(80);
          gfx_fill_rect(ax, ay, 24, 24, ColorBlack);

          { Atari 2600 scoring: 5-30 points based on row (bottom to top) }
          pts := (AlienRows - r) * 5;
          score := score + pts;
          DrawHUD;

          { Speed up }
          if aliensRemaining > 0 then
          begin
            alienMoveDelay := 2 + aliensRemaining div 4;
            if alienMoveDelay > 18 then alienMoveDelay := 18
          end;
          exit
        end
      end
    end
  end;

  { Draw bullet }
  gfx_fill_rect(bulletX, bulletY, 3, 12, ColorWhite)
end;

procedure FireAlienBullet;
var
  r, c, col, ax, ay: integer;
begin
  if aBulletActive = 1 then exit;

  { Pick random column with aliens }
  col := RandomN(AlienCols);

  { Find bottom alien in that column }
  for r := AlienRows - 1 downto 0 do
  begin
    if aliens[r * AlienCols + col] = 1 then
    begin
      ax := alienBaseX + col * AlienSpacingX;
      ay := alienBaseY + r * AlienSpacingY;
      aBulletX := ax + 10;
      aBulletY := ay + 24;
      aBulletActive := 1;
      aBulletFrame := 0;
      snd_beep(110, 15);
      exit
    end
  end
end;

procedure UpdateAlienBullet;
begin
  if aBulletActive = 0 then exit;

  { Clear old }
  gfx_fill_rect(aBulletX - 3, aBulletY, 9, 12, ColorBlack);

  aBulletY := aBulletY + 9;
  aBulletFrame := (aBulletFrame + 1) mod 4;

  if aBulletY > 540 then
  begin
    aBulletActive := 0;
    exit
  end;

  { Check shield hit }
  if HitShield(aBulletX, aBulletY + 9, 1) = 1 then
  begin
    aBulletActive := 0;
    snd_beep(110, 20);
    exit
  end;

  { Check player hit }
  if playerDead = 0 then
  begin
    if (aBulletY + 9 >= PlayerY - 6) and (aBulletY <= PlayerY + 12) then
    begin
      if (aBulletX >= playerX) and (aBulletX <= playerX + 39) then
      begin
        aBulletActive := 0;
        playerDead := 1;
        deathTimer := 30;
        playerLives := playerLives - 1;

        snd_noise(300);
        DrawHUD;

        if playerLives <= 0 then
          gameOver := 1;
        exit
      end
    end
  end;

  { Draw bullet - zigzag like Atari 2600 }
  if aBulletFrame < 2 then
    gfx_fill_rect(aBulletX - 3, aBulletY, 3, 9, ColorWhite)
  else
    gfx_fill_rect(aBulletX, aBulletY, 3, 9, ColorWhite)
end;

procedure UpdateUFO;
begin
  { Clear score display }
  if ufoScoreTimer > 0 then
  begin
    ufoScoreTimer := ufoScoreTimer - 1;
    DrawUFOScore;
    if ufoScoreTimer = 0 then
      gfx_fill_rect(ufoScoreX, 60, 60, 30, ColorBlack)
  end;

  if ufoActive = 0 then
  begin
    ufoTimer := ufoTimer + 1;
    if ufoTimer > 300 then
    begin
      ufoTimer := 0;
      if RandomN(100) < 15 then
      begin
        ufoActive := 1;
        if RandomN(2) = 0 then
        begin
          ufoX := 0;
          ufoDir := 4
        end
        else
        begin
          ufoX := WinWidth - 36;
          ufoDir := -4
        end;
        { UFO sound }
        snd_tone(880, 30)
      end
    end
  end
  else
  begin
    ClearUFO;
    ufoX := ufoX + ufoDir;

    { UFO warble sound }
    if (ufoX div 20) mod 2 = 0 then
      snd_tone(660, 15)
    else
      snd_tone(880, 15);

    if (ufoX < -36) or (ufoX > WinWidth) then
      ufoActive := 0
    else
      DrawUFO
  end
end;

procedure UpdateAliens;
var
  r, c, ax, ay, needDrop, leftmost, rightmost, bottomY: integer;
begin
  alienMoveTimer := alienMoveTimer + 1;
  if alienMoveTimer < alienMoveDelay then exit;
  alienMoveTimer := 0;

  { Toggle frame }
  alienFrame := 1 - alienFrame;

  { Classic Atari 2600 march sound - 4 descending tones }
  alienStepSound := (alienStepSound + 1) mod 4;
  if alienStepSound = 0 then snd_beep(120, 60)
  else if alienStepSound = 1 then snd_beep(100, 60)
  else if alienStepSound = 2 then snd_beep(80, 60)
  else snd_beep(60, 60);

  { Clear all }
  for r := 0 to AlienRows - 1 do
    for c := 0 to AlienCols - 1 do
      if aliens[r * AlienCols + c] = 1 then
        DrawAlien(r, c, 0);

  { Find bounds }
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
        ay := alienBaseY + r * AlienSpacingY;
        if ay > bottomY then bottomY := ay
      end
    end
  end;

  { Game over if aliens reach bottom }
  if bottomY >= 400 then
  begin
    gameOver := 1;
    exit
  end;

  { Check edges and move }
  needDrop := 0;
  if alienDir > 0 then
  begin
    ax := alienBaseX + rightmost * AlienSpacingX + 24;
    if ax >= WinWidth - 30 then needDrop := 1
  end
  else
  begin
    ax := alienBaseX + leftmost * AlienSpacingX;
    if ax <= 30 then needDrop := 1
  end;

  if needDrop = 1 then
  begin
    alienDir := 0 - alienDir;
    alienBaseY := alienBaseY + 18
  end
  else
    alienBaseX := alienBaseX + alienDir * 6;

  { Redraw }
  DrawAllAliens;

  { Alien shooting }
  alienShootTimer := alienShootTimer + 1;
  if alienShootTimer >= 8 then
  begin
    alienShootTimer := 0;
    if RandomN(100) < 25 then
      FireAlienBullet
  end
end;

procedure UpdatePlayer;
begin
  if playerDead = 1 then
  begin
    DrawPlayerExplosion(deathTimer);
    deathTimer := deathTimer - 1;
    if deathTimer <= 0 then
    begin
      playerDead := 0;
      ClearPlayer;
      playerX := WinWidth div 2 - PlayerWidth div 2;
      if playerLives > 0 then
        DrawPlayer
    end
  end
end;

procedure HandleInput;
var
  ch: integer;
begin
  if playerDead = 1 then exit;

  ch := gfx_read_key;
  if ch >= 0 then
  begin
    if (ch = KeyRight) or (ch = 100) or (ch = 68) then
    begin
      if playerX < WinWidth - PlayerWidth - 10 then
      begin
        ClearPlayer;
        playerX := playerX + 9;
        DrawPlayer
      end
    end
    else if (ch = KeyLeft) or (ch = 97) or (ch = 65) then
    begin
      if playerX > 10 then
      begin
        ClearPlayer;
        playerX := playerX - 9;
        DrawPlayer
      end
    end
    else if ch = 32 then
      FireBullet
    else if (ch = 113) or (ch = 81) then
      gameOver := 1
  end
end;

procedure NextWave;
begin
  wave := wave + 1;

  bulletActive := 0;
  aBulletActive := 0;
  ufoActive := 0;

  gfx_fill_rect(0, 60, WinWidth, 30, ColorBlack);

  InitAliens;

  { Aliens start lower each wave }
  alienBaseY := 96 + (wave - 1) * 24;
  if alienBaseY > 180 then alienBaseY := 180;

  { Victory sound }
  snd_beep(523, 100);
  gfx_sleep(120);
  snd_beep(659, 100);
  gfx_sleep(120);
  snd_beep(784, 100);
  gfx_sleep(120);
  snd_beep(1047, 200);
  gfx_sleep(300);

  DrawAllAliens
end;

procedure ShowGameOver;
var
  k: integer;
begin
  { Game over sound }
  for k := 0 to 5 do
  begin
    snd_beep(200 - k * 25, 200);
    gfx_sleep(220)
  end;

  { Flash GAME OVER }
  gfx_fill_rect(140, 260, 200, 60, ColorRed);
  gfx_fill_rect(145, 265, 190, 50, ColorBlack);

  { Score display }
  DrawNumber(190, 280, score, 4, ColorWhite);

  gfx_present;
  gfx_sleep(2000)
end;

{ Main program }
begin
  writeln('Starting Space Invaders (Atari 2600 Style)...');

  if gfx_init(WinWidth, WinHeight) = 0 then
  begin
    writeln('Error: Could not initialize graphics');
    halt(1)
  end;

  rngState := 31337;

  { Initialize game }
  score := 0;
  wave := 1;
  playerLives := 3;
  gameOver := 0;

  InitGame;

  gfx_clear(ColorBlack);
  DrawHUD;
  DrawAllShields;
  DrawAllAliens;
  DrawPlayer;
  gfx_present;
  gfx_poll_events;

  { Start sound }
  snd_beep(262, 80);
  gfx_sleep(100);
  snd_beep(330, 80);
  gfx_sleep(100);
  snd_beep(392, 150);
  gfx_sleep(200);

  { Main game loop }
  while gameOver = 0 do
  begin
    HandleInput;
    UpdatePlayer;
    UpdateBullet;
    UpdateAlienBullet;
    UpdateAliens;
    UpdateUFO;

    if aliensRemaining = 0 then
      NextWave;

    gfx_present;
    gfx_poll_events;
    gfx_sleep(25);

    if gfx_running = 0 then
      gameOver := 1
  end;

  ShowGameOver;

  gfx_close;

  writeln('Game Over!');
  write('Final Score: '); writeln(score);
  write('Wave: '); writeln(wave)
end.
