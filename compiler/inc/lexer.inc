{ ----- Lexer ----- }

procedure NextChar;
begin
  if pushback_ch >= 0 then
  begin
    ch := pushback_ch;
    pushback_ch := -1
  end
  else
  begin
    ch := readchar;
    if ch = 10 then
    begin
      line_num := line_num + 1;
      col_num := 0
    end
    else
      col_num := col_num + 1
  end
end;

procedure SkipWhitespace;
begin
  while (ch = 32) or (ch = 9) or (ch = 10) or (ch = 13) do
    NextChar;
  { Skip comments }
  if ch = 123 then  { '{' }
  begin
    while (ch <> 125) and (ch <> -1) do
      NextChar;
    if ch = 125 then
      NextChar;
    SkipWhitespace
  end
end;

procedure NextToken;
var
  i: integer;
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
    end;
    { Check for decimal point (but not ..) }
    if ch = 46 then  { '.' }
    begin
      NextChar;
      if IsDigit(ch) = 1 then
      begin
        { This is a float literal }
        tok_type := TOK_FLOAT_LITERAL;
        tok_float_int := tok_int;
        tok_float_frac := 0;
        tok_float_neg := 0;
        { Parse fractional digits (up to 6 digits of precision) }
        i := 0;
        while (IsDigit(ch) = 1) and (i < 6) do
        begin
          tok_float_frac := tok_float_frac * 10 + (ch - 48);
          i := i + 1;
          NextChar
        end;
        { Scale frac to 6 digits }
        while i < 6 do
        begin
          tok_float_frac := tok_float_frac * 10;
          i := i + 1
        end;
        { Skip any remaining fractional digits }
        while IsDigit(ch) = 1 do
          NextChar
      end
      else
      begin
        { It was just an integer followed by '.', push back current char }
        { and set pushback to '.' so next NextToken gets DOT or DOTDOT }
        pushback_ch := ch;
        ch := 46  { Put '.' back as current so the DOT handling code gets it }
      end
    end
  end
  else if IsAlpha(ch) = 1 then
  begin
    tok_type := TOK_IDENT;
    tok_len := 0;
    while (IsAlpha(ch) = 1) or (IsDigit(ch) = 1) do
    begin
      if tok_len < 255 then
      begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      end;
      NextChar
    end;
    tok_str[tok_len] := 0;

    { Check for keywords }
    { This is simplified - would need proper keyword table }
    if tok_len = 7 then
      if (ToLower(tok_str[0]) = 112) and (ToLower(tok_str[1]) = 114) then { pr }
        if (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 103) then { og }
          if (ToLower(tok_str[4]) = 114) and (ToLower(tok_str[5]) = 97) then { ra }
            if ToLower(tok_str[6]) = 109 then { m }
              tok_type := TOK_PROGRAM;
    if tok_len = 5 then
      if (ToLower(tok_str[0]) = 98) and (ToLower(tok_str[1]) = 101) then { be }
        if (ToLower(tok_str[2]) = 103) and (ToLower(tok_str[3]) = 105) then { gi }
          if ToLower(tok_str[4]) = 110 then { n }
            tok_type := TOK_BEGIN;
    if tok_len = 3 then
    begin
      if (ToLower(tok_str[0]) = 101) and (ToLower(tok_str[1]) = 110) then { en }
        if ToLower(tok_str[2]) = 100 then { d }
          tok_type := TOK_END;
      if (ToLower(tok_str[0]) = 118) and (ToLower(tok_str[1]) = 97) then { va }
        if ToLower(tok_str[2]) = 114 then { r }
          tok_type := TOK_VAR;
      if (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 105) then { di }
        if ToLower(tok_str[2]) = 118 then { v }
          tok_type := TOK_DIV;
      if (ToLower(tok_str[0]) = 109) and (ToLower(tok_str[1]) = 111) then { mo }
        if ToLower(tok_str[2]) = 100 then { d }
          tok_type := TOK_MOD;
      if (ToLower(tok_str[0]) = 97) and (ToLower(tok_str[1]) = 110) then { an }
        if ToLower(tok_str[2]) = 100 then { d }
          tok_type := TOK_AND;
      if (ToLower(tok_str[0]) = 110) and (ToLower(tok_str[1]) = 111) then { no }
        if ToLower(tok_str[2]) = 116 then { t }
          tok_type := TOK_NOT;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 111) then { fo }
        if ToLower(tok_str[2]) = 114 then { r }
          tok_type := TOK_FOR;
      if (ToLower(tok_str[0]) = 110) and (ToLower(tok_str[1]) = 105) then { ni }
        if ToLower(tok_str[2]) = 108 then { l }
          tok_type := TOK_NIL
    end;
    if tok_len = 2 then
    begin
      if (ToLower(tok_str[0]) = 105) and (ToLower(tok_str[1]) = 102) then { if }
        tok_type := TOK_IF;
      if (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 111) then { do }
        tok_type := TOK_DO;
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 111) then { to }
        tok_type := TOK_TO;
      if (ToLower(tok_str[0]) = 111) and (ToLower(tok_str[1]) = 114) then { or }
        tok_type := TOK_OR;
      if (ToLower(tok_str[0]) = 111) and (ToLower(tok_str[1]) = 102) then { of }
        tok_type := TOK_OF
    end;
    if tok_len = 4 then
    begin
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 104) then { th }
        if (ToLower(tok_str[2]) = 101) and (ToLower(tok_str[3]) = 110) then { en }
          tok_type := TOK_THEN;
      if (ToLower(tok_str[0]) = 101) and (ToLower(tok_str[1]) = 108) then { el }
        if (ToLower(tok_str[2]) = 115) and (ToLower(tok_str[3]) = 101) then { se }
          tok_type := TOK_ELSE;
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 114) then { tr }
        if (ToLower(tok_str[2]) = 117) and (ToLower(tok_str[3]) = 101) then { ue }
          tok_type := TOK_TRUE
    end;
    if tok_len = 5 then
    begin
      if (ToLower(tok_str[0]) = 99) and (ToLower(tok_str[1]) = 111) then { co }
        if (ToLower(tok_str[2]) = 110) and (ToLower(tok_str[3]) = 115) then { ns }
          if ToLower(tok_str[4]) = 116 then { t }
            tok_type := TOK_CONST;
      if (ToLower(tok_str[0]) = 119) and (ToLower(tok_str[1]) = 104) then { wh }
        if (ToLower(tok_str[2]) = 105) and (ToLower(tok_str[3]) = 108) then { il }
          if ToLower(tok_str[4]) = 101 then { e }
            tok_type := TOK_WHILE;
      if (ToLower(tok_str[0]) = 117) and (ToLower(tok_str[1]) = 110) then { un }
        if (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 105) then { ti }
          if ToLower(tok_str[4]) = 108 then { l }
            tok_type := TOK_UNTIL;
      if (ToLower(tok_str[0]) = 97) and (ToLower(tok_str[1]) = 114) then { ar }
        if (ToLower(tok_str[2]) = 114) and (ToLower(tok_str[3]) = 97) then { ra }
          if ToLower(tok_str[4]) = 121 then { y }
            tok_type := TOK_ARRAY;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 97) then { fa }
        if (ToLower(tok_str[2]) = 108) and (ToLower(tok_str[3]) = 115) then { ls }
          if ToLower(tok_str[4]) = 101 then { e }
            tok_type := TOK_FALSE
    end;
    if tok_len = 6 then
    begin
      if (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) then { re }
        if (ToLower(tok_str[2]) = 112) and (ToLower(tok_str[3]) = 101) then { pe }
          if (ToLower(tok_str[4]) = 97) and (ToLower(tok_str[5]) = 116) then { at }
            tok_type := TOK_REPEAT;
      if (ToLower(tok_str[0]) = 100) and (ToLower(tok_str[1]) = 111) then { do }
        if (ToLower(tok_str[2]) = 119) and (ToLower(tok_str[3]) = 110) then { wn }
          if (ToLower(tok_str[4]) = 116) and (ToLower(tok_str[5]) = 111) then { to }
            tok_type := TOK_DOWNTO;
      if (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) then { re }
        if (ToLower(tok_str[2]) = 97) and (ToLower(tok_str[3]) = 100) then { ad }
          if (ToLower(tok_str[4]) = 108) and (ToLower(tok_str[5]) = 110) then { ln }
            tok_type := TOK_READLN;
      if (ToLower(tok_str[0]) = 115) and (ToLower(tok_str[1]) = 116) then { st }
        if (ToLower(tok_str[2]) = 114) and (ToLower(tok_str[3]) = 105) then { ri }
          if (ToLower(tok_str[4]) = 110) and (ToLower(tok_str[5]) = 103) then { ng }
            tok_type := TOK_STRING_TYPE;
      if (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) then { re }
        if (ToLower(tok_str[2]) = 99) and (ToLower(tok_str[3]) = 111) then { co }
          if (ToLower(tok_str[4]) = 114) and (ToLower(tok_str[5]) = 100) then { rd }
            tok_type := TOK_RECORD
    end;
    if tok_len = 7 then
    begin
      if (ToLower(tok_str[0]) = 105) and (ToLower(tok_str[1]) = 110) then { in }
        if (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 101) then { te }
          if (ToLower(tok_str[4]) = 103) and (ToLower(tok_str[5]) = 101) then { ge }
            if ToLower(tok_str[6]) = 114 then { r }
              tok_type := TOK_INTEGER_TYPE;
      if (ToLower(tok_str[0]) = 98) and (ToLower(tok_str[1]) = 111) then { bo }
        if (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 108) then { ol }
          if (ToLower(tok_str[4]) = 101) and (ToLower(tok_str[5]) = 97) then { ea }
            if ToLower(tok_str[6]) = 110 then { n }
              tok_type := TOK_BOOLEAN_TYPE;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 111) then { fo }
        if (ToLower(tok_str[2]) = 114) and (ToLower(tok_str[3]) = 119) then { rw }
          if (ToLower(tok_str[4]) = 97) and (ToLower(tok_str[5]) = 114) then { ar }
            if ToLower(tok_str[6]) = 100 then { d }
              tok_type := TOK_FORWARD
    end;
    if tok_len = 4 then
    begin
      if (ToLower(tok_str[0]) = 99) and (ToLower(tok_str[1]) = 104) then { ch }
        if (ToLower(tok_str[2]) = 97) and (ToLower(tok_str[3]) = 114) then { ar }
          tok_type := TOK_CHAR_TYPE;
      if (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) then { re }
        if (ToLower(tok_str[2]) = 97) and (ToLower(tok_str[3]) = 100) then { ad }
          tok_type := TOK_READ;
      if (ToLower(tok_str[0]) = 114) and (ToLower(tok_str[1]) = 101) then { re }
        if (ToLower(tok_str[2]) = 97) and (ToLower(tok_str[3]) = 108) then { al }
          tok_type := TOK_REAL_TYPE;
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 121) then { ty }
        if (ToLower(tok_str[2]) = 112) and (ToLower(tok_str[3]) = 101) then { pe }
          tok_type := TOK_TYPE_KW;
      if (ToLower(tok_str[0]) = 99) and (ToLower(tok_str[1]) = 97) then { ca }
        if (ToLower(tok_str[2]) = 115) and (ToLower(tok_str[3]) = 101) then { se }
          tok_type := TOK_CASE;
      if (ToLower(tok_str[0]) = 119) and (ToLower(tok_str[1]) = 105) then { wi }
        if (ToLower(tok_str[2]) = 116) and (ToLower(tok_str[3]) = 104) then { th }
          tok_type := TOK_WITH;
      if (ToLower(tok_str[0]) = 116) and (ToLower(tok_str[1]) = 101) then { te }
        if (ToLower(tok_str[2]) = 120) and (ToLower(tok_str[3]) = 116) then { xt }
          tok_type := TOK_TEXT;
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 105) then { fi }
        if (ToLower(tok_str[2]) = 108) and (ToLower(tok_str[3]) = 101) then { le }
          tok_type := TOK_FILE
    end;
    if tok_len = 9 then
      if (ToLower(tok_str[0]) = 112) and (ToLower(tok_str[1]) = 114) then { pr }
        if (ToLower(tok_str[2]) = 111) and (ToLower(tok_str[3]) = 99) then { oc }
          if (ToLower(tok_str[4]) = 101) and (ToLower(tok_str[5]) = 100) then { ed }
            if (ToLower(tok_str[6]) = 117) and (ToLower(tok_str[7]) = 114) then { ur }
              if ToLower(tok_str[8]) = 101 then { e }
                tok_type := TOK_PROCEDURE;
    if tok_len = 8 then
      if (ToLower(tok_str[0]) = 102) and (ToLower(tok_str[1]) = 117) then { fu }
        if (ToLower(tok_str[2]) = 110) and (ToLower(tok_str[3]) = 99) then { nc }
          if (ToLower(tok_str[4]) = 116) and (ToLower(tok_str[5]) = 105) then { ti }
            if (ToLower(tok_str[6]) = 111) and (ToLower(tok_str[7]) = 110) then { on }
              tok_type := TOK_FUNCTION
  end
  else if ch = 39 then  { single quote - string }
  begin
    tok_type := TOK_STRING;
    tok_len := 0;
    NextChar;
    while (ch <> 39) and (ch <> -1) do
    begin
      if tok_len < 255 then
      begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      end;
      NextChar
    end;
    tok_str[tok_len] := 0;
    if ch = 39 then
      NextChar
  end
  else if ch = 43 then  { + }
  begin
    tok_type := TOK_PLUS;
    NextChar
  end
  else if ch = 45 then  { - }
  begin
    tok_type := TOK_MINUS;
    NextChar
  end
  else if ch = 42 then  { * }
  begin
    tok_type := TOK_STAR;
    NextChar
  end
  else if ch = 47 then  { / }
  begin
    tok_type := TOK_SLASH;
    NextChar
  end
  else if ch = 61 then  { = }
  begin
    tok_type := TOK_EQ;
    NextChar
  end
  else if ch = 60 then  { < }
  begin
    NextChar;
    if ch = 62 then  { <> }
    begin
      tok_type := TOK_NEQ;
      NextChar
    end
    else if ch = 61 then  { <= }
    begin
      tok_type := TOK_LE;
      NextChar
    end
    else
      tok_type := TOK_LT
  end
  else if ch = 62 then  { > }
  begin
    NextChar;
    if ch = 61 then  { >= }
    begin
      tok_type := TOK_GE;
      NextChar
    end
    else
      tok_type := TOK_GT
  end
  else if ch = 40 then  { ( }
  begin
    tok_type := TOK_LPAREN;
    NextChar
  end
  else if ch = 41 then  { ) }
  begin
    tok_type := TOK_RPAREN;
    NextChar
  end
  else if ch = 91 then  { [ }
  begin
    tok_type := TOK_LBRACKET;
    NextChar
  end
  else if ch = 93 then  { ] }
  begin
    tok_type := TOK_RBRACKET;
    NextChar
  end
  else if ch = 58 then  { : }
  begin
    NextChar;
    if ch = 61 then  { := }
    begin
      tok_type := TOK_ASSIGN;
      NextChar
    end
    else
      tok_type := TOK_COLON
  end
  else if ch = 59 then  { ; }
  begin
    tok_type := TOK_SEMICOLON;
    NextChar
  end
  else if ch = 44 then  { , }
  begin
    tok_type := TOK_COMMA;
    NextChar
  end
  else if ch = 46 then  { . }
  begin
    NextChar;
    if ch = 46 then  { .. }
    begin
      tok_type := TOK_DOTDOT;
      NextChar
    end
    else
      tok_type := TOK_DOT
  end
  else if ch = 94 then  { ^ (caret) }
  begin
    tok_type := TOK_CARET;
    NextChar
  end
  else if ch = 64 then  { @ }
  begin
    tok_type := TOK_AT;
    NextChar
  end
  else
  begin
    Error(1);  { unexpected character }
    NextChar
  end
end;

