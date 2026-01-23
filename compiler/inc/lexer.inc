{ ----- Lexer ----- }

{ Check If current include file is at EOF }
Function IncludeEof: Integer;
Begin
  IncludeEof := 0;
  If include_depth = 1 Then
  Begin If eof(include_file0) Then IncludeEof := 1 End
  Else If include_depth = 2 Then
  Begin If eof(include_file1) Then IncludeEof := 1 End
  Else If include_depth = 3 Then
  Begin If eof(include_file2) Then IncludeEof := 1 End
  Else If include_depth = 4 Then
  Begin If eof(include_file3) Then IncludeEof := 1 End
  Else If include_depth = 5 Then
  Begin If eof(include_file4) Then IncludeEof := 1 End
  Else If include_depth = 6 Then
  Begin If eof(include_file5) Then IncludeEof := 1 End
  Else If include_depth = 7 Then
  Begin If eof(include_file6) Then IncludeEof := 1 End
  Else If include_depth = 8 Then
  Begin If eof(include_file7) Then IncludeEof := 1 End
End;

{ Read a character from current include file }
Function IncludeRead: Integer;
Var
  c: Integer;
Begin
  c := -1;
  If include_depth = 1 Then
    Read(include_file0, c)
  Else If include_depth = 2 Then
    Read(include_file1, c)
  Else If include_depth = 3 Then
    Read(include_file2, c)
  Else If include_depth = 4 Then
    Read(include_file3, c)
  Else If include_depth = 5 Then
    Read(include_file4, c)
  Else If include_depth = 6 Then
    Read(include_file5, c)
  Else If include_depth = 7 Then
    Read(include_file6, c)
  Else If include_depth = 8 Then
    Read(include_file7, c);
  IncludeRead := c
End;

{ Close current include file }
Procedure IncludeClose;
Begin
  If include_depth = 1 Then
    close(include_file0)
  Else If include_depth = 2 Then
    close(include_file1)
  Else If include_depth = 3 Then
    close(include_file2)
  Else If include_depth = 4 Then
    close(include_file3)
  Else If include_depth = 5 Then
    close(include_file4)
  Else If include_depth = 6 Then
    close(include_file5)
  Else If include_depth = 7 Then
    close(include_file6)
  Else If include_depth = 8 Then
    close(include_file7)
End;

{ Read a single character from stdin Or current include file }
Function ReadCurrentChar: Integer;
Begin
  If include_depth = 0 Then
    ReadCurrentChar := ReadChar
  Else
  Begin
    If IncludeEof = 1 Then
      ReadCurrentChar := -1
    Else
      ReadCurrentChar := IncludeRead
  End
End;

{ Pop back To parent include file }
Procedure PopIncludeFile;
Begin
  If include_depth > 0 Then
  Begin
    IncludeClose;
    include_depth := include_depth - 1;
    { Restore parent file state }
    If include_depth > 0 Then
    Begin
      line_num := include_line[include_depth - 1];
      col_num := include_col[include_depth - 1];
      ch := include_ch[include_depth - 1];
      pushback_ch := include_pushback[include_depth - 1]
    End
  End
End;

Procedure NextChar;
Var
  c: Integer;
Begin
  If pushback_ch >= 0 Then
  Begin
    ch := pushback_ch;
    pushback_ch := -1
  End
  Else
  Begin
    c := ReadCurrentChar;
    { If EOF In include file, pop back To parent }
    While (c = -1) And (include_depth > 0) Do
    Begin
      PopIncludeFile;
      If include_depth = 0 Then
        c := ReadChar
      Else
        c := ReadCurrentChar
    End;
    ch := c;
    If ch = 10 Then
    Begin
      line_num := line_num + 1;
      col_num := 0
    End
    Else
      col_num := col_num + 1
  End
End;

