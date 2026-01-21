{ Test array access }
program ArrayTest;

var
  arr: array[1..5] of integer;
  i: integer;

begin
  { Initialize array }
  arr[1] := 10;
  arr[2] := 20;
  arr[3] := 30;
  arr[4] := 40;
  arr[5] := 50;

  { Print array elements }
  writeln(arr[1]);
  writeln(arr[3]);
  writeln(arr[5]);

  { Access with variable index }
  i := 2;
  writeln(arr[i]);

  { Compute with array element }
  writeln(arr[1] + arr[2])
end.
