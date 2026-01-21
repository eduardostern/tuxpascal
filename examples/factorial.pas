program Factorial;
var
  n, result, i: integer;
begin
  n := 10;
  result := 1;
  for i := 1 to n do
    result := result * i;
  write('10! = ');
  writeln(result)
end.
