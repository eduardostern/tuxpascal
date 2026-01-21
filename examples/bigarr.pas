program BigArr;
var arr: array[0..63] of integer;
var x: integer;
begin
  x := 5;
  arr[0] := 100;
  arr[x] := 200;
  writeln(arr[0]);
  writeln(arr[5])
end.