{ Open include file using filename from tok_str at given position }
Procedure IncludeOpenFromTokStr(start, len: Integer);
Begin
  If include_depth = 1 Then
  Begin assigntokstr(include_file0, start, len); reset(include_file0) End
  Else If include_depth = 2 Then
  Begin assigntokstr(include_file1, start, len); reset(include_file1) End
  Else If include_depth = 3 Then
  Begin assigntokstr(include_file2, start, len); reset(include_file2) End
  Else If include_depth = 4 Then
  Begin assigntokstr(include_file3, start, len); reset(include_file3) End
  Else If include_depth = 5 Then
  Begin assigntokstr(include_file4, start, len); reset(include_file4) End
  Else If include_depth = 6 Then
  Begin assigntokstr(include_file5, start, len); reset(include_file5) End
  Else If include_depth = 7 Then
  Begin assigntokstr(include_file6, start, len); reset(include_file6) End
  Else If include_depth = 8 Then
  Begin assigntokstr(include_file7, start, len); reset(include_file7) End
End;

{ Push a New include file onto the stack }
Procedure PushIncludeFile(path_start, path_len: Integer);
Var
  i: Integer;
Begin
  If include_depth >= 8 Then
    Error(17)  { include nesting too deep - Error halts }
  Else
  Begin
    { Save current state }
    If include_depth > 0 Then
    Begin
      include_line[include_depth - 1] := line_num;
      include_col[include_depth - 1] := col_num;
      include_ch[include_depth - 1] := ch;
      include_pushback[include_depth - 1] := pushback_ch
    End;

    { Open include file }
    include_depth := include_depth + 1;
    IncludeOpenFromTokStr(path_start, path_len);

    { Store path For reference (include_depth is already incremented) }
    For i := 0 To path_len - 1 Do
      include_path[(include_depth - 1) * 256 + i] := tok_str[path_start + i];
    include_path[(include_depth - 1) * 256 + path_len] := 0;

    line_num := 1;
    col_num := 0;
    pushback_ch := -1;
    { Read first character from New file }
    ch := ReadCurrentChar
  End
End;

{ Parse And process include directive: dollar-I Or dollar-INCLUDE }
Procedure ParseIncludeDirective;
Var
  i: Integer;
Begin
  { Skip whitespace after I Or INCLUDE }
  While (ch = 32) Or (ch = 9) Do
    NextChar;

  { Read filename Until closing brace Or whitespace }
  i := 0;
  While (ch <> 125) And (ch <> 32) And (ch <> 9) And (ch <> -1) And (i < 255) Do
  Begin
    tok_str[i] := ch;
    i := i + 1;
    NextChar
  End;
  tok_str[i] := 0;

  { Skip To End Of directive }
  While (ch <> 125) And (ch <> -1) Do
    NextChar;
  If ch = 125 Then
    NextChar;

  { Process include }
  If i > 0 Then
    PushIncludeFile(0, i)
End;

Procedure SkipWhitespace;
Var
  directive_char: Integer;
