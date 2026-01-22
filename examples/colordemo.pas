program ColorDemo;
var i: integer;
begin
  ClrScr;
  GotoXY(1, 1);
  writeln('TuxPascal Color Demo');
  writeln('====================');
  writeln;

  for i := 0 to 7 do
  begin
    TextColor(i);
    write('Color ');
    write(i);
    write(' ');
    TextBackground(i);
    write(' Block ');
    NormVideo;
    writeln
  end;

  writeln;
  HighVideo;
  writeln('This is BOLD text');
  LowVideo;
  writeln('This is dim text');
  NormVideo;
  writeln('This is normal text');

  GotoXY(1, 20);
  writeln('Demo complete!')
end.
