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
#define DEFAULT_COUNTDOWN 10  // Without args, counts down to 0 from here

// Count down from N to 0, printing one line per second
// Always returns 0!
int main(int argc, char *argv[]){
  // Initialize the counter
  int counter = DEFAULT_COUNTDOWN;

  // If arg is provided, attempt to convert to a number (base-10)
  if(argc == 2) {
    char *endptr = NULL;  // Used for error checking
    counter = strtol(argv[1], &endptr, 10); // Convert to a base-10 long
    if(*endptr != '\0' || *argv[1] == '\0') { // Value was not converted properly!
      counter = DEFAULT_COUNTDOWN;
    }
  }
	
  // Iterate counter times, counting down once per second
  while (counter >= 0) {
  	printf("%s[PID: %d] slow_countdown count down: %d ...\n%s", MAGENTA, getpid(), counter, RST);
  	counter--;
  	sleep(1);
  }

  return 0;
}
