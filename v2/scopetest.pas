program ScopeTest;
var
  arr: array[0..9] of integer;
  i: integer;

procedure Test;
begin
  if arr[0] < i then
    writeln(1)
  else
    writeln(0)
end;

begin
  arr[0] := 5;
  i := 10;
  Test
end.
