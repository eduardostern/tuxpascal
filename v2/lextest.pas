{ Lexer test for v2 }
program LexTest;

var
  ch: integer;
  tok_type: integer;
  tok_int: integer;
  tok_str: array[0..63] of integer;
  tok_len: integer;
  line_num: integer;

const
  TOK_EOF = 0;
  TOK_IDENT = 1;
  TOK_INTEGER = 2;

function IsDigit(c: integer): integer;
begin
  if (c >= 48) and (c <= 57) then
    IsDigit := 1
  else
    IsDigit := 0
end;

function IsAlpha(c: integer): integer;
begin
  if ((c >= 65) and (c <= 90)) or ((c >= 97) and (c <= 122)) or (c = 95) then
    IsAlpha := 1
  else
    IsAlpha := 0
end;

procedure NextChar;
begin
  ch := readchar;
  if ch = 10 then
    line_num := line_num + 1
end;

procedure SkipWhitespace;
begin
  while (ch = 32) or (ch = 9) or (ch = 10) or (ch = 13) do
    NextChar
end;

procedure NextToken;
begin
  SkipWhitespace;

  if ch = -1 then
  begin
    tok_type := TOK_EOF;
    tok_len := 0
  end
  else if IsDigit(ch) = 1 then
  begin
    tok_type := TOK_INTEGER;
    tok_int := 0;
    while IsDigit(ch) = 1 do
    begin
      tok_int := tok_int * 10 + (ch - 48);
      NextChar
    end
  end
  else if IsAlpha(ch) = 1 then
  begin
    tok_type := TOK_IDENT;
    tok_len := 0;
    while (IsAlpha(ch) = 1) or (IsDigit(ch) = 1) do
    begin
      if tok_len < 63 then
      begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      end;
      NextChar
    end
  end
  else
  begin
    tok_type := ch;
    NextChar
  end
end;

begin
  line_num := 1;
  NextChar;
  NextToken;

  while tok_type <> TOK_EOF do
  begin
    writeln(tok_type);
    NextToken
  end;

  writeln(0)
end.
