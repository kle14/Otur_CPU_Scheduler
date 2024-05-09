/* Standard Libraries */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/* System Libraries */
#include <unistd.h>
#include <sys/types.h>
/* Project Libraries */
#include "vm_printing.h"

/* Local Definitions */
#define DEFAULT_COUNTUP 10  // Without args, counts up from 0 from here

// Count up from 0 to N, printing one line per second
// Always returns N!
int main(int argc, char *argv[]){
  // Initialize the counter
  int goal = DEFAULT_COUNTUP;

  // If arg is provided, attempt to convert to a number (base-10)
  if(argc == 2) {
    char *endptr = NULL;  // Used for error checking
    goal = strtol(argv[1], &endptr, 10); // Convert to a base-10 long
    if(*endptr != '\0' || *argv[1] == '\0') { // Value was not converted properly!
      goal = DEFAULT_COUNTUP;
    }
  }
	
  // Iterate counter times, counting up once per second
  int counter = 0; // We need this in main scope for the return.
  for (counter = 0; counter < goal; counter++) {
  	printf("%s[PID: %d] slow_countup counting up: %d ...\n%s", YELLOW, getpid(), counter, RST);
  	sleep(1);
  }

  printf("%s[PID: %d] slow_countup counting up: %d ...\n%s", YELLOW, getpid(), counter, RST);

  return counter;
}
