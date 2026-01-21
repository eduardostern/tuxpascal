{ Test var parameters (pass by reference) }
program VarParamTest;

var
  a, b: integer;

procedure Swap(var x, y: integer);
var
  temp: integer;
begin
  temp := x;
  x := y;
  y := temp
end;

procedure Increment(var n: integer);
begin
  n := n + 1
end;

function AddAndDouble(var n: integer): integer;
begin
  n := n + 10;
  AddAndDouble := n * 2
end;

begin
  a := 5;
  b := 10;

  write('Before swap: a=');
  write(a);
  write(' b=');
  writeln(b);

  Swap(a, b);

  write('After swap: a=');
  write(a);
  write(' b=');
  writeln(b);

  Increment(a);
  write('After increment a: a=');
  writeln(a);

  write('AddAndDouble(a)=');
  write(AddAndDouble(a));
  write(' a=');
  writeln(a)
end.
