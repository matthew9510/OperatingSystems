/* getword.h - header file for the getword() function; similar to that used in
   a series of CS570 programs, SDSU.

  This header file, used in the zero'th programming assignment, is horridly
  documented.  A functionally similar, but appropriately documented .h file
  can be found in ~cs570/One/getword.h, to be used with p1, p2, and p4.  It is
  useful to read the more complete getword.h code, to better understand what the
  following declarations do.  But note that ~cs570/One/getword.h describes more
  *and different* functionality than what is required for p0.  See ~cs570/program0
  for the simple things your initial attempt a creating getword.c should handle.
*/

#include <stdio.h>
#include <string.h>
#include <strings.h>

#define STORAGE 255

int getword(char *w);
