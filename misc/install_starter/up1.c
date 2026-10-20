// Windows version ...
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
//#include <process.h>
#include <direct.h>
//#include "incbin.h"

//INCBIN(App, "app_bundle.tar.xz");

char *tmpdir;

void error(char *s)
{
    perror(s);
    free(tmpdir);
    exit(1);
}

int main(int argc, char **argv)
{
    const char* s = getenv("TMP");
    // If the environment variable doesn't exist, it returns NULL
    printf("PATH :%s\n", (s != NULL) ? s : "getenv returned NULL");

    
    tmpdir = _tempnam( NULL, "AppInstaller" );
    if (!tmpdir)
    {
       error("Error occurred creating unique filename.\n");
    }
    else
    {
       printf( "%s is safe to use as a temporary file.\n", tmpdir );
    }

    if ( _mkdir(tmpdir) ) {
        fprintf(stderr, "Failed to create temporary directory: %s\n", tmpdir);
        error("");
    }

    int count;
    int size;
    char *cmd = NULL;
    const char *cmd0 = "rmdir /s /q \"%s\"";
    count = _scprintf(cmd0, tmpdir);
    size = count + 1; // the string will need one more char for the null terminator
    cmd = malloc(sizeof(char) * size);
    sprintf_s(cmd, size, cmd0, tmpdir);
    printf("The length of the following string will be %i.\n", count);
    printf("%s\n", cmd);
    _flushall();
    int sysrc = system(cmd);
    printf("System -> %d\n", sysrc);
    if ( sysrc != 0 ) {
        fprintf(stderr, "Failed to remove temporary directory: %s\n", tmpdir);
        if ( sysrc == -1 ) error("");
    }
    free( cmd );

    printf("Done!\n");
    free(tmpdir);
  
    return 0;
}
