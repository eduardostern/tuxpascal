{ Expression Parser Example for TuxPascal }
{ Parses and evaluates mathematical expressions with proper precedence }
{ Operators: +, -, *, /, ^ (power), unary -, parentheses }

program ExpressionParser;

var
  expr: string;
  idx: integer;
  ch: char;

{ Advance to next character }
procedure NextChar;
begin
  idx := idx + 1;
  if idx <= length(expr) then
    ch := expr[idx]
  else
    ch := chr(0)
end;

{ Skip whitespace }
procedure SkipSpaces;
begin
  while (ch = ' ') or (ch = chr(9)) do
    NextChar
end;

{ Compute base^exp for non-negative exponents }
function Power(base, exp: integer): integer;
var
  result, i: integer;
begin
  result := 1;
  for i := 1 to exp do
    result := result * base;
  Power := result
end;

{ Forward declarations }
function ParseExpression: integer; forward;

{ Parse an integer literal }
function ParseNumber: integer;
var
  n: integer;
begin
  n := 0;
  while (ch >= '0') and (ch <= '9') do
  begin
    n := n * 10 + (ord(ch) - ord('0'));
    NextChar
  end;
  ParseNumber := n
end;

{ Parse a factor: number or parenthesized expression }
function ParseFactor: integer;
var
  result: integer;
begin
  SkipSpaces;
  if ch = '(' then
  begin
    NextChar;
    result := ParseExpression;
    SkipSpaces;
    if ch = ')' then
      NextChar;
    ParseFactor := result
  end
  else
    ParseFactor := ParseNumber
end;

{ Parse unary minus }
function ParseUnary: integer;
begin
  SkipSpaces;
  if ch = '-' then
  begin
    NextChar;
    ParseUnary := -ParseUnary
  end
  else
    ParseUnary := ParseFactor
end;

{ Parse power operator (right-associative) }
function ParsePower: integer;
var
  base: integer;
begin
  base := ParseUnary;
  SkipSpaces;
  if ch = '^' then
  begin
    NextChar;
    ParsePower := Power(base, ParsePower)
  end
  else
    ParsePower := base
end;

{ Parse term: multiplication and division }
function ParseTerm: integer;
var
  result: integer;
  op: char;
begin
  result := ParsePower;
  SkipSpaces;
  while (ch = '*') or (ch = '/') do
  begin
    op := ch;
    NextChar;
    if op = '*' then
      result := result * ParsePower
    else
      result := result div ParsePower;
    SkipSpaces
  end;
  ParseTerm := result
end;

{ Parse expression: addition and subtraction }
function ParseExpression: integer;
var
  result: integer;
  op: char;
begin
  result := ParseTerm;
  SkipSpaces;
  while (ch = '+') or (ch = '-') do
  begin
    op := ch;
    NextChar;
    if op = '+' then
      result := result + ParseTerm
    else
      result := result - ParseTerm;
    SkipSpaces
  end;
  ParseExpression := result
end;

{ Main program }
begin
  readln(expr);
  idx := 0;
  NextChar;
  writeln(ParseExpression)
end.
