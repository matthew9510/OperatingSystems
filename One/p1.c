/* ~cs570/One/getword.h provides a good example of the level of documentation
 * appropriate for this course. This p1.c provides an outstandingly BAD example.
 */

#include "getword.h"

int main()

{
int c;
char s[STORAGE];

for(;;) {
	(void) printf("n=%d, s=[%s]\n", c = getword(s), s);
	if (c == -255) break;
	}
}
