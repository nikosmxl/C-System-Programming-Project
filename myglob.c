#include <glob.h>
#include <stdlib.h>
#include <stdbool.h>

int myglob(char* parsed, char** arguments, int count, bool redirection){
    int initialcount = count;
    int r;
    glob_t gb;
    r = glob(parsed, GLOB_ERR, NULL, &gb);

    if (r != 0){
        if (r == GLOB_NOMATCH)
            return -1;
        else
            return -2;
    }

    if (redirection){
        if (gb.gl_pathc > 1){
            return -3;
        }
    }

    char** found = gb.gl_pathv;
    while(*found){
        arguments[count++] = *found;
        found++;
    }

    return count - initialcount;
}