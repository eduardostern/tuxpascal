program ProcTest;

var
  a: integer;

procedure PrintNum(n: integer);
begin
  writeln(n)
end;

function DoubleIt(x: integer): integer;
begin
  DoubleIt := x * 2
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
  a := DoubleIt(21);
  PrintNum(a);
  PrintNum(DoubleIt(10));
  writeln(Factorial(6))
end.
