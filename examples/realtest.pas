program realtest;
var
  x, y, z: real;
  i: integer;
begin
  x := 3.14;
  y := 2.0;
  z := x + y;
  writeln(z);
  z := x * y;
  writeln(z);
  z := x - y;
  writeln(z);
  i := 5;
  z := i / 2;
  writeln(z);
  z := 10.5;
  writeln(z);
  if x > y then
    writeln(1)
  else
    writeln(0);
  z := -3.5;
  writeln(z)
end.
