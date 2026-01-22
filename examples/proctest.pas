program ProcTest;

var
  a: integer;

procedure PrintNum(n: integer);
begin
  writeln(n)
end;

function Double(x: integer): integer;
begin
  Double := x * 2
end;

function Factorial(n: integer): integer;
begin
  if n <= 1 then
    Factorial := 1
  else
    Factorial := n * Factorial(n - 1)
end;

begin
  PrintNum(42);
  a := Double(21);
  PrintNum(a);
  PrintNum(Double(10));
  writeln(Factorial(6))
end.
