//#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include "incbin.h"

INCBIN(App, "../build/app_bundle.tar.xz");

char* tmpdir = NULL;
char* msg = NULL;
int childstate = 0;

void fatal();

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

int main(int argc, char **argv)
{
    // Note, the compiler may issue a security warning, e.g. GCC:
    // "warning: the use of `tempnam' is dangerous, better use `mkstemp'"
    tmpdir = tempnam(NULL, "AppIn");
    //printf("temporary file name: %s\n", tmpdir);
    if (mkdir(tmpdir, 0700) != 0) {
        const char *msg0 = "Failed to create temporary directory: %s\n";
        asprintf(&msg, msg0, tmpdir);
        fatal();
    }

    //printf("%p %u %p\n", &gAppData[0], gAppSize, &gAppEnd);
    //printf("%p\n", &gAppData[gAppSize]);
    //assert(&gAppData[gAppSize] == (const unsigned char*) &gAppEnd);

    // A pipe to send data to the child process
    // In a pipe, xx[0] is for reading, xx[1] is for writing
    int fd1[2];
    if (pipe(fd1) == -1) {
        msg = "Pipe Failed.";
        fatal();
    }

    pid_t p;
    p = fork();
    if (p < 0) {
        msg = "Fork Failed.";
        fatal();
    }

    // Parent process
    if (p > 0) {
        //printf("Spawned 'tar -xJ' as a child process at pid %d\n", p);

        close(fd1[0]); // Close reading end of pipe

        // Write input string and close writing end of first pipe.
        write(fd1[1], &gAppData[0], gAppSize);
        close(fd1[1]);

        // Wait for child to finish
        wait(NULL);
    }

    // Child process
    else {
        // Close stdin
        close(0);
        // Make the pipe the new stdin
        dup2(fd1[0],0);

        close(fd1[1]); // Close writing end of pipe

        // Hand control over to the 'tar' binary
        execlp("tar", "tar", "-xJf", "-", "-C", tmpdir, (char*)NULL);
        msg = "Unpacking app bundle failed.";
        childstate = -1;
        fatal();
    }

    if ( childstate != 0 ) {
        return 1;
    }
    
    chdir(tmpdir);
    if ( system("_installer_/app_install") != 0 ) {
        msg = "Installer failed.";
        fatal();
    }

    tidy();
    return 0;
}
