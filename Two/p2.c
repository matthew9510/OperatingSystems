/* File: p2.c
 * Name: Matthew Hess
 * Instructor: John Carroll
 * Class: CS570
 * Due Date: 10/10/18
 */

/* Include Files */
#include <stdio.h> // EOF is a variable in this module , includes fflush
#include <stdlib.h> // genenv() system call utilized from this library
#include "p2.h"
#include "getword.h"
#include <math.h>
#include <unistd.h>  // for chdir() system call, fork() system call, access() system call
#include <sys/types.h> // for open()
#include <sys/stat.h> // for open()
#include <fcntl.h> // for open()
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "CHK.h" //todo use this function


/* Global Variable Declarations */
int BACKSLASH_FLAG = 0; // external declaration, used in getword.c
int INPUT_REDIRECTION_FLAG = 0;
int OUTPUT_REDIRECTION_FLAG = 0;
int PIPELINE_FLAG = 0;
int WAIT_FLAG = 0;
int SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 0;
int SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = 0;
int SYNTAX_ERROR_MULTIPLE_PIPELINES = 0;
int SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 0;
int SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 0; // TODO RENAME UPPER CASE
char bigBuffer[MAXITEM * STORAGE];
char * bigBufferPtr = bigBuffer;
char * newArgV[MAXITEM];
int processTwoStartIndex;
char * newArgVCommand2[MAXITEM];
int wordCount = 0;
char * inputRedirection;
char * outputRedirection;
int fileDescriptor[2];
pid_t kidpid, grandpid;

