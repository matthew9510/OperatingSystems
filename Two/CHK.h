/* CHK.h
 * (CHK is modeled after assert.)
 * Usage       CHK( system call );
 * If the system call is one of the majority that return -1 on error,
 * this macro will cause the program to terminate on such an error
 * with a helpful diagnostic.
 * The "do ... while" is at the suggestion of David Butenhof (see p33,34
 * of his book _Programming with POSIX Threads_, Addison-Wesley, 1997)
 */
#ifndef CHK_H
/* guard against redefinition by previous includes */
#define CHK_H
/* CHK may be used in some subroutines that may not
 * normally print anything, so:			   */
#include <stdio.h>
#include <string.h>
#include <errno.h>
/* The values of __FILE__ and __LINE__ are
 * predefined macros related to the placement
 * of the line in the source file
 */
#define CHK(x)  \
  do {if((x) == -1)\
       {fprintf(stderr,"In file %s, on line %d:\n",__FILE__,__LINE__);\
        fprintf(stderr,"errno = %d\n",errno);\
        perror("Exiting because");\
        exit(1);\
       }\
     } while(0)

/* for pthread functions that return nonzero on error: */
#define pCHK(x)  \
  do {int res = 0;\
     if((res = (x)) != 0)\
       {fprintf(stderr,"In file %s, on line %d:\n",__FILE__,__LINE__);\
        fprintf(stderr,"pthreads problem: %s.\nExiting.\n",strerror(res));\
        exit(1);\
       }\
     } while(0)
#endif 

/* Each #define statement should be a one-line definition, but this would
 * be very hard to read.  C (and most shells) replace the backslash-newline
 * combination with a single space, so CHK() and pCHK() are both 'really'
 * one-line definitions, even though they are multi-line formatted to
 * highlight their logical structure and be more human-readable.
 */
