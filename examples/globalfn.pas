program GlobalFn;
var x: integer;

procedure SetX(val: integer);
begin
  x := val
end;

begin
  x := 0;
  writeln(x);
  SetX(42);
  writeln(x)
end.
