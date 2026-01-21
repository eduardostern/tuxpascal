program ArrayTest;
var
  arr: array[0..9] of integer;
  i: integer;
begin
  arr[0] := 42;
  i := arr[0];
  writeln(i)
end.
