TODO:
Update code with feedback from Professor: 
Overall it is pretty professional, so I don't have too much to say
other than a few minor quibbles.

p2.c, line 809: This else has a slew of declarations and initializations
in it.  That means that every time the process gets to line 810, all
the variables have to be allocated (and initialized), over and over.
That might be a good choice for a block of code that very rarely
gets called, but for something that may be called multiple times,
it is better to just declare all this stuff at the top of the function,
and then only initialize those variables that need to be (re)initialized
when you enter the 'else'.  Ditto for line 1374, etc.

p2.c, line 47: This is a great definition, since it automatically
changes the buffer size if either constant is changed.

p2.c, lines 57-78: There are a lot of mystery numbers here, which would
be better off being mnemonics in the p2.h file (or snagged from the
system include files, where appropriate).  Note that the number on
line 57 (20) should always be twice the number on line 60 (10),
so this should have been handled more like line 47 was.

Documentation is quite good (though tilde is misspelled in getword.c);
depending on the level of your 'audience', it might have been useful
to explain the '+1' on line 790, etc. -- though in a professional
environment, this would likely be overkill.  Oh, wait -- I see that
you do explain it on lines 976, 982, ... so doing it everywhere
isn't needed.

'todo' on line 1034: "\" would be a syntax error; the first " starts
a string, \" puts a literal double quote mark in the string, and then
there is no terminating '"' to end the string.