Begin
  While (ch = 32) Or (ch = 9) Or (ch = 10) Or (ch = 13) Do
    NextChar;
  { Skip comments And process directives }
  If ch = 123 Then  { '{' }
  Begin
    NextChar;
    { Check For compiler directive }
    If ch = 36 Then  { '$' }
    Begin
      NextChar;
      directive_char := ToLower(ch);
      If directive_char = 105 Then  { 'i' }
      Begin
        NextChar;
        { Check For 'include' Or just whitespace after 'i' }
        If (ch = 32) Or (ch = 9) Or (ch = 125) Then
        Begin
          { Short form: dollar-I filename }
          ParseIncludeDirective;
          SkipWhitespace
        End
        Else If ToLower(ch) = 110 Then  { 'n' - could be 'include' }
        Begin
          { Check For 'nclude' }
          NextChar;
          If ToLower(ch) = 99 Then  { 'c' }
          Begin
            NextChar;
            If ToLower(ch) = 108 Then  { 'l' }
            Begin
              NextChar;
              If ToLower(ch) = 117 Then  { 'u' }
              Begin
                NextChar;
                If ToLower(ch) = 100 Then  { 'd' }
                Begin
                  NextChar;
                  If ToLower(ch) = 101 Then  { 'e' }
                  Begin
                    NextChar;
                    { Full form: dollar-INCLUDE filename }
                    ParseIncludeDirective;
                    SkipWhitespace
                  End
                  Else
                  Begin
                    { Unknown directive, skip To End }
                    While (ch <> 125) And (ch <> -1) Do NextChar;
                    If ch = 125 Then NextChar;
                    SkipWhitespace
                  End
                End
                Else
                Begin
                  While (ch <> 125) And (ch <> -1) Do NextChar;
                  If ch = 125 Then NextChar;
                  SkipWhitespace
                End
              End
              Else
              Begin
                While (ch <> 125) And (ch <> -1) Do NextChar;
                If ch = 125 Then NextChar;
                SkipWhitespace
              End
            End
            Else
            Begin
              While (ch <> 125) And (ch <> -1) Do NextChar;
              If ch = 125 Then NextChar;
              SkipWhitespace
            End
          End
          Else
          Begin
            While (ch <> 125) And (ch <> -1) Do NextChar;
            If ch = 125 Then NextChar;
            SkipWhitespace
          End
        End
        Else
        Begin
          { Unknown directive starting With 'i', skip }
          While (ch <> 125) And (ch <> -1) Do NextChar;
          If ch = 125 Then NextChar;
          SkipWhitespace
        End
      End
      Else
      Begin
        { Unknown directive, skip as comment }
        While (ch <> 125) And (ch <> -1) Do NextChar;
        If ch = 125 Then NextChar;
        SkipWhitespace
      End
    End
    Else
    Begin
      { Regular comment }
      While (ch <> 125) And (ch <> -1) Do
        NextChar;
      If ch = 125 Then
        NextChar;
      SkipWhitespace
    End
  End
End;

Procedure NextToken;
Var
  i: Integer;
