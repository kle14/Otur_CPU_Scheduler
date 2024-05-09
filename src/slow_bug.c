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
#define PRINT_RATE_USEC 500000 // 1000 = 1ms, 1000000 = 1sec

// Prints out ASCII art.  One line every PRINT_RATE_USEC microseconds.
int main(int argc, char *argv[]){
  // Image definition to print
  int pic_lines = 22;
  char *pic[] = {
"Fight Bugs                      |     |        ",
"                                \\\\_V_//        ",
"                                \\/=|=\\/        ",
"                                 [=v=]         ",
"                               __\\___/_____    ",
"                              /..[  _____  ]   ",
"                             /_  [ [  M /] ]   ",
"                            /../.[ [ M /@] ]   ",
"                           <-->[_[ [M /@/] ]   ",
"                          /../ [.[ [ /@/ ] ]   ",
"     _________________]\\ /__/  [_[ [/@/ C] ]   ",
"    <_________________>>0---]  [=\\ \\@/ C / /   ",
"       ___      ___   ]/000o   /__\\ \\ C / /    ",
"          \\    /              /....\\ \\_/ /     ",
"       ....\\||/....           [___/=\\___/      ",
"      .    .  .    .          [...] [...]      ",
"     .      ..      .         [___/ \\___]      ",
"     .    0 .. 0    .         <---> <--->      ",
"  /\\/\\.    .  .    ./\\/\\      [..]   [..]      ",
" / / / .../|  |\\... \\ \\ \\    _[__]   [__]_     ",
"/ / /       \\/       \\ \\ \\  [____>   <____]    ",
"https://www.asciiart.eu/computers/bug Designed by Unknown Artist"
}; //https://asciiartist.com/respect-ascii-artists-campaign/

  int line = 0; // We'll need this after the loop.
  // Iterate to print all but the last line in the image, using the given print rate timing.
  for(line = 0; line < (pic_lines - 1); line++) {
    printf("%s[PID: %d] %s%s\n", CYAN, getpid(), pic[line], RST);
    usleep(PRINT_RATE_USEC); 
  }

  // Special for the last line to be a different color
  printf("%s[PID: %d]%s %s\n", CYAN, getpid(), RST, pic[line]);
  
  return 0;
}
