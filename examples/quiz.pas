{ Quiz Game - tests strings, readln, writeln }
program QuizGame;

var
  q1: string;
  q2: string;
  q3: string;
  q4: string;
  q5: string;
  answer: integer;
  score: integer;

begin
  score := 0;

  writeln('=== MATH QUIZ ===');
  writeln('Answer the following questions:');
  writeln;

  q1 := 'What is 7 + 8?';
  writeln(q1);
  write('Your answer: ');
  readln(answer);
  if answer = 15 then
  begin
    writeln('Correct!');
    score := score + 1
  end
  else
    writeln('Wrong! The answer was 15');
  writeln;

  q2 := 'What is 12 * 5?';
  writeln(q2);
  write('Your answer: ');
  readln(answer);
  if answer = 60 then
  begin
    writeln('Correct!');
    score := score + 1
  end
  else
    writeln('Wrong! The answer was 60');
  writeln;

  q3 := 'What is 100 div 4?';
  writeln(q3);
  write('Your answer: ');
  readln(answer);
  if answer = 25 then
  begin
    writeln('Correct!');
    score := score + 1
  end
  else
    writeln('Wrong! The answer was 25');
  writeln;

  q4 := 'What is 17 - 9?';
  writeln(q4);
  write('Your answer: ');
  readln(answer);
  if answer = 8 then
  begin
    writeln('Correct!');
    score := score + 1
  end
  else
    writeln('Wrong! The answer was 8');
  writeln;

  q5 := 'What is 3 * 3 * 3?';
  writeln(q5);
  write('Your answer: ');
  readln(answer);
  if answer = 27 then
  begin
    writeln('Correct!');
    score := score + 1
  end
  else
    writeln('Wrong! The answer was 27');
  writeln;

  writeln('=== RESULTS ===');
  write('You got ');
  write(score);
  writeln(' out of 5 correct!');

  if score = 5 then
    writeln('Perfect score! Excellent!')
  else if score >= 3 then
    writeln('Good job!')
  else
    writeln('Keep practicing!')
end.