int main() {
    /* Declaration of Locals */
    int returnValueParse;
    int inputFileFileDescriptorNum;
    int outputFileFileDescriptorNum;

    /* Setting up the process group for this process */
    if( (setpgid(getpid(),0)) != 0){ // setpgid and if there is an error jump in else continue on to next statement
        fprintf(stderr, "Cant't change process group\n");
    }
    /* Handling the SIGTERM signal appropriately */
    (void) signal(SIGTERM, signalCatcher); // dont kill main, & kill all other processes

    for (;;) {
        /* Print Prompt to user */
        printf(":570: ");
        /* Call the Parse function to read the stdin and setup appropriate data structures and set flags*/
        returnValueParse = parse();

        /* Handling of errors discovered in the function Parse()*/
        if (SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED == 1) {
            fprintf(stderr, "No input redirection file specified\n");
            continue;
        }
        if (SYNTAX_ERROR_NO_Output_FILE_SPECIFIED == 1) {
            fprintf(stderr, "No output redirection file specified\n");
            continue;
        }
        if (SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS == 1) {
            fprintf(stderr, "You can't have multiple input redirections.\n");
            continue;
        }
        if (SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS == 1) {
            fprintf(stderr, "You can't have multiple output redirections.\n");
            continue;
        }
        if (SYNTAX_ERROR_MULTIPLE_PIPELINES == 1) {
            fprintf(stderr, "You can't have multiple pipelines.\n");
            continue;
        }
        /* Handling if we read in the EOF character as the first character in stdin*/
        if (returnValueParse == -255 && wordCount == 0) {
            break;
        }
        /* If line is empty reissue prompt */
        else if (wordCount == 0 && returnValueParse == 0) {
            continue;
        }
        /* If */ //todo
        if(newArgV[0] == '\0'){ // todo change to NULLTERM
            fprintf(stderr, "No executable stated\n");
        }

        /* Handling cd */
        if ((strcmp(newArgV[0], "cd")) == 0) {
            if (wordCount > 2) {
                fprintf(stderr, "Too many arguments for the command 'cd'\n");
                continue;
            }
            else if (wordCount == 1) {
                char * envVar;
                envVar = getenv("HOME");
                if ((chdir(envVar)) == -1) { // If we can't change directory
                    fprintf(stderr, "Couldn't change to home directory.\n");
                    continue;
                }
                else {  // If changing directory was successful
                    continue;
                }
            }
            else if (wordCount == 2){ // If a directory is explicitly stated
                if ((chdir(newArgV[1])) == -1) {// If we can't change to specified directory
                    fprintf(stderr, "Couldn't change to specified directory.\n");
                    continue;
                }
                else { // If changing directory was successful
                    continue;
                }
            }
        }

        // Before forking Flush the data streams
        fflush(stdout);
        fflush(stderr);

        // Fork to execute a child process
        // CHK((kidpid = fork())) todo check if we need to implement this for forking
        if ((kidpid = fork()) == -1){ // fork and if there is an error jump into if statement otherwise continue on to next statement
            fprintf(stderr,"Fork Failed");
            continue;
        }
        else if (kidpid == 0){ // If child process forked successfully

            /* If a pipeline has been read in through the parse() function using the getword() function we will
                branch off to another function to handle pipelines appropriately*/
            if(PIPELINE_FLAG == 1){
               setupArgVCommand2();
               if (newArgVCommand2[0] == NULL){
                   fprintf(stderr, "Pipeline with no second command specified results in an error.\n");
                   exit(ERROR6);
                   continue;
               }
               pipeline();
               continue; // todo zombies and handling & wait() and all the flags again
            }
            /* Below is Handling a single command */
            // Handling of output redirection
            if(OUTPUT_REDIRECTION_FLAG == 1 && outputRedirection != NULL){
                // don't access first
                outputFileFileDescriptorNum = open(outputRedirection, O_WRONLY | O_CREAT | O_EXCL , 0600); //todo O_TRUNC | S_IRUSR| S_IRGRP | S_IRGRP | S_IWUSR possibly
                if (outputFileFileDescriptorNum == -1) {
                    fprintf(stderr, "%s: File exists.\n", outputRedirection);
                    exit(ERROR7);
                }
                CHK(dup2(outputFileFileDescriptorNum, STDOUT_FILENO));
                close(outputFileFileDescriptorNum);
            }

            // Handling of input redirection
            if (INPUT_REDIRECTION_FLAG == 1 && inputRedirection != NULL) {
                // does the file exist and do I have the correct permissions, using the calling processe's real UID & GID
                int accessInputRedirectionfile = access(inputRedirection, R_OK);
                if (accessInputRedirectionfile < 0) {
                    if (errno == EACCES) {
                        fprintf(stderr, "Input redirection file permissions denied\n");
                        exit(ERROR8);
                    }
                    else if (errno == ENOENT && errno == ENOTDIR) {
                        fprintf(stderr, "Input redirection path doesn't exist\n");
                        exit(ERROR8);
                    }
                }
                else if (accessInputRedirectionfile == 0){
                    inputFileFileDescriptorNum = open(inputRedirection, O_RDONLY);
                    if (inputFileFileDescriptorNum == -1) {
                        fprintf(stderr, "Couldn't open the input redirection file\n");
                        exit(ERROR9);
                    }
                }
                CHK(dup2(inputFileFileDescriptorNum, STDIN_FILENO));
                close(inputFileFileDescriptorNum);
            }

            if (INPUT_REDIRECTION_FLAG == 0 && WAIT_FLAG == 1){
                int devNullFileDescriptorNum = open("/dev/null",O_RDONLY);
                if (devNullFileDescriptorNum == -1){
                    fprintf(stderr, "Couldn't open /dev/null");
                    exit(ERROR10);
                }
                CHK(dup2(devNullFileDescriptorNum, STDIN_FILENO));
                close(devNullFileDescriptorNum);
            }

            /* After all redirections are satisfied execute the process */
            if ((execvp(newArgV[0],newArgV)) == -1){
                fprintf(stderr, "Couldn't execute command\n");
                exit(ERROR3);
            }

        } // End of child process scope
        // Parent process scope
        if(WAIT_FLAG == 1){
            printf("%s [%d]\n", *newArgV, kidpid);
            continue;
        }
        else{
            for(;;){
                pid_t find_pid;
                find_pid = wait(NULL);
                if(find_pid == kidpid){
                    break;
                }
            }

        }
    }
    killpg(getpgrp(), SIGTERM);
    printf("p2 terminated.\n");
    exit(ERROR4);
}


void signalCatcher(int signalNum){
    // stays empty: rather than dying, do nothing
}

