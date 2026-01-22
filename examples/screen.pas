program ScreenDemo;
var i: integer;
begin
  ClrScr;
  for i := 1 to 10 do
  begin
    GotoXY(i, i);
    write('*')
  end;
  GotoXY(1, 12);
  writeln('Done!')
end.
