{ Test Pascal-style string variables }
program StringTest;

var
  greeting: string;
  name: string;

begin
  greeting := 'Hello, ';
  name := 'World!';

  write(greeting);
  writeln(name);

  greeting := 'Goodbye!';
  writeln(greeting)
end.