void pipeline() {
    /*
     * Vertical piping:
     * p2 creates one child, then that child creates a pipe, and then forks its own child.
     * p2's child and grandchild then handle the two halves of the pipe command.
     * The order of the fork() and pipe() system calls is important: both children have to know about the pipe since they use it to communicate
     */

    // Note we are still in child process scope // kidpid; command 2;

    /* Declaration of locals */
    int accessInputRedirectionfile;
    int inputFileFileDescriptorNum;
    int outputFileFileDescriptorNum;

    /* Create a pipe so we can redirect output from command 1 to the input of command 2 */
    CHK(pipe(fileDescriptor));

    /* Before forking, flush out stderr and stdout */
    fflush(stdout);
    fflush(stderr);

    /* Fork to setup redirections and eventually execute command 1 */
    CHK((grandpid = fork()));
    if (grandpid == 0) { // were in grandchild process scope
        /* Handling of input redirection */
        if (INPUT_REDIRECTION_FLAG == 1 && inputRedirection != NULL) {
            // does the file exist and do I have the correct permissions, using the calling processe's real UID & GID
            CHK((accessInputRedirectionfile = access(inputRedirection, R_OK)));
            if(accessInputRedirectionfile == -1){
                return;
            }
            if (accessInputRedirectionfile == 0) {
                inputFileFileDescriptorNum = open(inputRedirection, O_RDONLY);
                if (inputFileFileDescriptorNum == -1) {
                    fprintf(stderr, "Couldn't open the input redirection file\n");
                    exit(ERROR9);
                    return; // todo Not in forloop, return a number to signify to continue, check if return is the right thing to do
                }
            }
            CHK(dup2(inputFileFileDescriptorNum, STDIN_FILENO));
            close(inputFileFileDescriptorNum);
        }

        /* If WAITFLAG == 1 and INPUT_REDIRECTION_FLAG == 0 then redirect stdin to /dev/null */
        if (INPUT_REDIRECTION_FLAG == 0 && WAIT_FLAG == 1){
            int devNullFileDescriptorNum = open("/dev/null",O_RDONLY);
            if (devNullFileDescriptorNum == -1){
                fprintf(stderr, "Couldn't open /dev/null");
                exit(ERROR10);
                return;//continue;  // todo Not in forloop, return a number to signify to continue
            }
            CHK(dup2(devNullFileDescriptorNum, STDIN_FILENO));
            close(devNullFileDescriptorNum);
        }

        /* Setup redirection using our pipe created in the child process, we want stdout to go to write end of pipe */
        CHK(dup2(fileDescriptor[1], STDOUT_FILENO));
        /* Close the read end of the pipe so that the pipe doesn't wait for more information to be transmitted */
        close(fileDescriptor[0]);

        /* Execute Command 1 */
        if ((execvp(newArgV[0], newArgV)) == -1) {
            fprintf(stderr, "Couldn't execute command\n");
            exit(ERROR3); //todo create a new Error#
        }
    }// after we execute the command1, we will be back in the child process, now with data stored in our pipe

    /* Handling of output redirection */
    if (OUTPUT_REDIRECTION_FLAG == 1 && outputRedirection != NULL) {
        // don't access first
        outputFileFileDescriptorNum = open(outputRedirection, O_WRONLY | O_CREAT | O_EXCL, 0600); //todo O_TRUNC | S_IRUSR| S_IRGRP | S_IRGRP | S_IWUSR possibly
        if (outputFileFileDescriptorNum == -1) {
            fprintf(stderr, "%s: File exists.\n", outputRedirection);
            exit(ERROR7);
            //return; //continue;  // todo Not in forloop, return a number to signify to continue
        }
        CHK(dup2(outputFileFileDescriptorNum, STDOUT_FILENO));
        close(outputFileFileDescriptorNum);
    }

    /* setup pipe redirections */
    CHK(dup2(fileDescriptor[0], STDIN_FILENO));
    close(fileDescriptor[1]);

    /* Execute Command 2 */
    if ((execvp(newArgVCommand2[0], newArgVCommand2)) == -1) {
        fprintf(stderr, "Couldn't execute command\n");
        exit(ERROR5);
    } // End of child Process

    // Back in parent process if we haven't exited
    if (WAIT_FLAG == 1) {
        printf("%s [%d]\n", *newArgV, kidpid);
        return; //continue;
    }
    else {
        for (;;) {
            pid_t find_pid;
            find_pid = wait(NULL);
            if (find_pid == kidpid) {
                //break;
                return;
            }
        }
    }
}

