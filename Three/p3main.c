/*  p3main.c
      
      This is the driver program for Program p3.
      It constitutes an important part of the specification for Program 3.
      You code, however, will all be in p3helper.c
      (The p3helper.c stub provided in the assignment directory
      should compile and link with this -- and run (though the results
      are clearly wrong).)

      If a run gets jammed, control-C will normally terminated it. 
      */
  
#include "p3.h"

/* Your p3helper() code should NOT access the following three semaphores: */
static sem_t alldone; /* for coordinating the windup printout */
static sem_t statsprotect; /* to guard access to the stats_file */
static sem_t randomprotect; /* rand() is not thread safe; see saferand below */

static pthread_t jogger_thread[MAXCUSTOMERS];
static pthread_t shooter_thread[MAXCUSTOMERS];

static int stats[2] = { 0, 0 }; /* Shared record-keeping area.  Your
                                   added code should not access this array. */
static int failure = 0;         /* set if there are ever simultaneous joggers
                                   and shooters.  Your added code should not
                                   access this variable. */
static int maxconcur = 0;       /* track the maximum concurrency of joggers */
static int maxconcur2 = 0;      /* track the maximum concurrency of shooters */

static long saferand(void); /* the standard random wrapped in a semaphore */
static void *jogger(void *);  /* code run by each jogger thread */
static void *shooter(void *pn); /* code run by each shooter thread */

/* BLECH! without the following arrays, it looks like threads
   can get the wrong jogger_id (and similarly for the shooters):*/
static int jog[MAXCUSTOMERS];
static int shoot[MAXCUSTOMERS];

int main(int argc, char *argv[]) {
  int jogger_id;
  int shooter_id;
  int nr_today; /* Total number of customers on this run */
  int i; /* loop counter */
  int pres; /* result code I'll use for pthread error reporting */

  printf("p3 is now running (pid = %d).\n",(int)getpid());
  (void) fflush(stdout);

  /* Initialize random number generator (with the first command
     line argument if any is provided). I'm doing this so we
     can simulate a variety of timings on different runs. */
  srand( (argc<2) ? SEED : atoi(argv[1]) );
  
  initstudentstuff();  /* provided by you, in p3helper.c */

  CHK(sem_init(&alldone,LOCAL,0));
  CHK(sem_init(&statsprotect,LOCAL,1));
  CHK(sem_init(&randomprotect,LOCAL,1));

  /* Starting up "today's" mix of customers: */
  nr_today = saferand()%MAXCUSTOMERS + 1;
  printf("%d customers today: ", nr_today);
  (void) fflush(stdout);
  jogger_id=0;
  shooter_id=0;
  for ( i=0; i < nr_today; i++ ) {
    if ( saferand()%2 ) {
      jogger_id++;
      jog[jogger_id] = jogger_id;
      if ((pres = pthread_create(&jogger_thread[jogger_id],
                                 NULL,
                                 jogger,
                                 (void *)&(jog[jogger_id]))) != 0) {
        fprintf(stderr, "Exiting process: could not create jogger %d, %s\n",
                jogger_id,strerror(pres));
        exit(1);
      }
    } else {
      shooter_id++;
      shoot[shooter_id] = shooter_id;
      if ((pres = pthread_create(&shooter_thread[shooter_id],
                                 NULL,
                                 shooter,
                                 (void *)&(shoot[shooter_id]))) != 0) {
        fprintf(stderr, "Exiting process: could not create shooter %d, %s\n",
                shooter_id,strerror(pres));
        exit(1);
      }
    }
  }

  printf("%d joggers, %d shooters\n", jogger_id, shooter_id);
  (void) fflush(stdout);

  /* Now the parent blocks till all the customers are done: */
  for (i=0; i< nr_today ; i++) {
    CHK(sem_wait(&alldone));
  }
  printf("alldone -- THE GYM IS NOW CLOSED\n");
  printf("DAILY REPORT:\nTotal number of different customers:\n");
  printf("Number of joggers:%d; Number of shooters:%d\n",jogger_id,shooter_id);
  printf("People still in the gym (better be 0 for each kind!):\n");
  printf("Number of joggers left: %d; Number of shooters left: %d\n",
         stats[JOGGER],stats[SHOOTER]);
  printf("Maximum of =%d= instances of protocol failure for %s!\n", failure, getenv("USER"));
  printf("Maximum simultaneous jogger occupancy was @%d@ for %s\n", maxconcur, getenv("USER"));
  printf("Maximum simultaneous shooter occupancy was @%d@ for %s\n", maxconcur2, getenv("USER"));
  (void) fflush(stdout);
  exit(0);
}

