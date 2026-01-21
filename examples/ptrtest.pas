program PtrTest;
var
  x, y: integer;
  p: ^integer;
  r: real;
  pr: ^real;
begin
  x := 42;
  p := @x;
  write('x = '); writeln(x);
  write('p^ = '); writeln(p^);

  p^ := 100;
  write('After p^ := 100, x = '); writeln(x);

  p := nil;
  write('p = nil: '); writeln(p);

  y := 200;
  p := @y;
  write('p^ pointing to y: '); writeln(p^);

  r := 3.14;
  pr := @r;
  write('Real via pointer: '); writeln(pr^);

  pr^ := 2.71;
  write('After pr^ := 2.71, r = '); writeln(r)
end.