void setupArgVCommand2(){
    int i = 0;
    while (newArgV[processTwoStartIndex + i] != NULL){
        newArgVCommand2[i] = newArgV[processTwoStartIndex + i];
        i = i + 1;
    }
}


int parse(){
    //todo DOCUMENT THIS FUNCTION
    /* Declaration of Locals */
    char * tempPtr;  // used for handling if we see the '&' character and the next character isn't a ';' 'NEWLINE' or '$ '
    char * bigBufferPtrCpy;  // used for handling if we see the '&' character and the next character isn't a ';' 'NEWLINE' or '$ '
    int getWordReturn;
    int i;

    /* Reset all global declarations to support invoking parse() multiple times */
    INPUT_REDIRECTION_FLAG = OUTPUT_REDIRECTION_FLAG = PIPELINE_FLAG = WAIT_FLAG = BACKSLASH_FLAG = wordCount = getWordReturn = 0;
    SYNTAX_ERROR_MULTIPLE_PIPELINES = SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = SYNTAX_ERROR_NO_Output_FILE_SPECIFIED= SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 0;
    inputRedirection = outputRedirection = tempPtr = bigBufferPtrCpy = NULL;
    bigBufferPtr = bigBuffer;
    for ( i = 0; i < MAXITEM * STORAGE ; i++){
        bigBuffer[i] = NULLTERM;
    }
    for(i = 0; i < MAXITEM; i++){
        newArgV[i] = NULL;
    }

    /* Parse stdin using the function getword() and set appropriate flags and load appropriate data structures */
    // Note flags set will be handled in p2.c main()
    while(((getWordReturn = getword(bigBufferPtr))) != -255) { // Note that if stdin is empty it returns EOF, and in getword() if (charCount == 0 and iochar == EOF) getword() returns -255 to end the parse function by returning -255
        /* Handling if the getword() function parsed either a SEMICOLON or a NEWLINE, or a DOLLARSIGN followed by a space or newline */
        if(getWordReturn == 0 && BACKSLASH_FLAG == 0){ // handles \;
            newArgV[wordCount] = NULL;
            return 0;
        }
        // Handling if the getword() function parsed a dollar sign at the beginning of a word, we need to strip the minus sign from the getwordReturn for prog2
        if (getWordReturn < 0 ){
            getWordReturn = abs(getWordReturn);
        }

        /* Handle Seeing flags */
        // Handling of input redirection
        if((strcmp(bigBufferPtr,"<")) == 0 && INPUT_REDIRECTION_FLAG == 0 && BACKSLASH_FLAG == 0){
            INPUT_REDIRECTION_FLAG = 1;
            // before calling getword() again we need to update the location of where bigBufferPtr is pointing to
            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
            // call getword again to handle the next "word in stdin" appropriately
            getWordReturn = getword(bigBufferPtr);
            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
                newArgV[wordCount] = NULL;
                return 0;
            }
            if(getWordReturn == -255){
                break;
            }
            // if(getWordReturn < 0 && getWordReturn != -255){} // previous conditional for handling -255 allows us to not need the second expression in the if statement
            if(getWordReturn < 0){
                getWordReturn = abs(getWordReturn);
            }
            if(getWordReturn > 0){
                inputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
        }
        // Handling of more than one input redirection
        if((strcmp(bigBufferPtr,"<")) == 0 && INPUT_REDIRECTION_FLAG == 1 && BACKSLASH_FLAG == 0){
            SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            getWordReturn = getword(bigBufferPtr);
            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                newArgV[wordCount] = NULL;
                return 0;
            }
            if(getWordReturn == -255){
                break;
            }
            // if(getWordReturn < 0 && getWordReturn != -255){} // previous conditional for handling -255 allows us to not need the second expression in the if statement
            if(getWordReturn < 0){
                getWordReturn = abs(getWordReturn);
            }
            if(getWordReturn > 0) {
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;  // continue reading in from stdin and catch this error once parse is done in the main function
            }
        }
        // Handling of output redirection
        if((strcmp(bigBufferPtr,">")) == 0  && OUTPUT_REDIRECTION_FLAG == 0 && BACKSLASH_FLAG == 0){
            OUTPUT_REDIRECTION_FLAG = 1; // set this flag so main() can handle output redirection
            // before calling getword() again we need to update the location of where bigBufferPtr is pointing to
            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
            getWordReturn = getword(bigBufferPtr);
            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
                newArgV[wordCount] = NULL;
                return 0;
            }
            if (getWordReturn == -255){
                break;
            }
            if (getWordReturn < 0){
                getWordReturn = abs(getWordReturn);
            }
            if (getWordReturn > 0){
                outputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
        }
        // Handling of more than one output redirection
        if((strcmp(bigBufferPtr,">")) == 0 && OUTPUT_REDIRECTION_FLAG == 1 && BACKSLASH_FLAG == 0){
            SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = 1;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            getWordReturn = getword(bigBufferPtr);
            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                newArgV[wordCount] = NULL;
                return 0;
            }
            if(getWordReturn == -255){
                break;
            }
            // if(getWordReturn < 0 && getWordReturn != -255){} // previous conditional for handling -255 allows us to not need the second expression in the if statement
            if(getWordReturn < 0){
                getWordReturn = abs(getWordReturn);
            }
            if(getWordReturn > 0) {
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;  // continue reading in from stdin and catch this error once parse is done in the main function
            }
        }
        // Handling a potential background process
        if((strcmp(bigBufferPtr,"&")) == 0 && BACKSLASH_FLAG == 0){
            tempPtr = bigBufferPtr; // make copy of location of '&' incase we need to add it to newArgV.
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            getWordReturn = getword(bigBufferPtr);
            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                WAIT_FLAG = 1;
                newArgV[wordCount] = NULL;
                return 0;
            }
            if(getWordReturn == -255){
                break;
            }
            if (getWordReturn < 0){ // negative getWordReturn size due to getword parsing a $before a "word"
                getWordReturn = abs(getWordReturn);
            }
            if(getWordReturn > 0){
                newArgV[wordCount++] = tempPtr; // add the '&' to newArgV; treat the '&' character as a argument for the command
                bigBufferPtrCpy = bigBufferPtr + getWordReturn - 1; // for reversing order
                // if getwordReturn == 1){ add space and possibly set backslashflag == 0
                // if bigBuffPrr is poiinting to meta chararacter; make while loop, dont forget to add space
                ungetc(SPACE, stdin); // to seperate the words
                while(*bigBufferPtrCpy != NULLTERM) {
                    ungetc(*bigBufferPtrCpy--, stdin); // note the reverse this order
                }
                if(BACKSLASH_FLAG == 1){
                    ungetc(BACKSLASH,stdin);
                    BACKSLASH_FLAG = 0;
                }
                // note bigBufferPtr is already in the correct location for the next iteration
                continue;
            }
        }
        // Handling of a pipeline
        if((strcmp(bigBufferPtr,"|")) == 0  && PIPELINE_FLAG == 0 && BACKSLASH_FLAG == 0){
            PIPELINE_FLAG = 1; // set this flag so main() can handle input redirection
            newArgV[wordCount++] = NULL;
            processTwoStartIndex = wordCount;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            continue;
        }
        // Handling multiple pipeline redirections
        if((strcmp(bigBufferPtr,"|")) == 0  && PIPELINE_FLAG == 1 && BACKSLASH_FLAG == 0){ //todo do I need to handle this
            SYNTAX_ERROR_MULTIPLE_PIPELINES = 1;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            continue; // continue reading in from stdin and catch this error once parse is done in the main function
        }

        // Handling a regular word
        newArgV[wordCount++] = bigBufferPtr;
        BACKSLASH_FLAG = 0;
        // Update pointer for next iteration
        bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
    }//note if the stdin
    return -255;
}