Begin
  SkipWhitespace;

  If ch = -1 Then
  Begin
    tok_type := TOK_EOF;
    tok_len := 0
  End
  Else If IsDigit(ch) = 1 Then
  Begin
    tok_type := TOK_INTEGER;
    tok_int := 0;
    While IsDigit(ch) = 1 Do
    Begin
      tok_int := tok_int * 10 + (ch - 48);
      NextChar
    End;
    { Check For decimal point (but Not ..) }
    If ch = 46 Then  { '.' }
    Begin
      NextChar;
      If IsDigit(ch) = 1 Then
      Begin
        { This is a float literal }
        tok_type := TOK_FLOAT_LITERAL;
        tok_float_int := tok_int;
        tok_float_frac := 0;
        tok_float_neg := 0;
        { Parse fractional digits (up To 6 digits Of precision) }
        i := 0;
        While (IsDigit(ch) = 1) And (i < 6) Do
        Begin
          tok_float_frac := tok_float_frac * 10 + (ch - 48);
          i := i + 1;
          NextChar
        End;
        { Scale frac To 6 digits }
        While i < 6 Do
        Begin
          tok_float_frac := tok_float_frac * 10;
          i := i + 1
        End;
        { Skip any remaining fractional digits }
        While IsDigit(ch) = 1 Do
          NextChar
      End
      Else
      Begin
        { It was just an Integer followed by '.', push back current Char }
        { And Set pushback To '.' so next NextToken gets DOT Or DOTDOT }
        pushback_ch := ch;
        ch := 46  { Put '.' back as current so the DOT handling code gets it }
      End
    End
  End
  Else If IsAlpha(ch) = 1 Then
  Begin
    tok_type := TOK_IDENT;
    tok_len := 0;
    While (IsAlpha(ch) = 1) Or (IsDigit(ch) = 1) Do
    Begin
      If tok_len < 255 Then
      Begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      End;
      NextChar
    End;
    tok_str[tok_len] := 0;

    { Check For keywords }
    { This is simplified - would need proper keyword table }
    If tok_len = 7 Then
      If (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 114) Then { pr }
        If (ToLower(tok_str[2]) = 111) And (ToLower(tok_str[3]) = 103) Then { og }
          If (ToLower(tok_str[4]) = 114) And (ToLower(tok_str[5]) = 97) Then { ra }
            If ToLower(tok_str[6]) = 109 Then { m }
              tok_type := TOK_PROGRAM;
    If tok_len = 5 Then
      If (ToLower(tok_str[0]) = 98) And (ToLower(tok_str[1]) = 101) Then { be }
        If (ToLower(tok_str[2]) = 103) And (ToLower(tok_str[3]) = 105) Then { gi }
          If ToLower(tok_str[4]) = 110 Then { n }
            tok_type := TOK_BEGIN;
    If tok_len = 3 Then
    Begin
      If (ToLower(tok_str[0]) = 101) And (ToLower(tok_str[1]) = 110) Then { en }
        If ToLower(tok_str[2]) = 100 Then { d }
          tok_type := TOK_END;
      If (ToLower(tok_str[0]) = 118) And (ToLower(tok_str[1]) = 97) Then { va }
        If ToLower(tok_str[2]) = 114 Then { r }
          tok_type := TOK_VAR;
      If (ToLower(tok_str[0]) = 100) And (ToLower(tok_str[1]) = 105) Then { di }
        If ToLower(tok_str[2]) = 118 Then { v }
          tok_type := TOK_DIV;
      If (ToLower(tok_str[0]) = 109) And (ToLower(tok_str[1]) = 111) Then { mo }
        If ToLower(tok_str[2]) = 100 Then { d }
          tok_type := TOK_MOD;
      If (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 110) Then { an }
        If ToLower(tok_str[2]) = 100 Then { d }
          tok_type := TOK_AND;
      If (ToLower(tok_str[0]) = 110) And (ToLower(tok_str[1]) = 111) Then { no }
        If ToLower(tok_str[2]) = 116 Then { t }
          tok_type := TOK_NOT;
      If (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 111) Then { fo }
        If ToLower(tok_str[2]) = 114 Then { r }
          tok_type := TOK_FOR;
      If (ToLower(tok_str[0]) = 110) And (ToLower(tok_str[1]) = 105) Then { ni }
        If ToLower(tok_str[2]) = 108 Then { l }
          tok_type := TOK_NIL;
      If (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 101) Then { se }
        If ToLower(tok_str[2]) = 116 Then { t }
          tok_type := TOK_SET
    End;
    If tok_len = 2 Then
    Begin
      If (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 102) Then { If }
        tok_type := TOK_IF;
      If (ToLower(tok_str[0]) = 100) And (ToLower(tok_str[1]) = 111) Then { Do }
        tok_type := TOK_DO;
      If (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 111) Then { To }
        tok_type := TOK_TO;
      If (ToLower(tok_str[0]) = 111) And (ToLower(tok_str[1]) = 114) Then { Or }
        tok_type := TOK_OR;
      If (ToLower(tok_str[0]) = 111) And (ToLower(tok_str[1]) = 102) Then { Of }
        tok_type := TOK_OF;
      If (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 110) Then { In }
        tok_type := TOK_IN
    End;
    If tok_len = 4 Then
    Begin
      If (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 104) Then { th }
        If (ToLower(tok_str[2]) = 101) And (ToLower(tok_str[3]) = 110) Then { en }
          tok_type := TOK_THEN;
      If (ToLower(tok_str[0]) = 101) And (ToLower(tok_str[1]) = 108) Then { el }
        If (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 101) Then { se }
          tok_type := TOK_ELSE;
      If (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 114) Then { tr }
        If (ToLower(tok_str[2]) = 117) And (ToLower(tok_str[3]) = 101) Then { ue }
          tok_type := TOK_TRUE
    End;
    If tok_len = 5 Then
    Begin
      If (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 111) Then { co }
        If (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 115) Then { ns }
          If ToLower(tok_str[4]) = 116 Then { t }
            tok_type := TOK_CONST;
      If (ToLower(tok_str[0]) = 119) And (ToLower(tok_str[1]) = 104) Then { wh }
        If (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 108) Then { il }
          If ToLower(tok_str[4]) = 101 Then { e }
            tok_type := TOK_WHILE;
      If (ToLower(tok_str[0]) = 117) And (ToLower(tok_str[1]) = 110) Then { un }
        If (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 105) Then { ti }
          If ToLower(tok_str[4]) = 108 Then { l }
            tok_type := TOK_UNTIL;
      If (ToLower(tok_str[0]) = 97) And (ToLower(tok_str[1]) = 114) Then { ar }
        If (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 97) Then { ra }
          If ToLower(tok_str[4]) = 121 Then { y }
            tok_type := TOK_ARRAY;
      If (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 97) Then { fa }
        If (ToLower(tok_str[2]) = 108) And (ToLower(tok_str[3]) = 115) Then { ls }
          If ToLower(tok_str[4]) = 101 Then { e }
            tok_type := TOK_FALSE
    End;
    If tok_len = 6 Then
    Begin
      If (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) Then { re }
        If (ToLower(tok_str[2]) = 112) And (ToLower(tok_str[3]) = 101) Then { pe }
          If (ToLower(tok_str[4]) = 97) And (ToLower(tok_str[5]) = 116) Then { at }
            tok_type := TOK_REPEAT;
      If (ToLower(tok_str[0]) = 100) And (ToLower(tok_str[1]) = 111) Then { Do }
        If (ToLower(tok_str[2]) = 119) And (ToLower(tok_str[3]) = 110) Then { wn }
          If (ToLower(tok_str[4]) = 116) And (ToLower(tok_str[5]) = 111) Then { To }
            tok_type := TOK_DOWNTO;
      If (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) Then { re }
        If (ToLower(tok_str[2]) = 97) And (ToLower(tok_str[3]) = 100) Then { ad }
          If (ToLower(tok_str[4]) = 108) And (ToLower(tok_str[5]) = 110) Then { ln }
            tok_type := TOK_READLN;
      If (ToLower(tok_str[0]) = 115) And (ToLower(tok_str[1]) = 116) Then { st }
        If (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 105) Then { ri }
          If (ToLower(tok_str[4]) = 110) And (ToLower(tok_str[5]) = 103) Then { ng }
            tok_type := TOK_STRING_TYPE;
      If (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) Then { re }
        If (ToLower(tok_str[2]) = 99) And (ToLower(tok_str[3]) = 111) Then { co }
          If (ToLower(tok_str[4]) = 114) And (ToLower(tok_str[5]) = 100) Then { rd }
            tok_type := TOK_RECORD
    End;
    If tok_len = 7 Then
    Begin
      If (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 110) Then { In }
        If (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 101) Then { te }
          If (ToLower(tok_str[4]) = 103) And (ToLower(tok_str[5]) = 101) Then { ge }
            If ToLower(tok_str[6]) = 114 Then { r }
              tok_type := TOK_INTEGER_TYPE;
      If (ToLower(tok_str[0]) = 98) And (ToLower(tok_str[1]) = 111) Then { bo }
        If (ToLower(tok_str[2]) = 111) And (ToLower(tok_str[3]) = 108) Then { ol }
          If (ToLower(tok_str[4]) = 101) And (ToLower(tok_str[5]) = 97) Then { ea }
            If ToLower(tok_str[6]) = 110 Then { n }
              tok_type := TOK_BOOLEAN_TYPE;
      If (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 111) Then { fo }
        If (ToLower(tok_str[2]) = 114) And (ToLower(tok_str[3]) = 119) Then { rw }
          If (ToLower(tok_str[4]) = 97) And (ToLower(tok_str[5]) = 114) Then { ar }
            If ToLower(tok_str[6]) = 100 Then { d }
              tok_type := TOK_FORWARD
    End;
    If tok_len = 4 Then
    Begin
      If (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 104) Then { ch }
        If (ToLower(tok_str[2]) = 97) And (ToLower(tok_str[3]) = 114) Then { ar }
          tok_type := TOK_CHAR_TYPE;
      If (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) Then { re }
        If (ToLower(tok_str[2]) = 97) And (ToLower(tok_str[3]) = 100) Then { ad }
          tok_type := TOK_READ;
      If (ToLower(tok_str[0]) = 114) And (ToLower(tok_str[1]) = 101) Then { re }
        If (ToLower(tok_str[2]) = 97) And (ToLower(tok_str[3]) = 108) Then { al }
          tok_type := TOK_REAL_TYPE;
      If (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 121) Then { ty }
        If (ToLower(tok_str[2]) = 112) And (ToLower(tok_str[3]) = 101) Then { pe }
          tok_type := TOK_TYPE_KW;
      If (ToLower(tok_str[0]) = 99) And (ToLower(tok_str[1]) = 97) Then { ca }
        If (ToLower(tok_str[2]) = 115) And (ToLower(tok_str[3]) = 101) Then { se }
          tok_type := TOK_CASE;
      If (ToLower(tok_str[0]) = 119) And (ToLower(tok_str[1]) = 105) Then { wi }
        If (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 104) Then { th }
          tok_type := TOK_WITH;
      If (ToLower(tok_str[0]) = 116) And (ToLower(tok_str[1]) = 101) Then { te }
        If (ToLower(tok_str[2]) = 120) And (ToLower(tok_str[3]) = 116) Then { xt }
          tok_type := TOK_TEXT;
      If (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 105) Then { fi }
        If (ToLower(tok_str[2]) = 108) And (ToLower(tok_str[3]) = 101) Then { le }
          tok_type := TOK_FILE;
      If (ToLower(tok_str[0]) = 117) And (ToLower(tok_str[1]) = 110) Then { un }
        If (ToLower(tok_str[2]) = 105) And (ToLower(tok_str[3]) = 116) Then { it }
          tok_type := TOK_UNIT;
      If (ToLower(tok_str[0]) = 117) And (ToLower(tok_str[1]) = 115) Then { us }
        If (ToLower(tok_str[2]) = 101) And (ToLower(tok_str[3]) = 115) Then { es }
          tok_type := TOK_USES
    End;
    If tok_len = 9 Then
    Begin
      If (ToLower(tok_str[0]) = 112) And (ToLower(tok_str[1]) = 114) Then { pr }
        If (ToLower(tok_str[2]) = 111) And (ToLower(tok_str[3]) = 99) Then { oc }
          If (ToLower(tok_str[4]) = 101) And (ToLower(tok_str[5]) = 100) Then { ed }
            If (ToLower(tok_str[6]) = 117) And (ToLower(tok_str[7]) = 114) Then { ur }
              If ToLower(tok_str[8]) = 101 Then { e }
                tok_type := TOK_PROCEDURE;
      { Interface = 105,110,116,101,114,102,97,99,101 }
      If (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 110) Then { In }
        If (ToLower(tok_str[2]) = 116) And (ToLower(tok_str[3]) = 101) Then { te }
          If (ToLower(tok_str[4]) = 114) And (ToLower(tok_str[5]) = 102) Then { rf }
            If (ToLower(tok_str[6]) = 97) And (ToLower(tok_str[7]) = 99) Then { ac }
              If ToLower(tok_str[8]) = 101 Then { e }
                tok_type := TOK_INTERFACE
    End;
    If tok_len = 8 Then
      If (ToLower(tok_str[0]) = 102) And (ToLower(tok_str[1]) = 117) Then { fu }
        If (ToLower(tok_str[2]) = 110) And (ToLower(tok_str[3]) = 99) Then { nc }
          If (ToLower(tok_str[4]) = 116) And (ToLower(tok_str[5]) = 105) Then { ti }
            If (ToLower(tok_str[6]) = 111) And (ToLower(tok_str[7]) = 110) Then { on }
              tok_type := TOK_FUNCTION;
    { Implementation = 105,109,112,108,101,109,101,110,116,97,116,105,111,110 }
    If tok_len = 14 Then
      If (ToLower(tok_str[0]) = 105) And (ToLower(tok_str[1]) = 109) Then { im }
        If (ToLower(tok_str[2]) = 112) And (ToLower(tok_str[3]) = 108) Then { pl }
          If (ToLower(tok_str[4]) = 101) And (ToLower(tok_str[5]) = 109) Then { em }
            If (ToLower(tok_str[6]) = 101) And (ToLower(tok_str[7]) = 110) Then { en }
              If (ToLower(tok_str[8]) = 116) And (ToLower(tok_str[9]) = 97) Then { ta }
                If (ToLower(tok_str[10]) = 116) And (ToLower(tok_str[11]) = 105) Then { ti }
                  If (ToLower(tok_str[12]) = 111) And (ToLower(tok_str[13]) = 110) Then { on }
                    tok_type := TOK_IMPLEMENTATION
  End
  Else If ch = 39 Then  { single quote - String }
  Begin
    tok_type := TOK_STRING;
    tok_len := 0;
    NextChar;
    While (ch <> 39) And (ch <> -1) Do
    Begin
      If tok_len < 255 Then
      Begin
        tok_str[tok_len] := ch;
        tok_len := tok_len + 1
      End;
      NextChar
    End;
    tok_str[tok_len] := 0;
    If ch = 39 Then
      NextChar
  End
  Else If ch = 43 Then  { + }
  Begin
    tok_type := TOK_PLUS;
    NextChar
  End
  Else If ch = 45 Then  { - }
  Begin
    tok_type := TOK_MINUS;
    NextChar
  End
  Else If ch = 42 Then  { * }
  Begin
    tok_type := TOK_STAR;
    NextChar
  End
  Else If ch = 47 Then  { / }
  Begin
    tok_type := TOK_SLASH;
    NextChar
  End
  Else If ch = 61 Then  { = }
  Begin
    tok_type := TOK_EQ;
    NextChar
  End
  Else If ch = 60 Then  { < }
  Begin
    NextChar;
    If ch = 62 Then  { <> }
    Begin
      tok_type := TOK_NEQ;
      NextChar
    End
    Else If ch = 61 Then  { <= }
    Begin
      tok_type := TOK_LE;
      NextChar
    End
    Else
      tok_type := TOK_LT
  End
  Else If ch = 62 Then  { > }
  Begin
    NextChar;
    If ch = 61 Then  { >= }
    Begin
      tok_type := TOK_GE;
      NextChar
    End
    Else
      tok_type := TOK_GT
  End
  Else If ch = 40 Then  { ( }
  Begin
    tok_type := TOK_LPAREN;
    NextChar
  End
  Else If ch = 41 Then  { ) }
  Begin
    tok_type := TOK_RPAREN;
    NextChar
  End
  Else If ch = 91 Then  { [ }
  Begin
    tok_type := TOK_LBRACKET;
    NextChar
  End
  Else If ch = 93 Then  { ] }
  Begin
    tok_type := TOK_RBRACKET;
    NextChar
  End
  Else If ch = 58 Then  { : }
  Begin
    NextChar;
    If ch = 61 Then  { := }
    Begin
      tok_type := TOK_ASSIGN;
      NextChar
    End
    Else
      tok_type := TOK_COLON
  End
  Else If ch = 59 Then  { ; }
  Begin
    tok_type := TOK_SEMICOLON;
    NextChar
  End
  Else If ch = 44 Then  { , }
  Begin
    tok_type := TOK_COMMA;
    NextChar
  End
  Else If ch = 46 Then  { . }
  Begin
    NextChar;
    If ch = 46 Then  { .. }
    Begin
      tok_type := TOK_DOTDOT;
      NextChar
    End
    Else
      tok_type := TOK_DOT
  End
  Else If ch = 94 Then  { ^ (caret) }
  Begin
    tok_type := TOK_CARET;
    NextChar
  End
  Else If ch = 64 Then  { @ }
  Begin
    tok_type := TOK_AT;
    NextChar
  End
  Else
  Begin
    Error(1);  { unexpected character }
    NextChar
  End
End;

