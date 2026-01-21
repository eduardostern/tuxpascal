program BigTest;
var
  a: integer;
  b: integer;
  arr1: array[0..9] of integer;
  c: integer;
  arr2: array[0..9] of integer;
  d: integer;

procedure TestForward; forward;

procedure DoTest;
begin
  if arr1[0] < c then
    writeln(1)
  else
    writeln(0)
end;

procedure TestForward;
begin
  if arr2[0] < d then
    writeln(2)
  else
    writeln(3)
end;

begin
  a := 1;
  b := 2;
  arr1[0] := 5;
  c := 10;
  arr2[0] := 15;
  d := 20;
  DoTest;
  TestForward
end.
