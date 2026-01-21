program RecTest;
type
  Point = record
    x: integer;
    y: integer
  end;
var
  p: Point;
  q: Point;
begin
  p.x := 10;
  p.y := 20;
  write('p.x = '); writeln(p.x);
  write('p.y = '); writeln(p.y);

  q.x := p.x + 5;
  q.y := p.y * 2;
  write('q.x = '); writeln(q.x);
  write('q.y = '); writeln(q.y);

  writeln('Record test complete')
end.
