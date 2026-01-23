program TestFileIO;
var
  inputFd, outputFd: integer;
  savedInput, savedOutput: integer;
  testFile: text;
  ch: integer;
begin
  { Test getinputfd and getoutputfd }
  savedInput := getinputfd;
  savedOutput := getoutputfd;

  writeln('Current input fd: ', savedInput);
  writeln('Current output fd: ', savedOutput);

  { Test createfile - create a new file }
  outputFd := createfile('test_output.txt');
  if outputFd < 0 then
    writeln('Error: could not create file')
  else
  begin
    writeln('Created file with fd: ', outputFd);

    { Write to the file using writefd }
    writefd(outputFd, 72);  { H }
    writefd(outputFd, 101); { e }
    writefd(outputFd, 108); { l }
    writefd(outputFd, 108); { l }
    writefd(outputFd, 111); { o }
    writefd(outputFd, 10);  { newline }

    { Close the file }
    closefd(outputFd);
    writeln('Closed output file')
  end;

  { Test openfile - read the file we just created }
  inputFd := openfile('test_output.txt');
  if inputFd < 0 then
    writeln('Error: could not open file')
  else
  begin
    writeln('Opened file with fd: ', inputFd);

    { Read and print contents using readfd }
    write('File contents: ');
    ch := readfd(inputFd);
    while ch >= 0 do
    begin
      writechar(ch);
      ch := readfd(inputFd)
    end;

    closefd(inputFd);
    writeln('Closed input file')
  end;

  { Test setinputfd and setoutputfd with file variables }
  assign(testFile, 'test_output2.txt');
  rewrite(testFile);

  { Set output to the file }
  savedOutput := getoutputfd;
  setoutput(testFile);

  { These writes should go to the file }
  writeln('This goes to file');
  writeln('Second line');

  { Restore stdout }
  setoutputfd(savedOutput);

  close(testFile);
  writeln('Wrote to test_output2.txt via setoutput');

  { Read it back }
  assign(testFile, 'test_output2.txt');
  reset(testFile);

  savedInput := getinputfd;
  setinput(testFile);

  writeln('Reading from test_output2.txt:');
  ch := readchar;
  while ch >= 0 do
  begin
    writechar(ch);
    ch := readchar
  end;

  setinputfd(savedInput);
  close(testFile);

  writeln('Test complete!')
end.
