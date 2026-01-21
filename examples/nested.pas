{ Nested procedures - note: accessing outer scope variables doesn't work yet }
program NestedTest;

procedure Outer(x: integer);

  procedure Inner(a: integer);
  begin
    writeln(a)
  end;

begin
  Inner(x);
  Inner(x * 2)
end;

begin
  Outer(5)
end.
