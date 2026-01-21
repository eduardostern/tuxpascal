program Echo;
var
  c: integer;
begin
  c := readchar;
  while c <> -1 do
  begin
    writechar(c);
    c := readchar
  end
end.
