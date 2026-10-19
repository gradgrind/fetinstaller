// Windows version ...
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <errno.h>
//#include <process.h>
#include <direct.h>
//#include "incbin.h"

INCBIN(App, "app_bundle.tar.xz");

char *tmpdir;
char* msg = NULL;
int childstate = 0;

void fatal();
char* compose(char* fmt, ...);

void tidy()
{
    // Tidy up: remove temporary directory
    if ( tmpdir ) {
        char *cmd = NULL;
        const char *cmd0 = "rm -r \"%s\"";
        asprintf(&cmd, cmd0, tmpdir);
        //printf("%s\n", cmd);
        int sysrc = system(cmd);
        //printf("System -> %d\n", sysrc);
        if ( sysrc != 0 ) {
            const char* msg0 = "Couldn't remove temporary directory: %s\n";
            asprintf(&msg, msg0, tmpdir);
            tmpdir = NULL;
            fatal();
        }
        free( cmd );
        free (tmpdir);
    }
}

void fatal()
{
    if ( errno != 0 ) {
        const char* emsg = strerror(errno);
        fprintf(stderr, emsg);
        fprintf(stderr, "\n");
    }

    if ( msg ) {
        fprintf(stderr, msg);
        fprintf(stderr, "\n");
    }

    // Desktop notification
    //TODO: Also app being installed?
    //char* e = NULL;
    const char* e0 = "notify-send \"AppIn\" \"Installation failed, more details in terminal.\" -i error";
    //asprintf(&e, e0, emsg, msg);
    //system(e);
    //free(e);
    system(e0);
    tidy();
    exit(1);
}

//TODO--
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


    tmpdir = _tempnam(NULL, "AppIn");
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
    free(cmd);

    printf("Done!\n");
    free(tmpdir);

    return 0;
}

// Variadic function to compose strings.
// This works like printf, but returns a string, whose memory must be freed.
char* compose(char* fmt, ...)
{
    int count;
    int size;
    char* cmps = NULL;
    //const char* fmt = "rmdir /s /q \"%s\"";
    va_list args;
    va_start(args, fmt);
    count = _vscprintf(fmt, args);
    size = count + 1; // the string will need one more char for the null terminator
    cmps = malloc(sizeof(char) * size);
    vsprintf_s(cmps, size, fmt, args);
    printf("The length of the following string will be %i.\n", count);
    printf("%s\n", cmps);
    //_flushall();
    //int sysrc = system(cmd);
    //printf("System -> %d\n", sysrc);
    //if ( sysrc != 0 ) {
    //    fprintf(stderr, "Failed to remove temporary directory: %s\n", tmpdir);
    //    if ( sysrc == -1 ) error("");
    //}

    return cmps;
    //free(cmps);
}
