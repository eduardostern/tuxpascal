{ Graphics Demo for TuxPascal }
{ Demonstrates the TuxGraph library - pixel graphics using macOS native APIs }

program GraphDemo;

{ Constants must come before procedure/function declarations }
const
  { Colors in 0xRRGGBB format (decimal values) }
  Black   = 0;
  White   = 16777215;
  Red     = 16711680;
  Green   = 65280;
  Blue    = 255;
  Yellow  = 16776960;
  Cyan    = 65535;
  Magenta = 16711935;
  Orange  = 16744448;
  DarkBlue = 4144;        { was $001030 }
  Gray1   = 2105376;      { was $202020 for gradient }
  LightPurple = 8421631;  { was $8080FF }
  LightGreen = 8454016;   { was $80FF80 }

var
  x, y, i: integer;
  angle: real;

{ External functions from tuxgraph.o library }
function gfx_init(width, height: integer): integer; external;
procedure gfx_close; external;
procedure gfx_set_pixel(x, y, color: integer); external;
procedure gfx_set_pixel_rgb(x, y, r, g, b: integer); external;
procedure gfx_clear(color: integer); external;
procedure gfx_line(x1, y1, x2, y2, color: integer); external;
procedure gfx_rect(x, y, w, h, color: integer); external;
procedure gfx_fill_rect(x, y, w, h, color: integer); external;
procedure gfx_circle(cx, cy, r, color: integer); external;
procedure gfx_fill_circle(cx, cy, r, color: integer); external;
procedure gfx_present; external;
function gfx_running: integer; external;
procedure gfx_sleep(ms: integer); external;
function gfx_width: integer; external;
function gfx_height: integer; external;
function gfx_get_key: integer; external;

{ Sound functions }
procedure snd_beep(frequency, duration: integer); external;
procedure snd_tone(frequency, duration: integer); external;
procedure snd_noise(duration: integer); external;

begin
  writeln('TuxPascal Graphics Demo');
  writeln('=======================');
  writeln;

  { Initialize 800x600 window }
  if gfx_init(800, 600) = 0 then
  begin
    writeln('Error: Could not initialize graphics');
    halt(1)
  end;

  writeln('Window opened. Drawing with sound...');

  { Clear to dark blue }
  gfx_clear(DarkBlue);

  { Draw some filled rectangles with sounds }
  snd_beep(262, 80);
  gfx_fill_rect(50, 50, 150, 100, Red);
  gfx_present; gfx_sleep(100);
  snd_beep(330, 80);
  gfx_fill_rect(250, 50, 150, 100, Green);
  gfx_present; gfx_sleep(100);
  snd_beep(392, 80);
  gfx_fill_rect(450, 50, 150, 100, Blue);
  gfx_present; gfx_sleep(100);

  { Draw rectangle outlines }
  snd_beep(440, 80);
  gfx_rect(50, 200, 150, 100, Yellow);
  gfx_rect(250, 200, 150, 100, Cyan);
  gfx_rect(450, 200, 150, 100, Magenta);
  gfx_present; gfx_sleep(100);

  { Draw some circles }
  snd_beep(523, 80);
  gfx_fill_circle(125, 450, 60, Orange);
  gfx_present; gfx_sleep(100);
  snd_beep(587, 80);
  gfx_fill_circle(325, 450, 60, LightPurple);
  gfx_present; gfx_sleep(100);
  snd_beep(659, 80);
  gfx_fill_circle(525, 450, 60, LightGreen);
  gfx_present; gfx_sleep(100);

  { Draw circle outlines }
  gfx_circle(125, 450, 70, White);
  gfx_circle(325, 450, 70, White);
  gfx_circle(525, 450, 70, White);

  { Draw diagonal lines }
  for i := 0 to 7 do
  begin
    gfx_line(650, 50 + i * 60, 750, 100 + i * 60, White - i * Gray1);
  end;

  { Draw a gradient using individual pixels }
  for y := 350 to 399 do
    for x := 620 to 779 do
      gfx_set_pixel_rgb(x, y, (x - 620), (y - 350) * 5, 128);

  { Draw a simple sine wave }
  for x := 0 to 799 do
  begin
    angle := x * 3.14159 / 100.0;
    y := 550 + round(sin(angle) * 30.0);
    gfx_set_pixel(x, y, White);
  end;

  { Show the result }
  gfx_present;

  writeln('Drawing complete!');
  writeln('Press any key in the graphics window to exit...');

  { Wait for key press }
  i := gfx_get_key;

  { Clean up }
  gfx_close;
  writeln('Done.')
end.
