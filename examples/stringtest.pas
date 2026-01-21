program StringTest;
var
  s1, s2, s3: string;
  i, code: integer;
  c: char;
begin
  writeln('=== String Test ===');

  { Basic assignment and length }
  s1 := 'Hello';
  writeln('s1 = Hello');
  write('length(s1) = ');
  writeln(length(s1));

  { String to string assignment }
  s2 := s1;
  write('s2 := s1, s2 = ');
  writeln(s2);

  { String comparison = and <> }
  if s1 = s2 then
    writeln('s1 = s2: true')
  else
    writeln('s1 = s2: false');

  s3 := 'World';
  if s1 <> s3 then
    writeln('s1 <> World: true')
  else
    writeln('s1 <> World: false');

  { String relational operators }
  s1 := 'apple';
  s2 := 'banana';
  if s1 < s2 then
    writeln('apple < banana: true')
  else
    writeln('apple < banana: false');

  if s2 > s1 then
    writeln('banana > apple: true')
  else
    writeln('banana > apple: false');

  s1 := 'abc';
  s2 := 'abc';
  if s1 <= s2 then
    writeln('abc <= abc: true')
  else
    writeln('abc <= abc: false');

  if s1 >= s2 then
    writeln('abc >= abc: true')
  else
    writeln('abc >= abc: false');

  { concat function and + operator }
  s1 := 'Hello';
  s2 := 'World';
  s3 := concat(s1, s2);
  write('concat(Hello, World) = ');
  writeln(s3);

  s3 := s1 + ' ' + s2;
  write('Hello + " " + World = ');
  writeln(s3);

  { copy function }
  s1 := 'Hello World';
  s2 := copy(s1, 1, 5);
  write('copy(Hello World, 1, 5) = ');
  writeln(s2);

  s2 := copy(s1, 7, 5);
  write('copy(Hello World, 7, 5) = ');
  writeln(s2);

  { String indexing read }
  s1 := 'ABCDE';
  write('s1[1] = ');
  writechar(s1[1]);
  writeln;
  write('s1[3] = ');
  writechar(s1[3]);
  writeln;
  write('s1[5] = ');
  writechar(s1[5]);
  writeln;

  { String indexing write }
  s1 := 'Hello';
  s1[1] := 'J';
  write('s1[1] := J, s1 = ');
  writeln(s1);

  s1[5] := 'y';
  write('s1[5] := y, s1 = ');
  writeln(s1);

  { pos function }
  s1 := 'Hello World';
  i := pos('World', s1);
  write('pos(World, Hello World) = ');
  writeln(i);

  i := pos('o', s1);
  write('pos(o, Hello World) = ');
  writeln(i);

  i := pos('xyz', s1);
  write('pos(xyz, Hello World) = ');
  writeln(i);

  { delete procedure }
  s1 := 'Hello World';
  delete(s1, 6, 6);
  write('delete(Hello World, 6, 6) = ');
  writeln(s1);

  s1 := 'ABCDEFGH';
  delete(s1, 3, 2);
  write('delete(ABCDEFGH, 3, 2) = ');
  writeln(s1);

  { insert procedure }
  s1 := 'Hello!';
  insert(' World', s1, 6);
  write('insert( World, Hello!, 6) = ');
  writeln(s1);

  s1 := 'AC';
  insert('B', s1, 2);
  write('insert(B, AC, 2) = ');
  writeln(s1);

  { str procedure }
  str(12345, s1);
  write('str(12345, s1), s1 = ');
  writeln(s1);

  str(-42, s1);
  write('str(-42, s1), s1 = ');
  writeln(s1);

  str(0, s1);
  write('str(0, s1), s1 = ');
  writeln(s1);

  { val procedure }
  s1 := '12345';
  val(s1, i, code);
  write('val(12345, i, code), i = ');
  write(i);
  write(', code = ');
  writeln(code);

  s1 := '-999';
  val(s1, i, code);
  write('val(-999, i, code), i = ');
  write(i);
  write(', code = ');
  writeln(code);

  s1 := '123abc';
  val(s1, i, code);
  write('val(123abc, i, code), i = ');
  write(i);
  write(', code = ');
  writeln(code);

  writeln('=== Test Complete ===')
end.
