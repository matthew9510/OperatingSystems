/* File: p2.c
 * Name: Matthew Hess
 * Instructor: John Carroll
 * Class: CS570
 * Due Date: 10/10/18 | 11/28/18
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
#include <libgen.h> // basename



/* Global Variable Declarations */
int BACKSLASH_FLAG = 0; // external declaration, used in getword.c
int BACKSLASH_FLAG_SQUIGGLE = 0;
int INPUT_REDIRECTION_FLAG = 0;
int OUTPUT_REDIRECTION_FLAG = 0;
int PIPELINE_FLAG = 0;
int WAIT_FLAG = 0;
int HEREIS_FLAG = 0;
int SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 0;
int SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = 0;
int SYNTAX_ERROR_MULTIPLE_PIPELINES = 0;
int SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 0;
int SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 0;
int SYNTAX_ERROR_MULTIPLE_SEQUENCE_OF_METACHARACTERS = 0;
int SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = 0;
int SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 0;
int SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 0;
int SYNTAX_ERROR_HEREIS_IDENTIFIER = 0;
int SYNTAX_ERROR_ENVVAR = 0;
int SYNTAX_ERROR_NO_EXECUTABLE = 0;
char bigBuffer[MAXITEM * STORAGE];
char * bigBufferPtr = bigBuffer;
char * newArgV[MAXITEM];
int processTwoStartIndex;
char * newArgVCommand2[MAXITEM];
int wordCount = 0;
char * inputRedirection;
char * outputRedirection;
char * hereIsKeyword;
char * envVarNameError;
int fileDescriptor[20];
pid_t kidpid, grandpid;
int numExecsCounter = 0; // the first process in the command line is process number zero; so the number of processes is equal to the current value of numExecsCounter + 1
int processOffsets[10]; // keeps track of some newArgV indexes, specifically the names of the executables
char userName[STORAGE]; // used for handling tilda '~' functionality to the shell
char newCommandArg[512]; // used for handling tilda '~' functionality to the shell
#define avgHomeDirectoryLength (20)
#define numPossibleAdditionalCharacters (253)
#define numTimesATildaCanBeSeenInOneCommand (128)
char tildaArray[numTimesATildaCanBeSeenInOneCommand * (avgHomeDirectoryLength + numPossibleAdditionalCharacters)];
int currentNumOfTildas = 0;
char * tildaArrayPtrs[numTimesATildaCanBeSeenInOneCommand];
char homeEnvVar[STORAGE];
int lengthOfHomeEnvVar = 0;
char * hereIsFileName = ".hereIshIddenFile";

