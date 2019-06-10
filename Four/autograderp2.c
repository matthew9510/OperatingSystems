     1	/* File: p2.c
     2	 * Name: Matthew Hess
     3	 * Instructor: John Carroll
     4	 * Class: CS570
     5	 * Due Date: 10/10/18
     6	 */
     7	
     8	/* Include Files */
     9	#include <stdio.h> // EOF is a variable in this module , includes fflush
    10	#include <stdlib.h> // genenv() system call utilized from this library
    11	#include "p2.h"
    12	#include "getword.h"
    13	#include <math.h>
    14	#include <unistd.h>  // for chdir() system call, fork() system call, access() system call
    15	#include <sys/types.h> // for open()
    16	#include <sys/stat.h> // for open()
    17	#include <fcntl.h> // for open()
    18	#include <signal.h>
    19	#include <string.h>
    20	#include <errno.h>
    21	#include <sys/wait.h>
    22	#include "CHK.h" //todo use this function
    23	#include <libgen.h> // basename
    24	
    25	
    26	
    27	/* Global Variable Declarations */
    28	int BACKSLASH_FLAG = 0; // external declaration, used in getword.c
    29	int INPUT_REDIRECTION_FLAG = 0;
    30	int OUTPUT_REDIRECTION_FLAG = 0;
    31	int PIPELINE_FLAG = 0;
    32	int WAIT_FLAG = 0;
    33	int HEREIS_FLAG = 0;
    34	int SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 0;
    35	int SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = 0;
    36	int SYNTAX_ERROR_MULTIPLE_PIPELINES = 0;
    37	int SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 0;
    38	int SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 0;
    39	int SYNTAX_ERROR_MULTIPLE_SEQUENCE_OF_METACHARACTERS = 0;
    40	int SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = 0;
    41	int SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 0;
    42	int SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 0;
    43	int SYNTAX_ERROR_HEREIS_IDENTIFIER = 0;
    44	int SYNTAX_ERROR_ENVVAR = 0;
    45	char bigBuffer[MAXITEM * STORAGE];
    46	char * bigBufferPtr = bigBuffer;
    47	char * newArgV[MAXITEM];
    48	int processTwoStartIndex;
    49	char * newArgVCommand2[MAXITEM];
    50	int wordCount = 0;
    51	char * inputRedirection;
    52	char * outputRedirection;
    53	char * hereIsKeyword;
    54	char * envVarNameError;
    55	int fileDescriptor[20];
    56	pid_t kidpid, grandpid;
    57	int numExecsCounter = 0; // the first process in the command line is process number zero; so the number of processes is equal to the current value of numExecsCounter + 1
    58	int processOffsets[10]; // keeps track of some newArgV indexes, specifically the names of the executables
    59	char userName[STORAGE]; // used for handling tilda '~' functionality to the shell
    60	char newCommandArg[512]; // used for handling tilda '~' functionality to the shell
    61	#define avgHomeDirectoryLength (20)
    62	#define numPossibleAdditionalCharacters (253)
    63	#define numTimesATildaCanBeSeenInOneCommand (128)
    64	char tildaArray[numTimesATildaCanBeSeenInOneCommand * (avgHomeDirectoryLength + numPossibleAdditionalCharacters)];
    65	int currentNumOfTildas = 0;
    66	char * tildaArrayPtrs[numTimesATildaCanBeSeenInOneCommand];
    67	char homeEnvVar[STORAGE];
    68	int lengthOfHomeEnvVar = 0;
    69	
    70	int main() {
    71	    /* Declaration of Locals */
    72	    int returnValueParse;
    73	    int inputFileFileDescriptorNum;
    74	    int outputFileFileDescriptorNum;
    75	    char cwd[256];
    76	    char * endOfCwd;
    77	
    78	    /* Setting up the process group for this process */
    79	    if( (setpgid(getpid(),0)) != 0){ // setpgid and if there is an error jump in else continue on to next statement
    80	        fprintf(stderr, "Cant't change process group\n");
    81	    }
    82	    /* Handling the SIGTERM signal appropriately */
    83	    (void) signal(SIGTERM, signalCatcher); // dont kill main, & kill all other processes
    84	
    85	    for (;;) {
    86	        /* Print Prompt to user */
    87	        printf(":570: ");
    88	        /* Call the Parse function to read the stdin and setup appropriate data structures and set flags*/
    89	        returnValueParse = parse();
    90	
    91	        /* Handling of errors discovered in the function Parse()*/
    92	        if (SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED == 1) {
    93	            fprintf(stderr, "No input redirection file specified\n");
    94	            continue;
    95	        }
    96	        if (SYNTAX_ERROR_NO_Output_FILE_SPECIFIED == 1) {
    97	            fprintf(stderr, "No output redirection file specified\n");
    98	            continue;
    99	        }
   100	        if (SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS == 1) {
   101	            fprintf(stderr, "You can't have multiple input redirections.\n");
   102	            continue;
   103	        }
   104	        if (SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS == 1) {
   105	            fprintf(stderr, "You can't have multiple output redirections.\n");
   106	            continue;
   107	        }
   108	        if (SYNTAX_ERROR_MULTIPLE_PIPELINES == 1) {
   109	            fprintf(stderr, "You can't have multiple pipelines.\n");
   110	            continue;
   111	        }
   112	        if (SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META == 1) {
   113	            fprintf(stderr, "You can't have a meta character as a filename for input redirection.\n");
   114	            continue;
   115	        }
   116	        if (SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META == 1) {
   117	            fprintf(stderr, "You can't have a meta character as a filename for output redirection.\n");
   118	            continue;
   119	        }
   120	        if(SYNTAX_ERROR_HEREIS_IDENTIFIER == 1){
   121	            fprintf(stderr, "Error with parsing \"hereis\" identifier.\n");
   122	            continue;
   123	        }
   124	        if(SYNTAX_ERROR_ENVVAR == 1){
   125	            fprintf(stderr, "%s: Undefined variable.\n", envVarNameError);
   126	            continue;
   127	        }
   128	        if(SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND == 1){
   129	            fprintf(stderr, "Undefined username for ~ functionality: %s\n", userName);
   130	            continue;
   131	        }
   132	
   133	
   134	        /* Handling if we read in the EOF character as the first character in stdin*/
   135	        if (returnValueParse == -255 && wordCount == 0) {
   136	            break;
   137	        }
   138	            /* If line is empty reissue prompt */
   139	        else if (wordCount == 0 && returnValueParse == 0) {
   140	            continue;
   141	        }
   142	
   143	        /* If */ //todo
   144	        if(newArgV[0] == '\0'){ // todo change to NULLTERM
   145	            fprintf(stderr, "No executable stated\n");
   146	        }
   147	
   148	        /* Handling cd */
   149	        if ((strcmp(newArgV[0], "cd")) == 0) {
   150	            if (wordCount > 2) {
   151	                fprintf(stderr, "Too many arguments for the command 'cd'\n");
   152	                continue;
   153	            }
   154	            else if (wordCount == 1) {
   155	                char * envVar;
   156	                envVar = getenv("HOME");
   157	                if ((chdir(envVar)) == -1) { // If we can't change directory
   158	                    fprintf(stderr, "Couldn't change to home directory.\n");
   159	                    continue;
   160	                }
   161	                else {  // If changing directory was successful
   162	                    continue;
   163	                }
   164	            }
   165	            else if (wordCount == 2){ // If a directory is explicitly stated
   166	                if ((chdir(newArgV[1])) == -1) {// If we can't change to specified directory
   167	                    fprintf(stderr, "Couldn't change to specified directory.\n");
   168	                    continue;
   169	                }
   170	                else { // If changing directory was successful
   171	                    getcwd(cwd, sizeof(cwd));
   172	                    endOfCwd = basename(cwd);
   173	                    printf("%s", endOfCwd);
   174	                    continue;
   175	                }
   176	            }
   177	        }
   178	
   179	        /* If a pipeline has been read in through the parse() function using the getword() function we will
   180	                branch off to another function to handle pipelines appropriately*/
   181	        if(PIPELINE_FLAG == 1){
   182	            if (newArgV[processOffsets[1]] == NULL){
   183	                fprintf(stderr, "Pipeline with no second command specified results in an error.\n");
   184	                continue;
   185	            }
   186	            pipeline();
   187	            continue; // once pipeline returns continue to reissue prompt and process next command
   188	        }
   189	
   190	        // Before forking Flush the data streams
   191	        fflush(stdout);
   192	        fflush(stderr);
   193	        // Fork to execute a child process - to handle rightmost process
   194	        // CHK((kidpid = fork())) todo check if we need to implement this for forking
   195	        if ((kidpid = fork()) == -1){ // fork and if there is an error jump into if statement otherwise continue on to next statement
   196	            fprintf(stderr,"Fork Failed");
   197	            continue;
   198	        }
   199	        else if (kidpid == 0){ // If child process forked successfully
   200	            /* Below is Handling a single command */
   201	            // Handling of output redirection
   202	            if(OUTPUT_REDIRECTION_FLAG == 1 && outputRedirection != NULL){
   203	                // don't access first
   204	                outputFileFileDescriptorNum = open(outputRedirection, O_WRONLY | O_CREAT | O_EXCL , 0600); //todo O_TRUNC | S_IRUSR| S_IRGRP | S_IRGRP | S_IWUSR possibly
   205	                if (outputFileFileDescriptorNum == -1) {
   206	                    fprintf(stderr, "%s: File exists.\n", outputRedirection);
   207	                    exit(ERROR7);
   208	                }
   209	                CHK(dup2(outputFileFileDescriptorNum, STDOUT_FILENO));
   210	                close(outputFileFileDescriptorNum);
   211	            }
   212	
   213	//            if(HEREIS_FLAG ==1 ){
   214	//                open dup2
   215	//            }
   216	
   217	            // Handling of input redirection
   218	            if (INPUT_REDIRECTION_FLAG == 1 && inputRedirection != NULL) {
   219	                // does the file exist and do I have the correct permissions, using the calling processe's real UID & GID
   220	                int accessInputRedirectionfile = access(inputRedirection, R_OK);
   221	                if (accessInputRedirectionfile < 0) {
   222	                    if (errno == EACCES) {
   223	                        fprintf(stderr, "Input redirection file permissions denied\n");
   224	                        exit(ERROR8);
   225	                    }
   226	                    else if (errno == ENOENT && errno == ENOTDIR) {
   227	                        fprintf(stderr, "Input redirection path doesn't exist\n");
   228	                        exit(ERROR8);
   229	                    }
   230	                }
   231	                else if (accessInputRedirectionfile == 0){
   232	                    inputFileFileDescriptorNum = open(inputRedirection, O_RDONLY);
   233	                    if (inputFileFileDescriptorNum == -1) {
   234	                        fprintf(stderr, "Couldn't open the input redirection file\n");
   235	                        exit(ERROR9);
   236	                    }
   237	                }
   238	                CHK(dup2(inputFileFileDescriptorNum, STDIN_FILENO));
   239	                close(inputFileFileDescriptorNum);
   240	            }
   241	
   242	            if (INPUT_REDIRECTION_FLAG == 0 && WAIT_FLAG == 1){
   243	                int devNullFileDescriptorNum = open("/dev/null",O_RDONLY);
   244	                if (devNullFileDescriptorNum == -1){
   245	                    fprintf(stderr, "Couldn't open /dev/null");
   246	                    exit(ERROR10);
   247	                }
   248	                CHK(dup2(devNullFileDescriptorNum, STDIN_FILENO));
   249	                close(devNullFileDescriptorNum);
   250	            }
   251	
   252	            /* After all redirections are satisfied execute the process */
   253	            if ((execvp(newArgV[0],newArgV)) == -1){
   254	                fprintf(stderr, "Couldn't execute command\n");
   255	                exit(ERROR3);
   256	            }
   257	
   258	        } // End of child process scope
   259	        //else if(kidpid != 0){} we are in parent process scope
   260	        // Parent process scope
   261	        if(WAIT_FLAG == 1){
   262	            printf("%s [%d]\n", *newArgV, kidpid);
   263	            continue;
   264	        }
   265	        else{
   266	            for(;;){
   267	                pid_t find_pid;
   268	                find_pid = wait(NULL);
   269	                if(find_pid == kidpid){
   270	                    break;
   271	                }
   272	            }
   273	
   274	        }
   275	    }
   276	    killpg(getpgrp(), SIGTERM);
   277	    printf("p2 terminated.\n");
   278	    exit(ERROR4);
   279	}
   280	
   281	
   282	void signalCatcher(int signalNum){
   283	    // stays empty: rather than dying, do nothing
   284	}
   285	
   286	void pipeline() {
   287	    /*
   288	     * Vertical piping:
   289	     * p2 creates one child, then that child creates a pipe, and then forks its own child.
   290	     * p2's child and grandchild then handle the two halves of the pipe command.
   291	     * The order of the fork() and pipe() system calls is important: both children have to know about the pipe since they use it to communicate
   292	     */
   293	
   294	    // Note we are still in child process scope // kidpid; command 2;
   295	
   296	    /* Declaration of locals */
   297	    int accessInputRedirectionfile;
   298	    int inputFileFileDescriptorNum;
   299	    int outputFileFileDescriptorNum;
   300	    int greatestOfAllChildren;
   301	    int rightMostPid;
   302	    int middlePid; //  greatGrandChildPid;
   303	    int greatGreatGrandChildPid;
   304	    int kidpid;
   305	    int outP2Pid;
   306	    int i;
   307	    int maxPipes = numExecsCounter;
   308	    int loopCount;
   309	    int pipeCount = 0;
   310	
   311	//    To Get Out Of p2
   312	//    /* Before forking, flush out stderr and stdout */
   313	    fflush(stdout);
   314	    fflush(stderr);
   315	    CHK((outP2Pid = fork())); // to get out of p2
   316	    if (outP2Pid == 0) { //This is not the right most process // !(child, right most process)
   317	        /*Create All pipes*/
   318	        for(i = 0; i < 10; i++){
   319	            CHK(pipe(fileDescriptor + (i*2)));
   320	        }
   321	        fflush(stdout);
   322	        fflush(stderr);
   323	        CHK((rightMostPid = fork())); // right most process
   324	        if (rightMostPid == 0){
   325	            for (loopCount = maxPipes-1, pipeCount = 1; loopCount >= 0; loopCount--, pipeCount++){
   326	                fflush(stdout);
   327	                fflush(stderr);
   328	                if(loopCount == 0){
   329	                    /* Handling of input redirection */
   330	                    if (INPUT_REDIRECTION_FLAG == 1 && inputRedirection != NULL) {
   331	                        // does the file exist and do I have the correct permissions, using the calling processe's real UID & GID
   332	                        CHK((accessInputRedirectionfile = access(inputRedirection, R_OK)));
   333	                        if(accessInputRedirectionfile == -1){
   334	                            //return; // todo get checked what do i do here?
   335	                            exit(ERROR8);
   336	                        }
   337	                        if (accessInputRedirectionfile == 0) {
   338	                            inputFileFileDescriptorNum = open(inputRedirection, O_RDONLY);
   339	                            if (inputFileFileDescriptorNum == -1) {
   340	                                fprintf(stderr, "Couldn't open the input redirection file\n");
   341	                                exit(ERROR9);
   342	                                //return; // todo Not in forloop, return a number to signify to continue, check if return is the right thing to do
   343	                            }
   344	                        }
   345	                        CHK(dup2(inputFileFileDescriptorNum, STDIN_FILENO));
   346	                        close(inputFileFileDescriptorNum);
   347	                    }
   348	
   349	                    /* If WAITFLAG == 1 and INPUT_REDIRECTION_FLAG == 0 then redirect stdin to /dev/null */
   350	                    if (INPUT_REDIRECTION_FLAG == 0 && WAIT_FLAG == 1){
   351	                        int devNullFileDescriptorNum = open("/dev/null",O_RDONLY);
   352	                        if (devNullFileDescriptorNum == -1){
   353	                            fprintf(stderr, "Couldn't open /dev/null");
   354	                            exit(ERROR10);
   355	                            return;//continue;  // todo Not in forloop, return a number to signify to continue
   356	                        }
   357	                        CHK(dup2(devNullFileDescriptorNum, STDIN_FILENO));
   358	                        close(devNullFileDescriptorNum);
   359	                    }
   360	
   361	                    /* Setup redirection using our pipe created in the child process, we want stdout to go to write end of pipe */
   362	                    CHK(dup2(fileDescriptor[pipeCount*2-1], STDOUT_FILENO));
   363	
   364	                    /* Close the read end of the pipe so that the pipe doesn't wait for more information to be transmitted */
   365	                    for(i = 0; i < 20; i++){
   366	                        close(fileDescriptor[i]);
   367	                    }
   368	                    /* Execute Command 1 */
   369	                    if ((execvp(newArgV[processOffsets[loopCount]], newArgV + processOffsets[loopCount])) == -1) {
   370	                        fprintf(stderr, "Couldn't execute command\n");
   371	                        exit(ERROR3); //todo create a new Error#
   372	                    }
   373	                }
   374	                CHK((middlePid = fork()));
   375	                if(middlePid == 0){
   376	
   377	                }
   378	                else if(middlePid != 0){  // second to last process
   379	                    dup2(fileDescriptor[pipeCount*2-1],STDOUT_FILENO);
   380	                    dup2(fileDescriptor[pipeCount*2],STDIN_FILENO);
   381	                    /* Close the read end of the pipe so that the pipe doesn't wait for more information to be transmitted */
   382	                    for(i = 0; i < 20; i++){
   383	                        close(fileDescriptor[i]);
   384	                    }
   385	                    CHK(execvp(newArgV[processOffsets[loopCount]],newArgV + processOffsets[loopCount]));
   386	                }
   387	            }
   388	            //outside of loop
   389	        }
   390	        else if(rightMostPid != 0){ // right most process
   391	            // finish pre-processing of right most process
   392	            /* Handling of output redirection */
   393	            if (OUTPUT_REDIRECTION_FLAG == 1 && outputRedirection != NULL) {
   394	                // don't access first
   395	                outputFileFileDescriptorNum = open(outputRedirection, O_WRONLY | O_CREAT | O_EXCL, 0600); //todo O_TRUNC | S_IRUSR| S_IRGRP | S_IRGRP | S_IWUSR possibly
   396	                if (outputFileFileDescriptorNum == -1) {
   397	                    fprintf(stderr, "%s: File exists.\n", outputRedirection);
   398	                    exit(ERROR7);
   399	                    //return; //continue;  // todo Not in forloop, return a number to signify to continue
   400	                }
   401	                CHK(dup2(outputFileFileDescriptorNum, STDOUT_FILENO));
   402	                close(outputFileFileDescriptorNum);
   403	            }
   404	
   405	            /* setup pipe redirections */
   406	            CHK(dup2(fileDescriptor[0], STDIN_FILENO));
   407	            for(i = 0; i < 20; i++){
   408	                close(fileDescriptor[i]);
   409	            }
   410	
   411	            /* Execute the right most command */
   412	            if ((execvp(newArgV[processOffsets[numExecsCounter]], newArgV + processOffsets[numExecsCounter])) == -1) { // todo change newArgV[processOffsets[numExecs]]
   413	                fprintf(stderr, "Couldn't execute command\n");
   414	                exit(ERROR5);
   415	            } // End of child Process
   416	        }
   417	    }
   418	    else if (outP2Pid != 0){
   419	        //does wait
   420	        if (WAIT_FLAG == 1) {
   421	            printf("%s [%d]\n", *newArgV, outP2Pid); // printf("%s [%d]\n", *newArgV, kidpid);
   422	            return; //continue;
   423	        }
   424	        else {
   425	            for (;;) {
   426	                pid_t find_pid;
   427	                find_pid = wait(NULL);
   428	                if (find_pid == outP2Pid) { // if (find_pid == outP2pid) {  todo check
   429	                    //break;
   430	                    return;
   431	                }
   432	            }
   433	        }
   434	    }
   435	}
   436	
   437	int parse(){
   438	    /* Declaration of Locals */
   439	    char * tempPtr;  // used for handling if we see the '&' character and the next character isn't a ';' 'NEWLINE' or '$ '
   440	    int getWordReturn;
   441	    int i;
   442	    char * currentTildaNewCommandArgPtr = tildaArray;
   443	    char * envVar; // declare a variable that will point to the system call getenv() function
   444	    lengthOfHomeEnvVar = 0;
   445	    /* get the current HOME enviroment variable path and load it into homeEnvVar a global variable */
   446	    for (i=0, envVar = getenv("HOME"); *envVar != '\0';  envVar++, i++ ){
   447	        // start the for loop with the first character from the returned character array from the getenv() system call, and terminate the for loop when we reach the null terminator in the character array
   448	        homeEnvVar[i] = *envVar;
   449	        lengthOfHomeEnvVar++;
   450	    }
   451	    homeEnvVar[i] = NULLTERM;
   452	    lengthOfHomeEnvVar++;
   453	
   454	    /* Reset all global declarations to support invoking parse() multiple times */
   455	    INPUT_REDIRECTION_FLAG = OUTPUT_REDIRECTION_FLAG = PIPELINE_FLAG = WAIT_FLAG = BACKSLASH_FLAG = wordCount = getWordReturn = HEREIS_FLAG = 0;
   456	    SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = SYNTAX_ERROR_MULTIPLE_SEQUENCE_OF_METACHARACTERS = SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = SYNTAX_ERROR_HEREIS_IDENTIFIER = SYNTAX_ERROR_ENVVAR = SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 0;
   457	    inputRedirection = outputRedirection = tempPtr = hereIsKeyword = envVarNameError = NULL;
   458	    bigBufferPtr = bigBuffer;
   459	    processOffsets[0] = 0; // the first executable name is located at newArgV[0]
   460	    numExecsCounter = 0;
   461	    currentNumOfTildas = 0;
   462	
   463	    for(i = 0; i < 10; i++){ // todo check if this is redundant?
   464	        processOffsets[i] = 0;
   465	    }
   466	    for ( i = 0; i < MAXITEM * STORAGE ; i++){
   467	        bigBuffer[i] = NULLTERM;
   468	    }
   469	    for(i = 0; i < MAXITEM; i++){
   470	        newArgV[i] = NULL;
   471	    }
   472	
   473	    /* Parse stdin using the function getword() and set appropriate flags and load appropriate data structures */
   474	    // Note flags set will be handled in p2.c main()
   475	    while(((getWordReturn = getword(bigBufferPtr))) != -255) { // Note that if stdin is empty it returns EOF, and in getword() if (charCount == 0 and iochar == EOF) getword() returns -255 to end the parse function by returning -255
   476	
   477	        /* Handle substituting '~' for the correct path */
   478	        if(strncmp(bigBufferPtr,"~",1) == 0 ){
   479	            if (getWordReturn == 1){
   480	                newArgV[wordCount++] = homeEnvVar;
   481	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   482	                continue;
   483	            }
   484	            else{
   485	                if(strncmp(bigBufferPtr,"~/",2) == 0 ){
   486	                    char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
   487	                    for (i = 0; i < lengthOfHomeEnvVar-1; i++){
   488	                        *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
   489	                    }
   490	                    for (i = 1; i < getWordReturn; i++){
   491	                        *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
   492	                    }
   493	                    *(currentTildaNewCommandArgPtr++) = NULLTERM;
   494	                    tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
   495	                    newArgV[wordCount++] = tildaArrayPtrs[currentNumOfTildas++];
   496	                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   497	                    continue;
   498	                }
   499	                /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
   500	                else{
   501	                    char * startOfUserName;
   502	                    char * userNamePtr;
   503	                    char * startAdditionalCharacterPtr;
   504	                    char * additionalCharactersPtr;
   505	                    //note: (char userName[STORAGE]) was declared as a global variable for error handling
   506	                    char additionalPath[STORAGE];
   507	                    int additionalFlag = 0;
   508	                    startOfUserName = bigBufferPtr + 1;
   509	                    userNamePtr = startOfUserName;
   510	                    i = 0;
   511	
   512	                    /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
   513	                    while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
   514	                        userName[i++] = *userNamePtr++;
   515	                    }
   516	                    userName[i] = NULLTERM; // null terminate the username string
   517	
   518	                    /* Handle collecting possible additional characters to eventually append to end of absolute path */
   519	                    startAdditionalCharacterPtr = userNamePtr;
   520	                    additionalCharactersPtr = startAdditionalCharacterPtr;
   521	                    i = 0;
   522	                    /* create and load an array to gather additional path to eventually append to absolute path */
   523	                    while(*additionalCharactersPtr != NULLTERM){
   524	                        additionalFlag = 1;
   525	                        additionalPath[i++] = *additionalCharactersPtr++;
   526	                    }
   527	                    if (additionalFlag == 1){
   528	                        additionalPath[i] = NULLTERM;
   529	                    }
   530	
   531	                    /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
   532	                    FILE * passwdFile = fopen("/etc/passwd", "r");
   533	
   534	                    /* Setup for parsing through the user entries in the file */
   535	                    /* Declarations for using the getline function */
   536	                    char * line = NULL;
   537	                    size_t bufferSize = 0;
   538	                    int lineFoundFlag = 0;
   539	                    int eofFound = 0;
   540	                    size_t lineSize;
   541	                    /* Declarations for using the strtok function */
   542	                    char * delimiter = ":";
   543	                    char * token; // used to tokenize the correct user entry
   544	
   545	                    char * absolutePathToHomeDirectory;
   546	
   547	                    /* Parse the file */
   548	                    while(lineFoundFlag != 1){
   549	                        lineSize = getline(&line, &bufferSize, passwdFile);
   550	                        token = strtok(line, delimiter);
   551	
   552	                        /* if we have found the end of the file or there was an error reading a line from the file */
   553	                        if (lineSize == -1){
   554	                            eofFound = 1;
   555	                            break;
   556	                        }
   557	
   558	                        /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
   559	                        if (strcmp(token,userName) == 0){
   560	                            lineFoundFlag = 1;
   561	
   562	                            /*the correct absolute path name to the home directory is the fifth element in the user entry*/
   563	                            for (i = 0; i < 5; i++){
   564	                                line = NULL; // do this to get the next token, read strtok docs for an explanation
   565	                                token = strtok(line, delimiter);
   566	                            }
   567	                            absolutePathToHomeDirectory = token; // save the pointer
   568	                        }
   569	                        /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
   570	                        if(strcmp(token,userName) != 0){
   571	                            line = line + lineSize;
   572	                        }
   573	                    }
   574	                    /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
   575	                    if(eofFound == 1){
   576	                        SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
   577	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   578	                        continue;
   579	                    }
   580	
   581	                    /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
   582	                    /* Declarations needed to form the new command argument */
   583	                    // note: (char newCommandArg[512]) was declared as a global variable
   584	                    char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
   585	                    char * tmpPtr = &*absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
   586	                    int lengthOfAbsolutePathToHomeDirectory = 0;
   587	                    int numOfAdditionalCharacters = 0;
   588	                    /*copy absolute path of home directory of a particular user */
   589	                    i = 0;
   590	                    while ((strncmp(tmpPtr,"\0",1)) != 0){
   591	                        *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
   592	                        lengthOfAbsolutePathToHomeDirectory++;
   593	                    }
   594	
   595	                    /* possibly append the additional characters of new command argument */
   596	                    if (additionalFlag == 1){
   597	                        i = 0;
   598	                        while(additionalPath[i] != NULLTERM){
   599	                            *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
   600	                            numOfAdditionalCharacters++;
   601	                        }
   602	                        *((currentTildaNewCommandArgPtr++)) = NULLTERM;
   603	                    }
   604	                    if(additionalFlag == 0){
   605	                        *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
   606	                    }
   607	                    int lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
   608	                    tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
   609	                    newArgV[wordCount++] = tildaArrayPtrs[currentNumOfTildas++];
   610	                    // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
   611	                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   612	                    continue;
   613	                }
   614	            }
   615	        }
   616	        /* Handling if the getword() function parsed either a SEMICOLON or a NEWLINE, or a DOLLARSIGN followed by a space or newline */
   617	        if(getWordReturn == 0 && BACKSLASH_FLAG == 0 ){
   618	            if(wordCount > 0){
   619	                if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
   620	                    if (strcmp(newArgV[wordCount - 1], "&") == 0) {
   621	                        newArgV[--wordCount] = NULL;
   622	                        WAIT_FLAG = 1;
   623	                        return 0;
   624	                    }
   625	                }
   626	            }
   627	            newArgV[wordCount] = NULL;
   628	            return 0;
   629	        }
   630	        // Handling if the getword() function parsed a dollar sign at the beginning of a word, we need to strip the minus sign from the getwordReturn for prog2
   631	        if (getWordReturn < 0 ){
   632	            getWordReturn = abs(getWordReturn);
   633	            newArgV[wordCount] = getenv(bigBufferPtr);
   634	            if(newArgV[wordCount] == NULL){
   635	                SYNTAX_ERROR_ENVVAR = 1;
   636	                envVarNameError = bigBufferPtr;
   637	                newArgV[wordCount] = NULL;
   638	            }
   639	            wordCount++;
   640	            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
   641	            continue;
   642	        }
   643	
   644	        /* Handle Seeing flags */
   645	        // Handling of input redirection
   646	        if((strcmp(bigBufferPtr,"<")) == 0 && INPUT_REDIRECTION_FLAG == 0 && BACKSLASH_FLAG == 0){
   647	            INPUT_REDIRECTION_FLAG = 1;
   648	            // before calling getword() again we need to update the location of where bigBufferPtr is pointing to
   649	            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
   650	            // call getword again to handle the next "word in stdin" appropriately
   651	            getWordReturn = getword(bigBufferPtr);
   652	
   653	            /* Handle substituting '~' for the correct path */
   654	            if(strncmp(bigBufferPtr,"~",1) == 0 ){
   655	                if (getWordReturn == 1){
   656	                    inputRedirection = homeEnvVar;
   657	                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   658	                    continue;
   659	                }
   660	                else{
   661	                    if(strncmp(bigBufferPtr,"~/",2) == 0 ){
   662	                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
   663	                        for (i = 0; i < lengthOfHomeEnvVar-1; i++){
   664	                            *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
   665	                        }
   666	                        for (i = 1; i < getWordReturn; i++){
   667	                            *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
   668	                        }
   669	                        *(currentTildaNewCommandArgPtr++) = NULLTERM;
   670	                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
   671	                        inputRedirection = tildaArrayPtrs[currentNumOfTildas++];
   672	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   673	                        continue;
   674	                    }
   675	                        /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
   676	                    else{
   677	                        char * startOfUserName;
   678	                        char * userNamePtr;
   679	                        char * startAdditionalCharacterPtr;
   680	                        char * additionalCharactersPtr;
   681	                        //note: (char userName[STORAGE]) was declared as a global variable for error handling
   682	                        char additionalPath[STORAGE];
   683	                        int additionalFlag = 0;
   684	                        startOfUserName = bigBufferPtr + 1;
   685	                        userNamePtr = startOfUserName;
   686	                        i = 0;
   687	
   688	                        /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
   689	                        while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
   690	                            userName[i++] = *userNamePtr++;
   691	                        }
   692	                        userName[i] = NULLTERM; // null terminate the username string
   693	
   694	                        /* Handle collecting possible additional characters to eventually append to end of absolute path */
   695	                        startAdditionalCharacterPtr = userNamePtr;
   696	                        additionalCharactersPtr = startAdditionalCharacterPtr;
   697	                        i = 0;
   698	                        /* create and load an array to gather additional path to eventually append to absolute path */
   699	                        while(*additionalCharactersPtr != NULLTERM){
   700	                            additionalFlag = 1;
   701	                            additionalPath[i++] = *additionalCharactersPtr++;
   702	                        }
   703	                        if (additionalFlag == 1){
   704	                            additionalPath[i] = NULLTERM;
   705	                        }
   706	
   707	                        /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
   708	                        FILE * passwdFile = fopen("/etc/passwd", "r");
   709	
   710	                        /* Setup for parsing through the user entries in the file */
   711	                        /* Declarations for using the getline function */
   712	                        char * line = NULL;
   713	                        size_t bufferSize = 0;
   714	                        int lineFoundFlag = 0;
   715	                        int eofFound = 0;
   716	                        size_t lineSize;
   717	                        /* Declarations for using the strtok function */
   718	                        char * delimiter = ":";
   719	                        char * token; // used to tokenize the correct user entry
   720	
   721	                        char * absolutePathToHomeDirectory;
   722	
   723	                        /* Parse the file */
   724	                        while(lineFoundFlag != 1){
   725	                            lineSize = getline(&line, &bufferSize, passwdFile);
   726	                            token = strtok(line, delimiter);
   727	
   728	                            /* if we have found the end of the file or there was an error reading a line from the file */
   729	                            if (lineSize == -1){
   730	                                eofFound = 1;
   731	                                break;
   732	                            }
   733	
   734	                            /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
   735	                            if (strcmp(token,userName) == 0){
   736	                                lineFoundFlag = 1;
   737	
   738	                                /*the correct absolute path name to the home directory is the fifth element in the user entry*/
   739	                                for (i = 0; i < 5; i++){
   740	                                    line = NULL; // do this to get the next token, read strtok docs for an explanation
   741	                                    token = strtok(line, delimiter);
   742	                                }
   743	                                absolutePathToHomeDirectory = token; // save the pointer
   744	                            }
   745	                            /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
   746	                            if(strcmp(token,userName) != 0){
   747	                                line = line + lineSize;
   748	                            }
   749	                        }
   750	                        /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
   751	                        if(eofFound == 1){
   752	                            SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
   753	                            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   754	                            continue;
   755	                        }
   756	
   757	                        /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
   758	                        /* Declarations needed to form the new command argument */
   759	                        // note: (char newCommandArg[512]) was declared as a global variable
   760	                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
   761	                        char * tmpPtr = &*absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
   762	                        int lengthOfAbsolutePathToHomeDirectory = 0;
   763	                        int numOfAdditionalCharacters = 0;
   764	                        /*copy absolute path of home directory of a particular user */
   765	                        i = 0;
   766	                        while ((strncmp(tmpPtr,"\0",1)) != 0){
   767	                            *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
   768	                            lengthOfAbsolutePathToHomeDirectory++;
   769	                        }
   770	
   771	                        /* possibly append the additional characters of new command argument */
   772	                        if (additionalFlag == 1){
   773	                            i = 0;
   774	                            while(additionalPath[i] != NULLTERM){
   775	                                *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
   776	                                numOfAdditionalCharacters++;
   777	                            }
   778	                            *((currentTildaNewCommandArgPtr++)) = NULLTERM;
   779	                        }
   780	                        if(additionalFlag == 0){
   781	                            *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
   782	                        }
   783	                        int lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
   784	                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
   785	                        inputRedirection = tildaArrayPtrs[currentNumOfTildas++];
   786	                        // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
   787	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   788	                        continue;
   789	                    }
   790	                }
   791	            }
   792	
   793	            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
   794	                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
   795	                if(wordCount > 0){
   796	                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
   797	                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
   798	                            newArgV[--wordCount] = NULL;
   799	                            WAIT_FLAG = 1;
   800	                            return 0;
   801	                        }
   802	                    }
   803	                }
   804	                newArgV[wordCount] = NULL;
   805	                return 0;
   806	            }
   807	            // Note: if(getWordReturn == 0 && BACKSLASH_FLAG == 1) would never trjgger
   808	            // Note: (strcmp(bigBufferPtr,"\n") == 0  && getWordReturn == 0 && BACKSLASH_FLAG == 1) handled in getword.c
   809	            if(getWordReturn == -255){
   810	                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
   811	                break;
   812	            }
   813	            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
   814	                getWordReturn = abs(getWordReturn);
   815	                inputRedirection = getenv(bigBufferPtr);
   816	                if(inputRedirection == NULL){
   817	                    SYNTAX_ERROR_ENVVAR = 1;
   818	                    envVarNameError = bigBufferPtr;
   819	                }
   820	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
   821	                continue;
   822	            }
   823	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0){
   824	                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
   825	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   826	                continue;
   827	            }
   828	            /* Handling a character that is a metacharacter*/
   829	            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
   830	                SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = 1;
   831	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   832	                continue;
   833	            }
   834	            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
   835	            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
   836	                BACKSLASH_FLAG = 0;
   837	                inputRedirection = bigBufferPtr;
   838	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
   839	                continue;
   840	            }
   841	
   842	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
   843	                inputRedirection = bigBufferPtr;
   844	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
   845	                continue;
   846	            }
   847	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
   848	                BACKSLASH_FLAG = 0;
   849	                inputRedirection = bigBufferPtr;
   850	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
   851	                continue;
   852	            }
   853	        }
   854	
   855	        // Handling of more than one input redirection
   856	        if((strcmp(bigBufferPtr,"<")) == 0 && INPUT_REDIRECTION_FLAG == 1 && BACKSLASH_FLAG == 0){
   857	            SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
   858	            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   859	            getWordReturn = getword(bigBufferPtr);
   860	            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
   861	                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
   862	                if(wordCount > 0){
   863	                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
   864	                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
   865	                            newArgV[--wordCount] = NULL;
   866	                            WAIT_FLAG = 1;
   867	                            return 0;
   868	                        }
   869	                    }
   870	                }
   871	                newArgV[wordCount] = NULL;
   872	                return 0;
   873	            }
   874	            // Note: if(getWordReturn == 0 && BACKSLASH_FLAG == 1) would never trjgger
   875	            // Note: (strcmp(bigBufferPtr,"\n") == 0  && getWordReturn == 0 && BACKSLASH_FLAG == 1) handled in getword.c
   876	            if(getWordReturn == -255){
   877	                SYNTAX_ERROR_NO_INPUT_FILE_SPECIFIED = 1;
   878	                break;
   879	            }
   880	            if(getWordReturn < 0){ // handling if getword returns a negative return value, note EOF is taken care of already
   881	                getWordReturn = abs(getWordReturn);
   882	                inputRedirection = getenv(bigBufferPtr);
   883	                if(inputRedirection == NULL){
   884	                    SYNTAX_ERROR_ENVVAR = 1;
   885	                    envVarNameError = bigBufferPtr;
   886	                }
   887	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
   888	                continue;
   889	            }
   890	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0){
   891	                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
   892	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   893	                continue;
   894	            }
   895	            /* Handling a character that is a metacharacter*/
   896	            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
   897	                SYNTAX_ERROR_INPUT_FILE_SPECIFIED_IS_META = 1;
   898	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   899	                continue;
   900	            }
   901	            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
   902	            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
   903	                BACKSLASH_FLAG = 0;
   904	                inputRedirection = bigBufferPtr;
   905	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
   906	                continue;
   907	            }
   908	
   909	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
   910	                inputRedirection = bigBufferPtr;
   911	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
   912	                continue;
   913	            }
   914	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
   915	                BACKSLASH_FLAG = 0;
   916	                inputRedirection = bigBufferPtr;
   917	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
   918	                continue;
   919	            }
   920	        }
   921	        // Handling of output redirection
   922	        if((strcmp(bigBufferPtr,">")) == 0  && OUTPUT_REDIRECTION_FLAG == 0 && BACKSLASH_FLAG == 0){
   923	            OUTPUT_REDIRECTION_FLAG = 1; // set this flag so main() can handle output redirection
   924	            // before calling getword() again we need to update the location of where bigBufferPtr is pointing to
   925	            bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
   926	            getWordReturn = getword(bigBufferPtr);
   927	
   928	            /* Handle substituting '~' for the correct path */
   929	            if(strncmp(bigBufferPtr,"~",1) == 0 ){
   930	                if (getWordReturn == 1){
   931	                    outputRedirection = homeEnvVar;
   932	                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   933	                    continue;
   934	                }
   935	                else{
   936	                    if(strncmp(bigBufferPtr,"~/",2) == 0 ){
   937	                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
   938	                        for (i = 0; i < lengthOfHomeEnvVar-1; i++){
   939	                            *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
   940	                        }
   941	                        for (i = 1; i < getWordReturn; i++){
   942	                            *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
   943	                        }
   944	                        *(currentTildaNewCommandArgPtr++) = NULLTERM;
   945	                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
   946	                        outputRedirection = tildaArrayPtrs[currentNumOfTildas++];
   947	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
   948	                        continue;
   949	                    }
   950	                        /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
   951	                    else{
   952	                        char * startOfUserName;
   953	                        char * userNamePtr;
   954	                        char * startAdditionalCharacterPtr;
   955	                        char * additionalCharactersPtr;
   956	                        //note: (char userName[STORAGE]) was declared as a global variable for error handling
   957	                        char additionalPath[STORAGE];
   958	                        int additionalFlag = 0;
   959	                        startOfUserName = bigBufferPtr + 1;
   960	                        userNamePtr = startOfUserName;
   961	                        i = 0;
   962	
   963	                        /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
   964	                        while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
   965	                            userName[i++] = *userNamePtr++;
   966	                        }
   967	                        userName[i] = NULLTERM; // null terminate the username string
   968	
   969	                        /* Handle collecting possible additional characters to eventually append to end of absolute path */
   970	                        startAdditionalCharacterPtr = userNamePtr;
   971	                        additionalCharactersPtr = startAdditionalCharacterPtr;
   972	                        i = 0;
   973	                        /* create and load an array to gather additional path to eventually append to absolute path */
   974	                        while(*additionalCharactersPtr != NULLTERM){
   975	                            additionalFlag = 1;
   976	                            additionalPath[i++] = *additionalCharactersPtr++;
   977	                        }
   978	                        if (additionalFlag == 1){
   979	                            additionalPath[i] = NULLTERM;
   980	                        }
   981	
   982	                        /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
   983	                        FILE * passwdFile = fopen("/etc/passwd", "r");
   984	
   985	                        /* Setup for parsing through the user entries in the file */
   986	                        /* Declarations for using the getline function */
   987	                        char * line = NULL;
   988	                        size_t bufferSize = 0;
   989	                        int lineFoundFlag = 0;
   990	                        int eofFound = 0;
   991	                        size_t lineSize;
   992	                        /* Declarations for using the strtok function */
   993	                        char * delimiter = ":";
   994	                        char * token; // used to tokenize the correct user entry
   995	
   996	                        char * absolutePathToHomeDirectory;
   997	
   998	                        /* Parse the file */
   999	                        while(lineFoundFlag != 1){
  1000	                            lineSize = getline(&line, &bufferSize, passwdFile);
  1001	                            token = strtok(line, delimiter);
  1002	
  1003	                            /* if we have found the end of the file or there was an error reading a line from the file */
  1004	                            if (lineSize == -1){
  1005	                                eofFound = 1;
  1006	                                break;
  1007	                            }
  1008	
  1009	                            /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
  1010	                            if (strcmp(token,userName) == 0){
  1011	                                lineFoundFlag = 1;
  1012	
  1013	                                /*the correct absolute path name to the home directory is the fifth element in the user entry*/
  1014	                                for (i = 0; i < 5; i++){
  1015	                                    line = NULL; // do this to get the next token, read strtok docs for an explanation
  1016	                                    token = strtok(line, delimiter);
  1017	                                }
  1018	                                absolutePathToHomeDirectory = token; // save the pointer
  1019	                            }
  1020	                            /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
  1021	                            if(strcmp(token,userName) != 0){
  1022	                                line = line + lineSize;
  1023	                            }
  1024	                        }
  1025	                        /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
  1026	                        if(eofFound == 1){
  1027	                            SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
  1028	                            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1029	                            continue;
  1030	                        }
  1031	
  1032	                        /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
  1033	                        /* Declarations needed to form the new command argument */
  1034	                        // note: (char newCommandArg[512]) was declared as a global variable
  1035	                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
  1036	                        char * tmpPtr = &*absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
  1037	                        int lengthOfAbsolutePathToHomeDirectory = 0;
  1038	                        int numOfAdditionalCharacters = 0;
  1039	                        /*copy absolute path of home directory of a particular user */
  1040	                        i = 0;
  1041	                        while ((strncmp(tmpPtr,"\0",1)) != 0){
  1042	                            *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
  1043	                            lengthOfAbsolutePathToHomeDirectory++;
  1044	                        }
  1045	
  1046	                        /* possibly append the additional characters of new command argument */
  1047	                        if (additionalFlag == 1){
  1048	                            i = 0;
  1049	                            while(additionalPath[i] != NULLTERM){
  1050	                                *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
  1051	                                numOfAdditionalCharacters++;
  1052	                            }
  1053	                            *((currentTildaNewCommandArgPtr++)) = NULLTERM;
  1054	                        }
  1055	                        if(additionalFlag == 0){
  1056	                            *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
  1057	                        }
  1058	                        int lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
  1059	                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
  1060	                        outputRedirection = tildaArrayPtrs[currentNumOfTildas++];
  1061	                        // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
  1062	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1063	                        continue;
  1064	                    }
  1065	                }
  1066	            }
  1067	
  1068	            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
  1069	                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
  1070	                if(wordCount > 0){
  1071	                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
  1072	                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
  1073	                            newArgV[--wordCount] = NULL;
  1074	                            WAIT_FLAG = 1;
  1075	                            return 0;
  1076	                        }
  1077	                    }
  1078	                }
  1079	                newArgV[wordCount] = NULL;
  1080	                return 0;
  1081	            }
  1082	            if (getWordReturn == -255){
  1083	                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
  1084	                break;
  1085	            }
  1086	            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
  1087	                getWordReturn = abs(getWordReturn);
  1088	                outputRedirection = getenv(bigBufferPtr);
  1089	                if(outputRedirection == NULL){
  1090	                    SYNTAX_ERROR_ENVVAR = 1;
  1091	                    envVarNameError = bigBufferPtr;
  1092	                }
  1093	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1094	                continue;
  1095	            }
  1096	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==0){
  1097	                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
  1098	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1099	                continue;
  1100	            }
  1101	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==1){
  1102	                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
  1103	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1104	                continue;
  1105	            }
  1106	            /* Handling a character that is a metacharacter*/
  1107	            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
  1108	                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
  1109	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1110	                continue;
  1111	            }
  1112	            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
  1113	            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
  1114	                BACKSLASH_FLAG = 0;
  1115	                outputRedirection = bigBufferPtr;
  1116	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1117	                continue;
  1118	            }
  1119	
  1120	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
  1121	                outputRedirection = bigBufferPtr;
  1122	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
  1123	                continue;
  1124	            }
  1125	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
  1126	                BACKSLASH_FLAG = 0;
  1127	                outputRedirection = bigBufferPtr;
  1128	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
  1129	                continue;
  1130	            }
  1131	        }
  1132	        // Handling of more than one output redirection
  1133	        if((strcmp(bigBufferPtr,">")) == 0 && OUTPUT_REDIRECTION_FLAG == 1 && BACKSLASH_FLAG == 0){
  1134	            SYNTAX_ERROR_MULTIPLE_OUTPUT_REDIRECTIONS = 1;
  1135	            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1136	            getWordReturn = getword(bigBufferPtr);
  1137	            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
  1138	                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
  1139	                if(wordCount > 0){
  1140	                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
  1141	                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
  1142	                            newArgV[--wordCount] = NULL;
  1143	                            WAIT_FLAG = 1;
  1144	                            return 0;
  1145	                        }
  1146	                    }
  1147	                }
  1148	                newArgV[wordCount] = NULL;
  1149	                return 0;
  1150	            }
  1151	            if (getWordReturn == -255){
  1152	                SYNTAX_ERROR_NO_Output_FILE_SPECIFIED = 1;
  1153	                break;
  1154	            }
  1155	            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
  1156	                getWordReturn = abs(getWordReturn);
  1157	                outputRedirection = getenv(bigBufferPtr);
  1158	                if(outputRedirection == NULL){
  1159	                    SYNTAX_ERROR_ENVVAR = 1;
  1160	                    envVarNameError = bigBufferPtr;
  1161	                }
  1162	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1163	                continue;
  1164	            }
  1165	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==0){
  1166	                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
  1167	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1168	                continue;
  1169	            }
  1170	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0 && INPUT_REDIRECTION_FLAG==1){
  1171	                SYNTAX_ERROR_MULTIPLE_INPUT_REDIRECTIONS = 1;
  1172	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1173	                continue;
  1174	            }
  1175	            /* Handling a character that is a metacharacter*/
  1176	            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
  1177	                SYNTAX_ERROR_OUT_FILE_SPECIFIED_IS_META = 1;
  1178	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1179	                continue;
  1180	            }
  1181	            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
  1182	            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
  1183	                BACKSLASH_FLAG = 0;
  1184	                outputRedirection = bigBufferPtr;
  1185	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1186	                continue;
  1187	            }
  1188	
  1189	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
  1190	                outputRedirection = bigBufferPtr;
  1191	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
  1192	                continue;
  1193	            }
  1194	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1 ){
  1195	                BACKSLASH_FLAG = 0;
  1196	                outputRedirection = bigBufferPtr;
  1197	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
  1198	                continue;
  1199	            }
  1200	        }
  1201	
  1202	        // Handling a "where is" file
  1203	        if((strcmp(bigBufferPtr,"<<")) == 0 && BACKSLASH_FLAG == 0){
  1204	            HEREIS_FLAG = 1;
  1205	            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1206	            getWordReturn = getword(bigBufferPtr);
  1207	
  1208	            /* Handle substituting '~' for the correct path */
  1209	            if(strncmp(bigBufferPtr,"~",1) == 0 ){
  1210	                if (getWordReturn == 1){
  1211	                    hereIsKeyword = homeEnvVar;
  1212	                    bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1213	                    continue;
  1214	                }
  1215	                else{
  1216	                    if(strncmp(bigBufferPtr,"~/",2) == 0 ){
  1217	                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
  1218	                        for (i = 0; i < lengthOfHomeEnvVar-1; i++){
  1219	                            *(currentTildaNewCommandArgPtr++) = homeEnvVar[i];
  1220	                        }
  1221	                        for (i = 1; i < getWordReturn; i++){
  1222	                            *(currentTildaNewCommandArgPtr++) = *(bigBufferPtr+i);
  1223	                        }
  1224	                        *(currentTildaNewCommandArgPtr++) = NULLTERM;
  1225	                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
  1226	                        hereIsKeyword = tildaArrayPtrs[currentNumOfTildas++];
  1227	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1228	                        continue;
  1229	                    }
  1230	                        /*If we need to get /etc/passwd relative path so that we can replace the absolute pathname to the home directory of the given username*/
  1231	                    else{
  1232	                        char * startOfUserName;
  1233	                        char * userNamePtr;
  1234	                        char * startAdditionalCharacterPtr;
  1235	                        char * additionalCharactersPtr;
  1236	                        //note: (char userName[STORAGE]) was declared as a global variable for error handling
  1237	                        char additionalPath[STORAGE];
  1238	                        int additionalFlag = 0;
  1239	                        startOfUserName = bigBufferPtr + 1;
  1240	                        userNamePtr = startOfUserName;
  1241	                        i = 0;
  1242	
  1243	                        /* store the username in an array so we have something to compare to when we parse the user entries in the passwd file */
  1244	                        while(*userNamePtr != '/' && *userNamePtr != NULLTERM) {
  1245	                            userName[i++] = *userNamePtr++;
  1246	                        }
  1247	                        userName[i] = NULLTERM; // null terminate the username string
  1248	
  1249	                        /* Handle collecting possible additional characters to eventually append to end of absolute path */
  1250	                        startAdditionalCharacterPtr = userNamePtr;
  1251	                        additionalCharactersPtr = startAdditionalCharacterPtr;
  1252	                        i = 0;
  1253	                        /* create and load an array to gather additional path to eventually append to absolute path */
  1254	                        while(*additionalCharactersPtr != NULLTERM){
  1255	                            additionalFlag = 1;
  1256	                            additionalPath[i++] = *additionalCharactersPtr++;
  1257	                        }
  1258	                        if (additionalFlag == 1){
  1259	                            additionalPath[i] = NULLTERM;
  1260	                        }
  1261	
  1262	                        /* Open the file located at the path "/etc/passwd" so we can get the correct home directory of a particular user */
  1263	                        FILE * passwdFile = fopen("/etc/passwd", "r");
  1264	
  1265	                        /* Setup for parsing through the user entries in the file */
  1266	                        /* Declarations for using the getline function */
  1267	                        char * line = NULL;
  1268	                        size_t bufferSize = 0;
  1269	                        int lineFoundFlag = 0;
  1270	                        int eofFound = 0;
  1271	                        size_t lineSize;
  1272	                        /* Declarations for using the strtok function */
  1273	                        char * delimiter = ":";
  1274	                        char * token; // used to tokenize the correct user entry
  1275	
  1276	                        char * absolutePathToHomeDirectory;
  1277	
  1278	                        /* Parse the file */
  1279	                        while(lineFoundFlag != 1){
  1280	                            lineSize = getline(&line, &bufferSize, passwdFile);
  1281	                            token = strtok(line, delimiter);
  1282	
  1283	                            /* if we have found the end of the file or there was an error reading a line from the file */
  1284	                            if (lineSize == -1){
  1285	                                eofFound = 1;
  1286	                                break;
  1287	                            }
  1288	
  1289	                            /* if we have found the correct user entry tokenize the entry until the 5th (absolute pathname to the home directory of the user) */
  1290	                            if (strcmp(token,userName) == 0){
  1291	                                lineFoundFlag = 1;
  1292	
  1293	                                /*the correct absolute path name to the home directory is the fifth element in the user entry*/
  1294	                                for (i = 0; i < 5; i++){
  1295	                                    line = NULL; // do this to get the next token, read strtok docs for an explanation
  1296	                                    token = strtok(line, delimiter);
  1297	                                }
  1298	                                absolutePathToHomeDirectory = token; // save the pointer
  1299	                            }
  1300	                            /*if the we have parsed a different user entry get the next user entry by shimmying the line pointer*/
  1301	                            if(strcmp(token,userName) != 0){
  1302	                                line = line + lineSize;
  1303	                            }
  1304	                        }
  1305	                        /* if user entry not found flag an error and continue parsing by calling getword() with the next characters in the stdin stream*/
  1306	                        if(eofFound == 1){
  1307	                            SYNTAX_ERROR_ABSOLUTE_PATH_NOT_FOUND = 1;
  1308	                            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1309	                            continue;
  1310	                        }
  1311	
  1312	                        /* If successful with retrieving the absolute path of home directory from passwd file then create the new command argument*/
  1313	                        /* Declarations needed to form the new command argument */
  1314	                        // note: (char newCommandArg[512]) was declared as a global variable
  1315	                        char * beggingingOfNewCommandArgPtr = currentTildaNewCommandArgPtr;
  1316	                        char * tmpPtr = &*absolutePathToHomeDirectory; // for copying absolute path of home directory path of a particular user
  1317	                        int lengthOfAbsolutePathToHomeDirectory = 0;
  1318	                        int numOfAdditionalCharacters = 0;
  1319	                        /*copy absolute path of home directory of a particular user */
  1320	                        i = 0;
  1321	                        while ((strncmp(tmpPtr,"\0",1)) != 0){
  1322	                            *((currentTildaNewCommandArgPtr++)) = *tmpPtr++;
  1323	                            lengthOfAbsolutePathToHomeDirectory++;
  1324	                        }
  1325	
  1326	                        /* possibly append the additional characters of new command argument */
  1327	                        if (additionalFlag == 1){
  1328	                            i = 0;
  1329	                            while(additionalPath[i] != NULLTERM){
  1330	                                *((currentTildaNewCommandArgPtr++)) = additionalPath[i++];
  1331	                                numOfAdditionalCharacters++;
  1332	                            }
  1333	                            *((currentTildaNewCommandArgPtr++)) = NULLTERM;
  1334	                        }
  1335	                        if(additionalFlag == 0){
  1336	                            *((currentTildaNewCommandArgPtr++) + lengthOfAbsolutePathToHomeDirectory) = NULLTERM;
  1337	                        }
  1338	                        int lengthOfNewCommandArg = lengthOfAbsolutePathToHomeDirectory + numOfAdditionalCharacters + 1;
  1339	                        tildaArrayPtrs[currentNumOfTildas] = beggingingOfNewCommandArgPtr;
  1340	                        hereIsKeyword = tildaArrayPtrs[currentNumOfTildas++];
  1341	                        // Note currentTildaNewCommandArgPtr already pointing at right location for the next iteration
  1342	                        bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1343	                        continue;
  1344	                    }
  1345	                }
  1346	            }
  1347	
  1348	            if(getWordReturn == 0 && BACKSLASH_FLAG == 0){
  1349	                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
  1350	                if(wordCount > 0){
  1351	                    if(newArgV[wordCount-1] != NULL) { // handling if getenv fails and it's the last thing that's added to newArgV (strcmp can't compare a NULL ptr to a char), we've already set SYNTAX_ERROR_ENVVAR to the value 1 just get out of parse and main will handle the error
  1352	                        if (strcmp(newArgV[wordCount - 1], "&") == 0) { // Handling background processes even though we have found an error
  1353	                            newArgV[--wordCount] = NULL;
  1354	                            WAIT_FLAG = 1;
  1355	                            return 0;
  1356	                        }
  1357	                    }
  1358	                }
  1359	                newArgV[wordCount] = NULL;
  1360	                return 0;
  1361	            }
  1362	            if(getWordReturn == -255){
  1363	                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
  1364	                break;
  1365	            }
  1366	            if (getWordReturn < 0 ){ // handling if getword returns a negative return value, note EOF is taken care of already
  1367	                getWordReturn = abs(getWordReturn);
  1368	                hereIsKeyword = getenv(bigBufferPtr);
  1369	                if(hereIsKeyword == NULL){
  1370	                    SYNTAX_ERROR_ENVVAR = 1;
  1371	                    envVarNameError = bigBufferPtr;
  1372	                }
  1373	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1374	                continue;
  1375	            }
  1376	            /* Handling seeing multiple hereis meta characters one after another */
  1377	            if(strcmp(bigBufferPtr,"<<") == 0 && BACKSLASH_FLAG == 0){
  1378	                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
  1379	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1380	                continue;
  1381	            }
  1382	            /* Handling a character that is a metacharacter*/
  1383	            if(getWordReturn == 1 && BACKSLASH_FLAG == 0 && ((strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )){ // todo check "\" vs "\\"
  1384	                SYNTAX_ERROR_HEREIS_IDENTIFIER = 1;
  1385	                bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1386	                continue;
  1387	            }
  1388	            /* Handling a character that is a metacharacter and BACKSLASH_FLAG == 1 */
  1389	            if(getWordReturn == 1 && BACKSLASH_FLAG == 1 && ((strcmp(bigBufferPtr,";") == 0) ||(strcmp(bigBufferPtr," ") == 0) || (strcmp(bigBufferPtr,"|") == 0) || (strcmp(bigBufferPtr,"&") == 0) || (strcmp(bigBufferPtr,"<") == 0) || (strcmp(bigBufferPtr,">") == 0) || (strcmp(bigBufferPtr,"\\") == 0) )) {
  1390	                BACKSLASH_FLAG = 0;
  1391	                hereIsKeyword =  bigBufferPtr;
  1392	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1393	                continue;
  1394	            }
  1395	
  1396	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 0){
  1397	                hereIsKeyword =  bigBufferPtr;
  1398	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
  1399	                continue;
  1400	            }
  1401	            if(getWordReturn >= 1 && BACKSLASH_FLAG == 1){
  1402	                BACKSLASH_FLAG = 0;
  1403	                hereIsKeyword =  bigBufferPtr;
  1404	                bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for null terminator
  1405	                continue;
  1406	            }
  1407	        }
  1408	
  1409	        // Handling a potential background process
  1410	        if((strcmp(bigBufferPtr,"&")) == 0 && BACKSLASH_FLAG == 0){
  1411	            newArgV[wordCount++] = bigBufferPtr;
  1412	            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1413	            continue;
  1414	        }
  1415	        // Handling of a single or multiple pipelines, keeps track of locations of executable names by tracking indexes values of newArgV
  1416	        if((strcmp(bigBufferPtr,"|")) == 0  && BACKSLASH_FLAG == 0){
  1417	            PIPELINE_FLAG = 1; // set this flag so main() can handle accordingly 
  1418	            newArgV[wordCount++] = NULL;
  1419	            processOffsets[++numExecsCounter] = wordCount;
  1420	            bigBufferPtr = bigBufferPtr + getWordReturn + 1;
  1421	            continue;
  1422	        }
  1423	
  1424	        // Handling a regular word
  1425	        newArgV[wordCount++] = bigBufferPtr;
  1426	        BACKSLASH_FLAG = 0;
  1427	        // Update pointer for next iteration
  1428	        bigBufferPtr = bigBufferPtr + getWordReturn + 1; // +1 for the null terminator
  1429	    }//note if the stdin
  1430	    return -255;
  1431	}
