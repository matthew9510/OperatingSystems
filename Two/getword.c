/* File: getword.c
 * Name: Matthew Hess
 * Instructor: John Carroll
 * Class: CS570
 * Due Date: 10/5/18
 */

/* Include Files */
#include <stdio.h> // EOF is a variable in this module
#include <stdlib.h> // genenv() system call utilized from this library
#include "getword.h"
#include "p2.h" //todo check if it is correct to keep my Global #Defines in p2.h; yes because you're using them in p2.c as well?
//#include "p2.c" // You shouldn't include another .c file within an program that is linked together

/* Global Variable Declarations */
//extern int AND_FLAG;
extern int BACKSLASH_FLAG;

int getword(char *w )
{
/**
* The function getword() is a lexical analyzer with specific parsing rules.
* The getword() function gets one word from the input stream.
* It returns -255 if end-of-file is encountered;
* otherwise, it [usually] returns the number of characters in the word [but
* note the exceptions listed in the details below]
*
* INPUT: a pointer to the beginning of a character string [a character array]
* OUTPUT: the number of characters in the word (or the negative of that number)
* SIDE EFFECTS: bytes beginning at address w will be overwritten.
*   Anyone using this routine should have w pointing to an
*   available area at least STORAGE bytes long before calling getword().

Upon return, the string pointed to by w contains the next word in the line from
stdin. A "word" is a string containing one metacharacter OR a string consisting
of non-metacharacters delimited by blanks, newlines, metacharacters or EOF.
The metacharacters for Program1 are ";", "<", ">", "|", "&", "~", and "<<".

The last word on a line may be terminated by a space, the newline character
OR by end-of-file.  "$", "~", and "\" also follow special rules, as
described later (unlike the other metacharacters, these are NOT delimiters).

getword() skips leading blanks, so if getword() is called and there are no
more words on the line, then w points to an empty string. All strings,
including the empty string, will be delimited by a zero-byte (eight 0-bits),
as per the normal C convention (this delimiter is not 'counted' when determining
the length of the string that getword will report as a return value).

The backslash character "\" is special, and may change the behavior of
the character that directly follows it on the input line.  When "\" precedes
a metacharacter, that metacharacter is treated like most other characters.
(That is, the symbol will be part of a word rather than a word delimiter.)
**/
	int iochar; // a place holder variable holding one ascii character at a time assigned with the getchar() function to access the stdin
	int charCount = 0; // counter to keep track of characters being added to the input pointer w
	int dollarFlipped = 1; // if 1 then it means no leading '$'; if dollarFlipped is set to -1 then there is a leading '$' and we need to return negative of charCount

	while ( ( iochar = getchar() ) != EOF )
	{
		if ( charCount == STORAGE - 1 ) { // handling of a word being longer than 254 characters long
			ungetc(iochar,stdin); // invoke ungetc function to place character back in stdin so that we can pick the while loop iteration where we left off 
			*w++ = NULLTERM; // place a null terminator in the character array being passed into the function as the parameter
			if (dollarFlipped == -1) {  // if we have seen a dollar as the first character jump in this condition
				return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs
			}
			else { // if dollarFlipped == 1 (No preceding '$' before "word")
				return charCount;
			}
		}

		if ( iochar == BACKSLASH ) {
		    BACKSLASH_FLAG = 1;  // so that characters are handled appropriately in p2.c
			iochar = getchar(); // get next character in input stream so that we can handle it appropriately 
			switch( iochar ) { // set up a switch statment to handle a few particular cases
				case NEWLINE :
					if ( charCount == 0 ){ // if we haven't seen a character before continue the while loop to get the next character from the input stream
						continue;
					}
					if ( charCount > 0 ){ // if we have seen at lease one character before then place a null terminator in the character array passed in and return the number of characters in the character array
						*w = NULLTERM;
						if ( dollarFlipped == -1 ) // if we have seen a dollar as the first character jump in this condition
						{
							return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs
						}
						else // if dollarFlipped == 1 (No preceding '$' before "word")
						{
							return charCount;
						}
					}

				case EOF:
					if ( ( iochar == EOF ) && (charCount == 0 ) ){ // handling of if the character in the input stream is EOF in addition to not adding anything to the character array passed in 
						*w = NULLTERM;
						return -255;
					}
					if ( ( iochar == EOF ) && (charCount > 0 ) ){ // handling of if the character in the input stream is EOF in addition to already having added a character to the character array passed in 
						*w = NULLTERM;
						ungetc(EOF,stdin);
						if (dollarFlipped == -1)  // if we have seen a dollar as the first character jump in this condition and return the negative charcount number
						{
							return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs
						}
						else  // if dollarFlipped == 1 (No preceding '$' before "word")
						{
							return charCount;
						}
					}
				default : // handling of all non-meta characters and some meta-characters such as ('a-z', ';' ,'SPACE' , <, >, |, &, ~, $, \)
					*w++ = iochar;
					charCount++;
					continue;
			}
		}

		if ( charCount == 0 ) // handling of if we haven't already seen any character and if the character from the input stream is a special character where action is needed
		{
			if ( iochar == SPACE ) // skip leading spaces
			{
				continue;   //just continue the while loop. Don't increment counter or pointer
			}
			else if ( ( iochar == SEMICOLON ) || ( iochar == NEWLINE ) ) //delimiters that place a null terminator in the character array passed in and return 0
			{
				*w = NULLTERM;
				return 0;
			}
			else if ( iochar == DOLLARSIGN ) // hadling of seeing a leading '$' as the first character
			{
				dollarFlipped = -1; // set the dollar Flag
				charCount++; // this is added so that if we get multiple preceding '$' we handle it properly and don't produce incorrect output
				continue; // just continue the while loop. Don't increment counter or pointer
			}
			if ( ( iochar == LESSTHAN ) || ( iochar == GREATERTHAN ) || ( iochar == PIPELINE ) || ( iochar == AND ) ){
				// handling of the character from input stream if the char is '<' or '>' or '|' or '&' by delimiting the char array after placing the char in the array and null terminate and returning 1
				charCount++;
				*w++ = iochar;
				iochar = getchar(); // we need to see what the next character is in the input stream to handle it appropriately
				if (iochar == LESSTHAN){ // if the next character is '<' store another '<' in the character array and retrurn 2 instead of 1
					charCount++;
					*w++ = iochar;
					*w++ = NULLTERM; // Terminate the character array before returning
				}
				else{
					*w++ = NULLTERM; // Terminate the character array before returning
					ungetc(iochar,stdin); // if the next character is anything other than '<' place it back in the input stream
				}
				return charCount;
			}
			if (iochar == SQUIGLE){ // leading '~' character will store the root directory in the character array passed and update the ammount of characters appropriately 
				char * envVar; // declare a variable that will point to the system call getenv() function 
				for ( envVar = getenv("HOME"); *envVar != '\0'; envVar++ ){
					// start the for loop with the first character from the returned character array from the getenv() system call, and terminate the for loop when we reach the null terminator in the character array
					charCount++; // add one to the charcount counter for each letter we add to the character array passed in to the function
					*w++ = *envVar; // assign the letter to the character array passed in
				}
				continue; // jump to the next character in the input stream
			}
		}


		if ( charCount > 0 ) // handling of having at least added one character to the character array passed in
		{
			if ( iochar == SPACE ) // treat this condition as delimiting the word and returning that charcount size of the word in the character array passed in
			{
				*w = NULLTERM; // place a null terminator in the character array
				if ( dollarFlipped == -1 ) // if we have seen a dollar as the first character jump in this condition and return the negative charcount number
				{
					return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs
				}
				else // if dollarFlipped == 1 (No preceding '$' before "word")
				{
					return charCount;
				}
			}

			if ( ( iochar == SEMICOLON ) || ( iochar == NEWLINE ) ) //return the charCount (length) of the word in the character array and then when getword gets called again print out the semicolon delimiter in the case where charcount == 0 and iochar is 'SEMICOLON' or 'NEWLINE'
			{
				ungetc(iochar, stdin); // place the character back in the stdin stream to delimit the next word with returning 0 and not assigning the character array passed in with anything
				*w = NULLTERM;  // place a null terminator in the character array 
				if ( dollarFlipped == -1 )  // if we have seen a dollar as the first character jump in this condition and return the negative charcount number
				{
					return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs
				}
				else // if dollarFlipped == 1 (No preceding '$' before "word")
				{
					return charCount;
				}
			}
			if ( ( iochar == LESSTHAN ) || ( iochar == GREATERTHAN ) || ( iochar == PIPELINE ) || ( iochar == AND ) ){
				// handling of the character from input stream if the char is '<' or '>' or '|' or '&' by first returning the charCount (length) of the word in the character array and then when getword gets called again print out the delimiting the char array after placing that specific char in the array and null terminate and returning 1
				*w++ = NULLTERM; // place a null terminator in the character array
				ungetc(iochar, stdin); // we need to see what the next character is in the input stream to handle it appropriately
				if ( dollarFlipped == -1 ){  // if we have seen a dollar as the first character jump in this condition and return the negative charcount number
					return (charCount - 1) * dollarFlipped;
				}
				else {
					return charCount;
				}
			}
		}
		// Note if iochar == '\' then the following code in the while loop's scope will not execute
		// leading spaces will not trigger the following code in the while loop's scope
		// handling of all the other cases where the character from the input stream wasn't a special character, assign it to the character array passed in increment the charcount counter
		*w++ = iochar;
		charCount++;
	}
	if ( ( iochar == EOF ) && (charCount == 0 ) ){ // handling of if the character in the input stream is EOF in addition to not adding anything to the character array passed in 
		*w = NULLTERM; // place a null terminator in the character array
		return -255;
	}
	if ( ( iochar == EOF ) && (charCount > 0 ) ){// handling of if the character in the input stream is EOF in addition to already having added a character to the character array passed in 
		*w = NULLTERM; // place a null terminator in the character array
		// ungetc(EOF,stdin); 
		if (dollarFlipped == -1) // if we have seen a dollar as the first character jump in this condition and return the negative charcount number
		{
			return (charCount - 1) * dollarFlipped ; //note: minus one due to the handling of multiple DOLLARSIGNs
		}
		else  // if dollarFlipped == 1 (No preceding '$' before "word")
		{
			return charCount;
		}
	}
}