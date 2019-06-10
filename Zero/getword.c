/* File: getword.c
 * Name: Matthew Hess
 * Instructor: John Carroll
 * Class: CS570
 * Due Date: 9/5/18
 */

/* Include Files */
#include <stdio.h>
#include "getword.h"
#define SPACE ' '
#define NEWLINE '\n'
#define SEMICOLON ';'
#define DOLLARSIGN '$'
#define NULLTERM '\0'
 
int getword(char *w )
{
/**
	The function getword() is a lexical analyzer.
	It will process "words" from the input stream by adding certain characters to the character array passed in to the routine,
		and by counting the ammount of characters in the input stream depending the character.

	Input: Character pointer to a character array 
	Output: -255, 0, or (plus or minus) the number of characters in the word
	
	Output based on characters in the input stream:
		- if the input stream has collected some characters (charCount > 0), it will return the size of the word once the input stream has encountered a space, newline, or EOF
		- if EOF is encountered while the character count is still zero, the routine will return (-255)
		- if a ';' (SEMICOLON) or a '\n' (NEWLINE) is encountered while the character count is still zero, the routine will return 0
		- if the word from the input stream starts with a '$' (DOLLARSIGN) the routine will return the negative of the word length
**/
    int iochar;
    int charCount = 0;
    int dollarFlipped = 1; // if 1 then it means no leading '$'; if dollarFlipped is set to -1 then there is a leading '$' and we need to return negative of charCount

    while ( ( iochar = getchar() ) != EOF )
    {
		if ( charCount == 0 )
		{
		   if ( iochar == SPACE )
		   {
			    continue;   //just continue the while loop. Don't increment counter or pointer
		   }
		   else if ( ( iochar == SEMICOLON ) || ( iochar == NEWLINE ) )
		   {
			   *w = NULLTERM;
			   return 0;
		   }
		   else if ( iochar == DOLLARSIGN )
		   {
		       dollarFlipped = -1;
			   charCount++; // this is added so that if we get multiple preceding '$' we handle it properly
			   continue; // just continue the while loop. Don't increment counter or pointer
		   }
		}

		if ( charCount > 0 ) 
		{
			if ( iochar == SPACE )
			{
			    *w = NULLTERM;
				if ( dollarFlipped == -1 )
				{
			        return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs 
				}
				else // if dollarFlipped == 1 (No preceding '$' before "word")
				{
				    return charCount; 
				}
			}

			if ( ( iochar == SEMICOLON ) || ( iochar == NEWLINE ) )
			{
				ungetc(iochar, stdin);
				*w = NULLTERM;
				if ( dollarFlipped == -1 )
				{
			        return (charCount - 1) * dollarFlipped; //note: minus one due to the handling of multiple DOLLARSIGNs 
				}
				else // if dollarFlipped == 1 (No preceding '$' before "word")
				{
				    return charCount; 
				}
			}
		}
		// leading spaces will not trigger the following code in the while loop's scope
		*w = iochar;
		w++; 
		charCount++; 
	}
	if ( ( iochar == EOF ) && (charCount == 0 ) ){
	    *w = NULLTERM;
	    return -255;
	}
	if ( ( iochar == EOF ) && (charCount > 0 ) ){
	    *w = NULLTERM;
	    if (dollarFlipped == -1)
		{
	        return (charCount - 1) * dollarFlipped ; //note: minus one due to the handling of multiple DOLLARSIGNs 
		}
		else  // if dollarFlipped == 1 (No preceding '$' before "word")
		{
		    return charCount; 
		}
	}	
}