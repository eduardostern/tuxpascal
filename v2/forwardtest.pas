program ForwardTest;

procedure A; forward;
procedure B; forward;

procedure A;
begin
  writeln(1)
end;

procedure B;
begin
  writeln(2)
end;

begin
  A;
  B
end.