/*-----------------------------------------------------------------------
 * Name: jogger
 * Purpose: Simulates a jogger in our gym.
 This routine is intended to be run as a separate thread.
 Prints an entrance announcement (to stdout).
 Prints (to stdout) a second line when it is leaving the gym.
 Upon completion, sem_post(&alldone) and terminates.
 * Input parameters: The jogger's id number.
 * Output parameters: None.
 * Other side effects: See above. Any error reporting is done from within
 the routine.
 * Routines called: prolog,epilog,sem_post,sem_wait
 * References: See the program3 assignment file
 */
void *jogger(void *pn) {
  int  k; /* loop counter for turns */
  int turn; /* should be one more than the loop counter :-( */
  int goal;
  int n;

  goal = (saferand()%MAXSESSIONS)+1;
  n = *(int *)pn;
  
  ARRIVAL_DELAY();
  for (k=0; k<goal ; k++) {
    turn = k+1;
    prolog(JOGGER); /* get permission to enter the gym floor */

    /* The following lines can be thought of as the customer's signing in.
       In fact, they provide us with the major testing output of the program
     */    
    CHK(sem_wait(&statsprotect));
    (stats[JOGGER])++;
    printf("J%d [%d/%d]. Just after entering: "
            "%d JOGGERS and %d shooters\n",
            n,turn,goal,stats[JOGGER],stats[SHOOTER]);
    if ( stats[JOGGER] > maxconcur ) {
          maxconcur = stats[JOGGER];
    }
    if ( stats[JOGGER]&&stats[SHOOTER] ) {
            failure++;
            printf("ERROR! Both types of customers are present in the gym!\n");
    }
    (void) fflush(stdout);
    CHK(sem_post(&statsprotect));
    
    JOGGING(); /* this macro is defined in p3.h */
    
    /* The following lines can be thought of as the customer's signing out.
       In fact, they provide us with the major testing output of the program
     */    
    CHK(sem_wait(&statsprotect));
    (stats[JOGGER])--;
    printf("J%d [%d/%d]. Just after leaving: "
            "%d JOGGERS and %d shooters\n",
            n,turn,goal,stats[JOGGER],stats[SHOOTER]);
    (void) fflush(stdout);
    CHK(sem_post(&statsprotect));
    
    epilog(JOGGER); /* customer gives up his/her access permission to floor */
    RESTING();RESTING();
  }
  printf("J%d all done\n",n);
  (void) fflush(stdout);
  CHK(sem_post(&alldone));
  return(NULL);
}

/*-----------------------------------------------------------------------
 * Name: shooter
 * Purpose: Simulates a shooter in our gym.
 This routine is intended to be run as a separate thread.
 Prints an entrance announcement (to stdout).
 Prints (to stdout) a second line when it is leaving the gym.
 Upon completion, sem_post(&alldone) and terminates.
 * Input parameters: The shooter's id number.
 * Output parameters: None.
 * Other side effects: See above. Any error reporting is done from within
 the routine.
 * Routines called: prolog,epilog,sem_post,sem_wait
 * References:  See the program3 assignment file
 */
void *shooter(void *pn) {
  int  k; /* loop counter for turns */
  int turn; /* should be one more than the loop counter :-( */
  int goal;
  int n;

  goal = (saferand()%MAXSESSIONS)+1;
  n = *(int *)pn;
  
  ARRIVAL_DELAY();
  for (k=0; k<goal ; k++) {
    turn = k+1;
    prolog(SHOOTER); /* get permission to enter the gym floor */

    /* The following lines can be thought of as the customer's signing in.
       In fact, they provide us with the major testing output of the program
     */    
    CHK(sem_wait(&statsprotect));
    (stats[SHOOTER])++;
    printf("S%d [%d/%d]. Just after entering: "
            "%d joggers and %d SHOOTERS\n",
            n,turn,goal,stats[JOGGER],stats[SHOOTER]);
    if ( stats[SHOOTER] > maxconcur2 ) {
          maxconcur2 = stats[SHOOTER];
    }
    (void) fflush(stdout);
    CHK(sem_post(&statsprotect));
    
    SHOOTING(); /* this macro is defined in p3.h */
    
    /* The following lines can be thought of as the customer's signing out.
       In fact, they provide us with the major testing output of the program
     */    
    CHK(sem_wait(&statsprotect));
    (stats[SHOOTER])--;
    printf("S%d [%d/%d]. Just after leaving: "
            "%d joggers and %d SHOOTERS\n",
            n,turn,goal,stats[JOGGER],stats[SHOOTER]);
    (void) fflush(stdout);
    CHK(sem_post(&statsprotect));

    epilog(SHOOTER); /* customer gives up his/her access permission to floor */
    RESTING();RESTING();
  }
  printf("S%d all done\n",n);
  (void) fflush(stdout);
  CHK(sem_post(&alldone));
  return(NULL);
}

long saferand(void) {
  long result;
  
  CHK(sem_wait(&randomprotect));
  result = rand();
  CHK(sem_post(&randomprotect));
  return result;
}
