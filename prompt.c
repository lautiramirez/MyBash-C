#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define ANSI_RESET  "\x1b[0m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_WHITE  "\033[38;2;255;255;255m"

#include "prompt.h"

void show_prompt(void){
    char direc[100];
    char hostname[100];
    gethostname(hostname, sizeof(hostname));
    struct passwd *p = getpwuid(getuid());
    printf(ANSI_WHITE"%s@%s"ANSI_RESET":"ANSI_GREEN"~%s"ANSI_RESET"$ ", p->pw_name, hostname, getcwd(direc, 100));
    fflush (stdout);
}