int main() {
    /* Declaration of Locals */
    int returnValueParse;
    int inputFileFileDescriptorNum;
    int outputFileFileDescriptorNum;
    char cwd[256];
    char * endOfCwd = "";
    int i;


    /* Setting up the process group for this process */
    if( (setpgid(getpid(),0)) != 0){ // setpgid and if there is an error jump in else continue on to next statement
        fprintf(stderr, "Cant't change process group\n");
    }
    /* Handling the SIGTERM signal appropriately */
    (void) signal(SIGTERM, signalCatcher); // dont kill main, & kill all other processes

    for (;;) {
        /* Print Prompt to user */
        printf("%s:570: ",endOfCwd);
//        if(HEREIS_FLAG == 1){
//            fprintf(stderr,"Here is flag not reset");
//            break;
//        }
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
        if (SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META == 1) {
            fprintf(stderr, "You can't have a meta character as a filename for input redirection.\n");
            continue;
        }
        if (SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META == 1) {
            fprintf(stderr, "You can't have a meta character as a filename for output redirection.\n");
            continue;
        }
        if(SYNTAX_ERROR_HEREIS_IDENTIFIER == 1){
            fprintf(stderr, "Error with parsing \"hereis\" identifier.\n");
            continue;
        }
        if(SYNTAX_ERROR_ENVVAR == 1){
            fprintf(stderr, "%s: Undefined variable.\n", envVarNameError);
            continue;
        }
        if(SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND == 1){
            fprintf(stderr, "Undefined username for ~ functionality: %s\n", userName);
            continue;
        }
        if(HEREIS_FLAG == 1 && INPUT_REDIRECTION_FLAG == 1){
            fprintf(stderr, "Not allowed to have a hereis file and input redirection.\n");
            continue;
        }


        /* Handling if we read in the EOF character as the first character in stdin*/
        if (returnValueParse == -255 && wordCount == 0) {
            break;
        }
        if(wordCount == 0 && INPUT_REDIRECTION_FLAG == 1){
            fprintf(stderr,"No executable specified with input redirection\n");
            continue;
        }
        if(wordCount == 0 && OUTPUT_REDIRECTION_FLAG == 1){
            fprintf(stderr,"No executable specified with output redirection\n");
            continue;
        }
        if(wordCount == 0 && HEREIS_FLAG == 1){
            fprintf(stderr,"No executable specified with hereis redirection\n");
            continue;
        }
        /* If line is empty reissue prompt */
        if (wordCount == 0 && returnValueParse == 0) {
            continue;
        }
        if(wordCount == 0){
            fprintf(stderr, "No executable stated\n");
        }

        /* Handle possible pipeline errors*/
        for(i = 0; i <= numExecsCounter; i++){
            if(newArgV[processOffsets[i]] == NULL){
                SYNTAX_ERROR_NO_EXECUTABLE = 1;
                if(i == 0){
                    fprintf(stderr, "No executable stated.\n");
                }
                if (i > 0){
                    fprintf(stderr, "No executable stated after pipe number %d.\n",i);
                }
            }
        }
        if(SYNTAX_ERROR_NO_EXECUTABLE == 1){
            SYNTAX_ERROR_NO_EXECUTABLE = 0;
            continue;
        }

        /* Handling cd */
        if ((strcmp(newArgV[0], "cd")) == 0) {
            if (wordCount > 2) {
                fprintf(stderr, "Too many arguments for the command 'cd'.\n");
                continue;
            }
            else if (wordCount == 1) {
                char * envVar;
                envVar = getenv("HOME"); // if unsuccessful the next if statement will take care of it.
                if ((chdir(envVar)) == -1) { // If we can't change directory
                    fprintf(stderr, "Couldn't change to home directory.\n");
                    continue;
                }
                else {  // If changing directory was successful
                    (void) getcwd(cwd, sizeof(cwd));
                    endOfCwd = basename(cwd);
                    continue;
                }
            }
            else if (wordCount == 2){ // If a directory is explicitly stated
                if ((chdir(newArgV[1])) == -1) {// If we can't change to specified directory
                    fprintf(stderr, "Couldn't change to specified directory.\n");
                    continue;
                }
                else { // If changing directory was successful
                    (void) getcwd(cwd, sizeof(cwd));
                    endOfCwd = basename(cwd);
                    continue;
                }
            }
        }

        /*ENVIRON*/
        if(strcmp(newArgV[0], "environ") == 0){
            char * getEnvPtr;
            if (wordCount <= 1 || wordCount > 3){
                fprintf(stderr, "To little or to many arguments for the command environ.\n");
                continue;
            }
            if (wordCount == 2){
                getEnvPtr = getenv(newArgV[1]);
                if(getEnvPtr == NULL){
                    printf("\n");
                }
                else{
                    printf("%s\n",getEnvPtr);
                }
                continue;
            }
            if(wordCount == 3){
                if(setenv(newArgV[1],newArgV[2],1) == -1){
                    fprintf(stderr, "Cannot set environment variable.\n");
                    continue;
                }
                continue;
            }
        }
        if (HEREIS_FLAG == 1){
            char * lineptr;
            char lineBuffer[STORAGE];
            ssize_t lineLength;
            size_t *n;
            int getLineError = 0;
            int delimiterFound = 0;
            FILE *hereIsFile = fopen(hereIsFileName, "w+");
            if (hereIsFile == NULL){
                fprintf(stderr, "Can't open file for writing in hereis document.\n");
                continue;
            }

            //scan from stdin using getline
            //check line  if its a valid line I put in file
            //if its the Delimiter then break for loop
            while(delimiterFound != 1){
                if((lineLength = getline(&lineptr,n,stdin)) == -1){
                    fprintf(stderr,"Couldn't getline when handling hereis functionality");
                    getLineError = 1;
                    break;
                }
                strcpy(lineBuffer,lineptr);
                if(lineBuffer[lineLength-1] == '\n'){
                    lineBuffer[lineLength-1] = '\0';
                }
                if((strcmp(lineBuffer,hereIsKeyword)) == 0){
                    break;
                }
                else{ // if delimiter not found
                    (void) fwrite(lineptr,lineLength,1,hereIsFile);
                }
            }

            if ((fclose(hereIsFile)) == EOF){
                fprintf(stderr, "Couldn't close %s",hereIsFileName);
                continue;
            }
            if(getLineError == 1){
                if((remove(hereIsFileName)) == -1 ){
                    fprintf(stderr,"Couldn't remove file with file name %s",hereIsFileName);
                }
                continue;
            }
        }

        /* If a pipeline has been read in through the parse() function using the getword() function we will
                branch off to another function to handle pipelines appropriately*/
        if(PIPELINE_FLAG == 1){
            pipeline();
            continue; // once pipeline returns continue to reissue prompt and process next command
        }

        // Before forking Flush the data streams
        fflush(stdout);
        fflush(stderr);
        // Fork to execute a child process - to handle rightmost process
        // CHK((kidpid = fork())) todo check if we need to implement this for forking
        if ((kidpid = fork()) == -1){ // fork and if there is an error jump into if statement otherwise continue on to next statement
            fprintf(stderr,"Fork Failed");
            continue;
        }
        else if (kidpid == 0){ // If child process forked successfully
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
            /* Handle hereis */
            if (HEREIS_FLAG == 1){
                int hereisFileDescriptorNum = open(hereIsFileName,O_RDONLY);
                if (hereisFileDescriptorNum == -1){
                    fprintf(stderr, "Couldn't open %s",hereIsFileName);
                    exit(ERROR10);
                }
                CHK(dup2(hereisFileDescriptorNum, STDIN_FILENO));
                close(hereisFileDescriptorNum);
                remove(hereIsFileName);
            }

            /* After all redirections are satisfied execute the process */
            if ((execvp(newArgV[0],newArgV)) == -1){
                fprintf(stderr, "Couldn't execute command\n");
                exit(ERROR3);
            }

        } // End of child process scope
        //else if(kidpid != 0){} we are in parent process scope
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
    if(killpg(getpgrp(), SIGTERM) == -1){
        fprintf(stderr,"killpg failed.");
    }
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

    /* Declaration of locals */
    int accessInputRedirectionfile;
    int inputFileFileDescriptorNum;
    int outputFileFileDescriptorNum;
    int rightMostPid;
    int middlePid;
    int outP2Pid;
    int i;
    int maxPipes = numExecsCounter;
    int loopCount;
    int pipeCount = 0;

    /* To Get Out Of p2 */
    /* Before forking, flush out stderr and stdout */
    fflush(stdout);
    fflush(stderr);
    CHK((outP2Pid = fork())); // to get out of p2
    if (outP2Pid == 0) { //This is not the right most process // !(child, right most process)
        /*Create All pipes*/
        for(i = 0; i < 10; i++){
            CHK(pipe(fileDescriptor + (i*2)));
        }
        fflush(stdout);
        fflush(stderr);
        CHK((rightMostPid = fork())); // right most process
        if (rightMostPid == 0){
            for (loopCount = maxPipes-1, pipeCount = 1; loopCount >= 0; loopCount--, pipeCount++){
                fflush(stdout);
                fflush(stderr);
                if(loopCount == 0){
                    /* Handle hereis */
                    if (HEREIS_FLAG == 1 && hereIsKeyword != NULL){

                    }
                    /* Handling of input redirection */
                    if (INPUT_REDIRECTION_FLAG == 1 && inputRedirection != NULL) {
                        // does the file exist and do I have the correct permissions, using the calling processe's real UID & GID
                        CHK((accessInputRedirectionfile = access(inputRedirection, R_OK)));
                        if(accessInputRedirectionfile == -1){
                            //return; // todo get checked what do i do here?
                            exit(ERROR8);
                        }
                        if (accessInputRedirectionfile == 0) {
                            inputFileFileDescriptorNum = open(inputRedirection, O_RDONLY);
                            if (inputFileFileDescriptorNum == -1) {
                                fprintf(stderr, "Couldn't open the input redirection file\n");
                                exit(ERROR9);
                                //return; // todo Not in forloop, return a number to signify to continue, check if return is the right thing to do
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
                    CHK(dup2(fileDescriptor[pipeCount*2-1], STDOUT_FILENO));

                    /* Close the read end of the pipe so that the pipe doesn't wait for more information to be transmitted */
                    for(i = 0; i < 20; i++){
                        close(fileDescriptor[i]);
                    }
                    /* Handle hereis */
                    if (HEREIS_FLAG == 1){
                        int hereisFileDescriptorNum = open(hereIsFileName,O_RDONLY);
                        if (hereisFileDescriptorNum == -1){
                            fprintf(stderr, "Couldn't open %s",hereIsFileName);
                            exit(ERROR10);
                        }
                        CHK(dup2(hereisFileDescriptorNum, STDIN_FILENO));
                        close(hereisFileDescriptorNum);
                        remove(hereIsFileName);
                    }
                    /* Execute Command 1 */
                    if ((execvp(newArgV[processOffsets[loopCount]], newArgV + processOffsets[loopCount])) == -1) {
                        fprintf(stderr, "Couldn't execute command\n");
                        exit(ERROR3); //todo create a new Error#
                    }
                }
                CHK((middlePid = fork()));
                if(middlePid == 0){

                }
                else if(middlePid != 0){  // second to last process
                    dup2(fileDescriptor[pipeCount*2-1],STDOUT_FILENO);
                    dup2(fileDescriptor[pipeCount*2],STDIN_FILENO);
                    /* Close the read end of the pipe so that the pipe doesn't wait for more information to be transmitted */
                    for(i = 0; i < 20; i++){
                        close(fileDescriptor[i]);
                    }
                    CHK(execvp(newArgV[processOffsets[loopCount]],newArgV + processOffsets[loopCount]));
                }
            }
            //outside of loop
        }
        else if(rightMostPid != 0){ // right most process
            // finish pre-processing of right most process
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
            for(i = 0; i < 20; i++){
                close(fileDescriptor[i]);
            }

            /* Execute the right most command */
            if ((execvp(newArgV[processOffsets[numExecsCounter]], newArgV + processOffsets[numExecsCounter])) == -1) { // todo change newArgV[processOffsets[numExecs]]
                fprintf(stderr, "Couldn't execute command\n");
                exit(ERROR5);
            } // End of child Process
        }
    }
    else if (outP2Pid != 0){
        //does wait
        if (WAIT_FLAG == 1) {
            printf("%s [%d]\n", *newArgV, outP2Pid);
            return;
        }
        else {
            for (;;) {
                pid_t find_pid;
                find_pid = wait(NULL);
                if (find_pid == outP2Pid) {
                    return;
                }
            }
        }
    }
}

int parse(){
    /* Declaration of Locals */
    char * tempPtr;  // used for handling if we see the '&' character and the next character isn't a ';' 'NEWLINE' or '$ '
    int getWordReturn;
    int i;
    char * currentTildaNewCommandArgPtr = tildaArray;
    char * envVar; // declare a variable that will point to the system call getenv() function
    HEREIS_FLAG = 0;
    lengthOfHomeEnvVar = 0;
    /* get the current HOME enviroment variable path and load it into homeEnvVar a global variable */
    for (i=0, envVar = getenv("HOME"); *envVar != '\0';  envVar++, i++ ){
        // start the for loop with the first character from the returned character array from the getenv() system call, and terminate the for loop when we reach the null terminator in the character array
        homeEnvVar[i] = *envVar;
        lengthOfHomeEnvVar++;
    }
    homeEnvVar[i] = NULLTERM;
    lengthOfHomeEnvVar++;

    /* Reset all global declarations to support invoking parse() multiple times */
    INPUT_REDIRECTION_FLAG = OUTPUT_REDIRECTION_FLAG = PIPELINE_FLAG = WAIT_FLAG = BACKSLASH_FLAG = wordCount = getWordReturn = HEREIS_FLAG = 0;
    SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = SYNTAX_ERROR_MULTIPLE_SEQUENCE_OF_METACHARACTERS = SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = SYNTAX_ERROR_HEREIS_IDENTIFIER = SYNTAX_ERROR_ENVVAR = SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 0;
    inputRedirection = outputRedirection = tempPtr = hereIsKeyword = envVarNameError = NULL;
    bigBufferPtr = bigBuffer;
    processOffsets[0] = 0; // the first executable name is located at newArgV[0]
    numExecsCounter = 0;
    currentNumOfTildas = 0;

    for(i = 0; i < 10; i++){ // todo check if this is redundant?
        processOffsets[i] = 0;
    }
    for ( i = 0; i < MAXITEM * STORAGE ; i++){
        bigBuffer[i] = NULLTERM;
    }
    for(i = 0; i < MAXITEM; i++){
        newArgV[i] = NULL;
    }

    /* Parse stdin using the function getword() and set appropriate flags and load appropriate data structures */
    // Note flags set will be handled in p2.c main()
    while(((getWordReturn = getword(bigBufferPtr))) != -255) { // Note that if stdin is empty it returns EOF, and in getword() if (charCount == 0 and iochar == EOF) getword() returns -255 to end the parse function by returning -255

        /* Handle substituting '~' for the correct path */
        if(strncmp(bigBufferPtr,"~",1) == 0 && BACKSLASH_FLAG_SQUIGGLE == 0){
            if (getWordReturn == 1){
                newArgV[wordCount++] = homeEnvVar;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            else{
                if(strncmp(bigBufferPtr,"~/",2) == 0 ){
                    char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                    for (i = 0; i < lengthOfHomeEnvVar-1; i++){
                        *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
                    }
                    for (i = 1; i < getWordReturn; i++){
                        *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
                    }
                    *(currentTildaNewCommandArgPtr++) = NULLTERM;
                    tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                    newArgV[wordCount++] = tildaArrayPtrs[currentNumOfTildas++];
                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                    continue;
                }
                    /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
                else{
                    char * startOfUserName;
                    char * userNamePtr;
                    char * startAdditionalCharacterPtr;
                    char * additionalCharactersPtr;
                    //note: (char userName[STORAGE]) was declared as a global variable for error handling
                    char additionalPath[STORAGE];
                    FILE * passwdFile;
                    /* Setup for parsing through the user entries in the file */
                    /* Declarations for using the getline function */
                    char * line = NULL;
                    size_t bufferSize = 0;
                    int lineFoundFlag = 0;
                    int eofFound = 0;
                    size_t lineSize;
                    /* Declarations for using the strtok function */
                    char * delimiter = ":";
                    char * token; // used to tokenize the correct user entry
                    char * absolutePathToHomeDirectory;
                    int additionalFlag = 0;
                    /* Declarations needed to form the new command argument */
                    // note: (char newCommandArg[512]) was declared as a global variable
                    char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                    char * tmpPtr = absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
                    int lengthOfAbsolutePathToHomeDirectory = 0;
                    int numOfAdditionalCharacters = 0;
                    int lengthOfNewCommandArg;


                    /* copy username to userName array*/
                    startOfUserName = bigBufferPtr + 1;
                    userNamePtr = startOfUserName;
                    i = 0;
                    /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
                    while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
                        userName[i++] = *userNamePtr++;
                    }
                    userName[i] = NULLTERM; // null terminate the username string

                    /* Handle collecting possible additional characters to eventually append to end of absolute path */
                    startAdditionalCharacterPtr = userNamePtr;
                    additionalCharactersPtr = startAdditionalCharacterPtr;
                    i = 0;
                    /* create and load an array to gather additional path to eventually append to absolute path */
                    while(*additionalCharactersPtr != NULLTERM){
                        additionalFlag = 1;
                        additionalPath[i++] = *additionalCharactersPtr++;
                    }
                    if (additionalFlag == 1){
                        additionalPath[i] = NULLTERM;
                    }

                    /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
                    passwdFile = fopen("/etc/passwd", "r");


                    /* Parse the file */
                    while(lineFoundFlag != 1){
                        lineSize = getline(&line, &bufferSize, passwdFile);
                        token = strtok(line, delimiter);

                        /* if we have found the end of the file or there was an error reading a line from the file */
                        if (lineSize == -1){
                            eofFound = 1;
                            break;
                        }

                        /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
                        if (strcmp(token,userName) == 0){
                            lineFoundFlag = 1;

                            /*the correct absolute path name to the home directory is the fifth element in the user entry*/
                            for (i = 0; i < 5; i++){
                                line = NULL; // do this to get the next token, read strtok docs for an explanation
                                token = strtok(line, delimiter);
                            }
                            absolutePathToHomeDirectory = token; // save the pointer
                            tmpPtr = absolutePathToHomeDirectory; // for copying the absolute path to a tilda array
                            continue;
                        }
                        /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
                        if(strcmp(token,userName) != 0){
                            line = line + lineSize;
                        }
                    }
                    /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
                    if(eofFound == 1){
                        SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }

                    /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
                    /*copy absolute path of home directory of a particular user */
                    i = 0;
                    while ((strncmp(tmpPtr,"\0",1)) != 0){
                        *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
                        lengthOfAbsolutePathToHomeDirectory++;
                    }

                    /* possibly append the additional characters of new command argument */
                    if (additionalFlag == 1){
                        i = 0;
                        while(additionalPath[i] != NULLTERM){
                            *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
                            numOfAdditionalCharacters++;
                        }
                        *((currentTildaNewCommandArgPtr++)) = NULLTERM;
                    }
                    if(additionalFlag == 0){
                        *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
                    }
                    lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
                    tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                    newArgV[wordCount++] = tildaArrayPtrs[currentNumOfTildas++];
                    // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                    continue;
                }
            }
        }
        /* Handling if the getword() function parsed either a SEMICOLON or a NEWLINE, or a DOLLARSIGN followed by a space or newline */
        if(getWordReturn == 0 && BACKSLASH_FLAG == 0 ){
            if(wordCount > 0){
                if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
                    if (strcmp(newArgV[wordCount - 1], "&") == 0) {
                        newArgV[--wordCount] = NULL;
                        WAIT_FLAG = 1;
                        return 0;
                    }
                }
            }
            newArgV[wordCount] = NULL;
            return 0;
        }
        // Handling if the getword() function parsed a dollar sign at the beginning of a word, we need to strip the minus sign from the getwordReturn for prog2
        if (getWordReturn < 0 ){
            getWordReturn = abs(getWordReturn);
            newArgV[wordCount] = getenv(bigBufferPtr);
            if(newArgV[wordCount] == NULL){
                SYNTAX_ERROR_ENVVAR = 1;
                envVarNameError = bigBufferPtr;
                newArgV[wordCount] = NULL;
            }
            wordCount++;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
            continue;
        }

        /* Handle Seeing flags */
        // Handling of input redirection
        if((strcmp(bigBufferPtr,"<")) == 0 && INPUT_REDIRECTION_FLAG == 0 && BACKSLASH_FLAG == 0){
            INPUT_REDIRECTION_FLAG = 1;
            // before calling getword() again we need to update the location of where bigBufferPtr is pointing to
            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
            // call getword again to handle the next "word in stdin" appropriately
            getWordReturn = getword(bigBufferPtr);

            /* Handle substituting '~' for the correct path */
            if(strncmp(bigBufferPtr,"~",1) == 0 && BACKSLASH_FLAG_SQUIGGLE == 0 ){
                if (getWordReturn == 1){
                    inputRedirection = homeEnvVar;
                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                    continue;
                }
                else{
                    if(strncmp(bigBufferPtr,"~/",2) == 0 ){
                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                        for (i = 0; i < lengthOfHomeEnvVar-1; i++){
                            *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
                        }
                        for (i = 1; i < getWordReturn; i++){
                            *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
                        }
                        *(currentTildaNewCommandArgPtr++) = NULLTERM;
                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                        inputRedirection = tildaArrayPtrs[currentNumOfTildas++];
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }
                        /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
                    else{
                        char * startOfUserName;
                        char * userNamePtr;
                        char * startAdditionalCharacterPtr;
                        char * additionalCharactersPtr;
                        //note: (char userName[STORAGE]) was declared as a global variable for error handling
                        char additionalPath[STORAGE];
                        FILE * passwdFile;
                        /* Setup for parsing through the user entries in the file */
                        /* Declarations for using the getline function */
                        char * line = NULL;
                        size_t bufferSize = 0;
                        int lineFoundFlag = 0;
                        int eofFound = 0;
                        size_t lineSize;
                        /* Declarations for using the strtok function */
                        char * delimiter = ":";
                        char * token; // used to tokenize the correct user entry
                        char * absolutePathToHomeDirectory;
                        int additionalFlag = 0;
                        /* Declarations needed to form the new command argument */
                        // note: (char newCommandArg[512]) was declared as a global variable
                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                        char * tmpPtr = absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
                        int lengthOfAbsolutePathToHomeDirectory = 0;
                        int numOfAdditionalCharacters = 0;
                        int lengthOfNewCommandArg;


                        /* copy username to userName array*/
                        startOfUserName = bigBufferPtr + 1;
                        userNamePtr = startOfUserName;
                        i = 0;
                        /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
                        while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
                            userName[i++] = *userNamePtr++;
                        }
                        userName[i] = NULLTERM; // null terminate the username string

                        /* Handle collecting possible additional characters to eventually append to end of absolute path */
                        startAdditionalCharacterPtr = userNamePtr;
                        additionalCharactersPtr = startAdditionalCharacterPtr;
                        i = 0;
                        /* create and load an array to gather additional path to eventually append to absolute path */
                        while(*additionalCharactersPtr != NULLTERM){
                            additionalFlag = 1;
                            additionalPath[i++] = *additionalCharactersPtr++;
                        }
                        if (additionalFlag == 1){
                            additionalPath[i] = NULLTERM;
                        }

                        /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
                        passwdFile = fopen("/etc/passwd", "r");


                        /* Parse the file */
                        while(lineFoundFlag != 1){
                            lineSize = getline(&line, &bufferSize, passwdFile);
                            token = strtok(line, delimiter);

                            /* if we have found the end of the file or there was an error reading a line from the file */
                            if (lineSize == -1){
                                eofFound = 1;
                                break;
                            }

                            /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
                            if (strcmp(token,userName) == 0){
                                lineFoundFlag = 1;

                                /*the correct absolute path name to the home directory is the fifth element in the user entry*/
                                for (i = 0; i < 5; i++){
                                    line = NULL; // do this to get the next token, read strtok docs for an explanation
                                    token = strtok(line, delimiter);
                                }
                                absolutePathToHomeDirectory = token; // save the pointer
                                tmpPtr = absolutePathToHomeDirectory; // for copying the absolute path to a tilda array
                                continue;
                            }
                            /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
                            if(strcmp(token,userName) != 0){
                                line = line + lineSize;
                            }
                        }
                        /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
                        if(eofFound == 1){
                            SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
                            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                            continue;
                        }

                        /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
                        /*copy absolute path of home directory of a particular user */
                        i = 0;
                        while ((strncmp(tmpPtr,"\0",1)) != 0){
                            *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
                            lengthOfAbsolutePathToHomeDirectory++;
                        }

                        /* possibly append the additional characters of new command argument */
                        if (additionalFlag == 1){
                            i = 0;
                            while(additionalPath[i] != NULLTERM){
                                *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
                                numOfAdditionalCharacters++;
                            }
                            *((currentTildaNewCommandArgPtr++)) = NULLTERM;
                        }
                        if(additionalFlag == 0){
                            *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
                        }
                        lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                        inputRedirection = tildaArrayPtrs[currentNumOfTildas++];
                        // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }
                }
            }

            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
                if(wordCount > 0){
                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
                            newArgV[--wordCount] = NULL;
                            WAIT_FLAG = 1;
                            return 0;
                        }
                    }
                }
                newArgV[wordCount] = NULL;
                return 0;
            }
            // Note: if(getWordReturn == 0 && BACKSLASH_FLAG == 1) would never trjgger
            // Note: (strcmp(bigBufferPtr,"\n") == 0  && getWordReturn == 0 && BACKSLASH_FLAG == 1) handled in getword.c
            if(getWordReturn == -255){
                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
                break;
            }
            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
                getWordReturn = abs(getWordReturn);
                inputRedirection = getenv(bigBufferPtr);
                if(inputRedirection == NULL){
                    SYNTAX_ERROR_ENVVAR = 1;
                    envVarNameError = bigBufferPtr;
                }
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter*/
            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
                SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
                BACKSLASH_FLAG = 0;
                inputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }

            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
                inputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
                BACKSLASH_FLAG = 0;
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
                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
                if(wordCount > 0){
                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
                            newArgV[--wordCount] = NULL;
                            WAIT_FLAG = 1;
                            return 0;
                        }
                    }
                }
                newArgV[wordCount] = NULL;
                return 0;
            }
            // Note: if(getWordReturn == 0 && BACKSLASH_FLAG == 1) would never trjgger
            // Note: (strcmp(bigBufferPtr,"\n") == 0  && getWordReturn == 0 && BACKSLASH_FLAG == 1) handled in getword.c
            if(getWordReturn == -255){
                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
                break;
            }
            if(getWordReturn < 0){ // handling if getword returns a negative return value, note EOF is taken care of already
                getWordReturn = abs(getWordReturn);
                inputRedirection = getenv(bigBufferPtr);
                if(inputRedirection == NULL){
                    SYNTAX_ERROR_ENVVAR = 1;
                    envVarNameError = bigBufferPtr;
                }
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter*/
            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
                SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
                BACKSLASH_FLAG = 0;
                inputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }

            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
                inputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
                BACKSLASH_FLAG = 0;
                inputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
        }
        // Handling of output redirection
        if((strcmp(bigBufferPtr,">")) == 0  && OUTPUT_REDIRECTION_FLAG == 0 && BACKSLASH_FLAG == 0){
            OUTPUT_REDIRECTION_FLAG = 1; // set this flag so main() can handle output redirection
            // before calling getword() again we need to update the location of where bigBufferPtr is pointing to
            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
            getWordReturn = getword(bigBufferPtr);

            /* Handle substituting '~' for the correct path */
            if(strncmp(bigBufferPtr,"~",1) == 0 && BACKSLASH_FLAG_SQUIGGLE == 0 ){
                if (getWordReturn == 1){
                    outputRedirection = homeEnvVar;
                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                    continue;
                }
                else{
                    if(strncmp(bigBufferPtr,"~/",2) == 0 ){
                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                        for (i = 0; i < lengthOfHomeEnvVar-1; i++){
                            *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
                        }
                        for (i = 1; i < getWordReturn; i++){
                            *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
                        }
                        *(currentTildaNewCommandArgPtr++) = NULLTERM;
                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                        outputRedirection = tildaArrayPtrs[currentNumOfTildas++];
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }
                        /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
                    else{
                        char * startOfUserName;
                        char * userNamePtr;
                        char * startAdditionalCharacterPtr;
                        char * additionalCharactersPtr;
                        //note: (char userName[STORAGE]) was declared as a global variable for error handling
                        char additionalPath[STORAGE];
                        FILE * passwdFile;
                        /* Setup for parsing through the user entries in the file */
                        /* Declarations for using the getline function */
                        char * line = NULL;
                        size_t bufferSize = 0;
                        int lineFoundFlag = 0;
                        int eofFound = 0;
                        size_t lineSize;
                        /* Declarations for using the strtok function */
                        char * delimiter = ":";
                        char * token; // used to tokenize the correct user entry
                        char * absolutePathToHomeDirectory;
                        int additionalFlag = 0;
                        /* Declarations needed to form the new command argument */
                        // note: (char newCommandArg[512]) was declared as a global variable
                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                        char * tmpPtr = absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
                        int lengthOfAbsolutePathToHomeDirectory = 0;
                        int numOfAdditionalCharacters = 0;
                        int lengthOfNewCommandArg;


                        /* copy username to userName array*/
                        startOfUserName = bigBufferPtr + 1;
                        userNamePtr = startOfUserName;
                        i = 0;
                        /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
                        while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
                            userName[i++] = *userNamePtr++;
                        }
                        userName[i] = NULLTERM; // null terminate the username string

                        /* Handle collecting possible additional characters to eventually append to end of absolute path */
                        startAdditionalCharacterPtr = userNamePtr;
                        additionalCharactersPtr = startAdditionalCharacterPtr;
                        i = 0;
                        /* create and load an array to gather additional path to eventually append to absolute path */
                        while(*additionalCharactersPtr != NULLTERM){
                            additionalFlag = 1;
                            additionalPath[i++] = *additionalCharactersPtr++;
                        }
                        if (additionalFlag == 1){
                            additionalPath[i] = NULLTERM;
                        }

                        /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
                        passwdFile = fopen("/etc/passwd", "r");


                        /* Parse the file */
                        while(lineFoundFlag != 1){
                            lineSize = getline(&line, &bufferSize, passwdFile);
                            token = strtok(line, delimiter);

                            /* if we have found the end of the file or there was an error reading a line from the file */
                            if (lineSize == -1){
                                eofFound = 1;
                                break;
                            }

                            /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
                            if (strcmp(token,userName) == 0){
                                lineFoundFlag = 1;

                                /*the correct absolute path name to the home directory is the fifth element in the user entry*/
                                for (i = 0; i < 5; i++){
                                    line = NULL; // do this to get the next token, read strtok docs for an explanation
                                    token = strtok(line, delimiter);
                                }
                                absolutePathToHomeDirectory = token; // save the pointer
                                tmpPtr = absolutePathToHomeDirectory; // for copying the absolute path to a tilda array
                                continue;
                            }
                            /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
                            if(strcmp(token,userName) != 0){
                                line = line + lineSize;
                            }
                        }
                        /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
                        if(eofFound == 1){
                            SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
                            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                            continue;
                        }

                        /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
                        /*copy absolute path of home directory of a particular user */
                        i = 0;
                        while ((strncmp(tmpPtr,"\0",1)) != 0){
                            *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
                            lengthOfAbsolutePathToHomeDirectory++;
                        }

                        /* possibly append the additional characters of new command argument */
                        if (additionalFlag == 1){
                            i = 0;
                            while(additionalPath[i] != NULLTERM){
                                *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
                                numOfAdditionalCharacters++;
                            }
                            *((currentTildaNewCommandArgPtr++)) = NULLTERM;
                        }
                        if(additionalFlag == 0){
                            *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
                        }
                        lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                        outputRedirection = tildaArrayPtrs[currentNumOfTildas++];
                        // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }
                }
            }

            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
                if(wordCount > 0){
                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
                            newArgV[--wordCount] = NULL;
                            WAIT_FLAG = 1;
                            return 0;
                        }
                    }
                }
                newArgV[wordCount] = NULL;
                return 0;
            }
            if (getWordReturn == -255){
                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
                break;
            }
            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
                getWordReturn = abs(getWordReturn);
                outputRedirection = getenv(bigBufferPtr);
                if(outputRedirection == NULL){
                    SYNTAX_ERROR_ENVVAR = 1;
                    envVarNameError = bigBufferPtr;
                }
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==0){
                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==1){
                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter*/
            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
                BACKSLASH_FLAG = 0;
                outputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }

            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
                outputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
                BACKSLASH_FLAG = 0;
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
                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
                if(wordCount > 0){
                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
                            newArgV[--wordCount] = NULL;
                            WAIT_FLAG = 1;
                            return 0;
                        }
                    }
                }
                newArgV[wordCount] = NULL;
                return 0;
            }
            if (getWordReturn == -255){
                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
                break;
            }
            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
                getWordReturn = abs(getWordReturn);
                outputRedirection = getenv(bigBufferPtr);
                if(outputRedirection == NULL){
                    SYNTAX_ERROR_ENVVAR = 1;
                    envVarNameError = bigBufferPtr;
                }
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==0){
                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==1){
                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter*/
            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
                BACKSLASH_FLAG = 0;
                outputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }

            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
                outputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
                BACKSLASH_FLAG = 0;
                outputRedirection = bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
        }

        // Handling a "where is" file
        if((strcmp(bigBufferPtr,"<<")) == 0 && BACKSLASH_FLAG == 0){
            HEREIS_FLAG = 1;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            getWordReturn = getword(bigBufferPtr);

            /* Handle substituting '~' for the correct path */
            if(strncmp(bigBufferPtr,"~",1) == 0 && BACKSLASH_FLAG_SQUIGGLE == 0 ){
                if (getWordReturn == 1){
                    hereIsKeyword = homeEnvVar;
                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                    continue;
                }
                else{
                    if(strncmp(bigBufferPtr,"~/",2) == 0 ){
                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                        for (i = 0; i < lengthOfHomeEnvVar-1; i++){
                            *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
                        }
                        for (i = 1; i < getWordReturn; i++){
                            *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
                        }
                        *(currentTildaNewCommandArgPtr++) = NULLTERM;
                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                        hereIsKeyword = tildaArrayPtrs[currentNumOfTildas++];
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }
                        /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
                    else{
                        char * startOfUserName;
                        char * userNamePtr;
                        char * startAdditionalCharacterPtr;
                        char * additionalCharactersPtr;
                        //note: (char userName[STORAGE]) was declared as a global variable for error handling
                        char additionalPath[STORAGE];
                        FILE * passwdFile;
                        /* Setup for parsing through the user entries in the file */
                        /* Declarations for using the getline function */
                        char * line = NULL;
                        size_t bufferSize = 0;
                        int lineFoundFlag = 0;
                        int eofFound = 0;
                        size_t lineSize;
                        /* Declarations for using the strtok function */
                        char * delimiter = ":";
                        char * token; // used to tokenize the correct user entry
                        char * absolutePathToHomeDirectory;
                        int additionalFlag = 0;
                        /* Declarations needed to form the new command argument */
                        // note: (char newCommandArg[512]) was declared as a global variable
                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
                        char * tmpPtr = absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
                        int lengthOfAbsolutePathToHomeDirectory = 0;
                        int numOfAdditionalCharacters = 0;
                        int lengthOfNewCommandArg;


                        /* copy username to userName array*/
                        startOfUserName = bigBufferPtr + 1;
                        userNamePtr = startOfUserName;
                        i = 0;
                        /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
                        while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
                            userName[i++] = *userNamePtr++;
                        }
                        userName[i] = NULLTERM; // null terminate the username string

                        /* Handle collecting possible additional characters to eventually append to end of absolute path */
                        startAdditionalCharacterPtr = userNamePtr;
                        additionalCharactersPtr = startAdditionalCharacterPtr;
                        i = 0;
                        /* create and load an array to gather additional path to eventually append to absolute path */
                        while(*additionalCharactersPtr != NULLTERM){
                            additionalFlag = 1;
                            additionalPath[i++] = *additionalCharactersPtr++;
                        }
                        if (additionalFlag == 1){
                            additionalPath[i] = NULLTERM;
                        }

                        /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
                        passwdFile = fopen("/etc/passwd", "r");


                        /* Parse the file */
                        while(lineFoundFlag != 1){
                            lineSize = getline(&line, &bufferSize, passwdFile);
                            token = strtok(line, delimiter);

                            /* if we have found the end of the file or there was an error reading a line from the file */
                            if (lineSize == -1){
                                eofFound = 1;
                                break;
                            }

                            /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
                            if (strcmp(token,userName) == 0){
                                lineFoundFlag = 1;

                                /*the correct absolute path name to the home directory is the fifth element in the user entry*/
                                for (i = 0; i < 5; i++){
                                    line = NULL; // do this to get the next token, read strtok docs for an explanation
                                    token = strtok(line, delimiter);
                                }
                                absolutePathToHomeDirectory = token; // save the pointer
                                tmpPtr = absolutePathToHomeDirectory; // for copying the absolute path to a tilda array
                                continue;
                            }
                            /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
                            if(strcmp(token,userName) != 0){
                                line = line + lineSize;
                            }
                        }
                        /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
                        if(eofFound == 1){
                            SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
                            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                            continue;
                        }

                        /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
                        /*copy absolute path of home directory of a particular user */
                        i = 0;
                        while ((strncmp(tmpPtr,"\0",1)) != 0){
                            *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
                            lengthOfAbsolutePathToHomeDirectory++;
                        }

                        /* possibly append the additional characters of new command argument */
                        if (additionalFlag == 1){
                            i = 0;
                            while(additionalPath[i] != NULLTERM){
                                *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
                                numOfAdditionalCharacters++;
                            }
                            *((currentTildaNewCommandArgPtr++)) = NULLTERM;
                        }
                        if(additionalFlag == 0){
                            *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
                        }
                        lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
                        hereIsKeyword = tildaArrayPtrs[currentNumOfTildas++];
                        // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                        continue;
                    }
                }
            }

            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
                if(wordCount > 0){
                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
                            newArgV[--wordCount] = NULL;
                            WAIT_FLAG = 1;
                            return 0;
                        }
                    }
                }
                newArgV[wordCount] = NULL;
                return 0;
            }
            if(getWordReturn == -255){
                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
                break;
            }
            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
                getWordReturn = abs(getWordReturn);
                hereIsKeyword = getenv(bigBufferPtr);
                if(hereIsKeyword == NULL){
                    SYNTAX_ERROR_ENVVAR = 1;
                    envVarNameError = bigBufferPtr;
                }
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }
            /* Handling seeing multiple hereis meta characters one after another */
            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0){
                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter*/
            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
                continue;
            }
            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
                BACKSLASH_FLAG = 0;
                hereIsKeyword =  bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
                continue;
            }

            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
                hereIsKeyword =  bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1){
                BACKSLASH_FLAG = 0;
                hereIsKeyword =  bigBufferPtr;
                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
                continue;
            }
        }

        // Handling a potential background process
        if((strcmp(bigBufferPtr,"&")) == 0 && BACKSLASH_FLAG == 0){
            newArgV[wordCount++] = bigBufferPtr;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            continue;
        }
        // Handling of a single or multiple pipelines, keeps track of locations of executable names by tracking indexes values of newArgV
        if((strcmp(bigBufferPtr,"|")) == 0  && BACKSLASH_FLAG == 0){
            PIPELINE_FLAG = 1; // set this flag so main() can handle accordingly 
            newArgV[wordCount++] = NULL;
            processOffsets[++numExecsCounter] = wordCount;
            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
            continue;
        }

        // Handling a regular word
        newArgV[wordCount++] = bigBufferPtr;
        BACKSLASH_FLAG = 0;
        // Update pointer for next iteration
        bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
    }//note if the stdin
    return -255;
}
