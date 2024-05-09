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

/* Print out a LOTR Image Slowly */
int main() {
  // Define the image to print
  int pic_lines = 28; // From the image itself
  char *pic[] = {
"                    . .:.:.:.:. .:\\     /:. .:.:.:.:. ,",
"               .-._  `..:.:. . .:.:`- -':.:. . .:.:.,'  _.-.",
"              .:.:.`-._`-._..-''_...---..._``-.._.-'_.-'.:.:.",
"           .:.:. . .:_.`' _..-''._________,``-.._ `.._:. . .:.:.",
"        .:.:. . . ,-'_.-''      ||_-(O)-_||      ``-._`-. . . .:.:.",
"       .:. . . .,'_.'           '---------'           `._`.. . . .:.",
"     :.:. . . ,','               _________               `.`. . . .:.:",
"    `.:.:. .,','            _.-''_________``-._            `._.     _.'",
"  -._  `._./ /            ,'_.-'' ,       ``-._`.          ,' '`:..'  _.-",
" .:.:`-.._' /           ,','                   `.`.       /'  '  \\\\.-':.:.",
" :.:. . ./ /          ,','               ,       `.`.    / '  '  '\\\\. .:.:",
":.:. . ./ /          / /    ,                      \\ \\  :  '  '  ' \\\\. .:.:",
".:. . ./ /          / /            ,          ,     \\ \\ :  '  '  ' '::. .:.",
":. . .: :    o     / /                               \\ ;'  '  '  ' ':: . .:",
".:. . | |   /_\\   : :     ,                      ,    : '  '  '  ' ' :: .:.",
":. . .| |  ((<))  | |,          ,       ,             |\\'__',-._.' ' ||. .:",
".:.:. | |   `-'   | |---....____                      | ,---\\/--/  ' ||:.:.",
"------| |         : :    ,.     ```--..._   ,         |''  '  '  ' ' ||----",
"_...--. |  ,       \\ \\             ,.    `-._     ,  /: '  '  '  ' ' ;;..._",
":.:. .| | -O-       \\ \\    ,.                `._    / /:'  '  '  ' ':: .:.:",
".:. . | |_(`__       \\ \\                        `. / / :'  '  '  ' ';;. .:.",
":. . .<' (_)  `>      `.`.          ,.    ,.     ,','   \\  '  '  ' ;;. . .:",
".:. . |):-.--'(         `.`-._  ,.           _,-','      \\ '  '  '//| . .:.",
":. . .;)()(__)(___________`-._`-.._______..-'_.-'_________\\'  '  //_:. . .:",
".:.:,' \\/\\/--\\/--------------------------------------------`._',;'`. `.:.:.",
":.,' ,' ,'  ,'  /   /   /   ,-------------------.   \\   \\   \\  `. `.`. `..:",
",' ,'  '   /   /   /   /   //                   \\\\   \\   \\   \\   \\  ` `.SSt",
"https://www.asciiart.eu/books/lord-of-the-rings  Designed by Unknown Artist (SSt?)"
}; //https://asciiartist.com/respect-ascii-artists-campaign/

  int line = 0; // We'll need this after the loop.
  // Iterate to print all but the last line in the image, using the given print rate timing.
  for(line = 0; line < (pic_lines - 1); line++) {
    printf("%s[PID: %d] %s%s\n", GREEN, getpid(), pic[line], RST);
    usleep(PRINT_RATE_USEC); 
  }

  // Special for the last line to be a different color
  printf("%s[PID: %d]%s %s\n", GREEN, getpid(), RST, pic[line]);
  
  return 0;
}
