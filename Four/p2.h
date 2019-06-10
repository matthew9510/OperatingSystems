/* File: p2.h
 * Name: Matthew Hess
 * Instructor: John Carroll
 * Class: CS570
 * Due Date: 10/10/18
 */

/* Include Files */
#include <stdio.h>
#include "getword.h"

/* Define Statements*/
#define SPACE ' '
#define NEWLINE '\n'
#define SEMICOLON ';'
#define DOLLARSIGN '$'
#define NULLTERM '\0'
#define BACKSLASH '\\'
#define SQUIGLE '~'
#define AND '&'
#define PIPELINE '|'
#define LESSTHAN '<'
#define GREATERTHAN '>'
#define MAXITEM 100 /* max number of words per line */
#define STORAGE 255 /* One more than getword()'s maximum wordsize */
/* Error Messages */
#define ERROR1 (1) // Error 1 signifies a input line whose first word is 'EOF'
#define ERROR2 (2) // Error 2 signifies the inability to set the process group
#define ERROR3 (3) // Error 3 signifies that the child process couldn't execute the command
#define ERROR4 (4) // Error 4 signifies that we have printed "p2 terminated." in main and that we have ignored the SIGTERM signal
#define ERROR5 (5) // Error 5 signifies that the grand child process with pipeline couldn't execute it's command
#define ERROR6 (6) // Error 6 signifies that there was no arguments after a pipeline after forking //todo erase
#define ERROR7 (7) // Error 7 signifies output redirection file can't be opened after forking
#define ERROR8 (8) // Error 8 signifies that the input redirection file couldn't be accessed maybe due to permissions or due to Input redirection path doesn't exist
#define ERROR9 (9) // Error 9 signifies that the input redirection file couldn't be opened
#define ERROR10 (10) // Error 10 signifies that the /dev/null couldn't be opened for redirection
#define Error11 (11) // Error 11 signifies that a metacharacter was parsed right after another metacharacter which causes a semantics error
#define MAXPIPES (10) /* max number of pipelines */


/*
 * This program acts like a simple command line interpreter for a UNIX System.
 *
 */





void signalCatcher(int);
/*
 * The signalCatcher() function is a substitute function for handling the SIGTERM signal, It allows us to print that
 * our p2 terminates before the process is killed.
 */
int parse();
/*
 * All syntactic analysis should be done within parse().  This means, among
* other things, that parse() should set appropriate flags when getword()
* encounters words that are metacharacters. Parse() also sets up additional data structures
 * used throughout p2, and sets error flags if a particular case is seen that needs to be handled appropriately
*/
void pipeline();
/*
 * This function handles the case where a pipeline is seen after parse calls getword().
 * Calling a separate function helps with clean code.
 */
void setupArgVCommand2();
/*
 * This function creates a newArgv2 so that we can use this new array for executing the grandchild process.
 */
int main();
/*
 * Repeatedly prints the user a prompt and executes commands after redirecting files.
 */
