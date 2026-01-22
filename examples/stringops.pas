program StringOps;
var
  s1, s2, s3: string;
begin
  s1 := 'Hello';
  s2 := s1;
  writeln(length(s1));
  if s1 = s2 then
    writeln('equal')
  else
    writeln('not equal');
  s3 := concat(s1, s2);
  writeln(s3);
  s3 := s1 + s2;
  writeln(s3);
  s3 := copy(s1, 1, 3);
  writeln(s3)
end.
