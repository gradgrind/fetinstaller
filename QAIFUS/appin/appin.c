// Linux version ...

// Compile with:
//   musl-gcc -static -o appin appin.c

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
int msg_free = 0;
int childstate = 0;

void fatal();
void tidy();

int main(int argc, char **argv)
{
    // Create a temporary directory to receive the unpacked installer.
    // The following code assumes that only one installer should be active
    // at any time (for any one user). If a second installer is started,
    // "app_install" can detect this by means of the trailing "_" on the
    // path name, so it can report the situation, but otherwise be unhindered.
    // A further attempt to start an installer will fail.
    char* tmp0 = getenv("TMPDIR");
    if ( tmp0 == NULL || strlen(tmp0) < 4 ) {
        tmp0 = "/tmp";
    }
    char* user = getlogin();
    asprintf(&tmpdir, "%s/%s_gradgrind_AppIn", tmp0, user);
    if ( mkdir(tmpdir, 0700) != 0 ) {
        free(tmpdir);
        asprintf(&tmpdir, "%s/%s_gradgrind_AppIn_", tmp0, user);
        if ( mkdir(tmpdir, 0700) != 0 ) {
            asprintf(&msg, "Temporary directory unavailable: %s\n", tmpdir);
            msg_free = 1;
            free(tmpdir);
            tmpdir = NULL;
            fatal();
        }
        // In this case, app_install should comment on this, actually
        // there should be only one instance per user.
    }

    //printf("%p %u %p\n", &gAppData[0], gAppSize, &gAppEnd);
    //printf("%p\n", &gAppData[gAppSize]);
    //assert(&gAppData[gAppSize] == (const unsigned char*) &gAppEnd);

    // A pipe to send data to the child process.
    // In a pipe, xx[0] is for reading, xx[1] is for writing.
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
        //printf("Spawned 'tar -xJ ...' as a child process at pid %d\n", p);
        close(fd1[0]); // Close reading end of pipe
        // Write input string and close writing end of first pipe.
        write(fd1[1], &gAppData[0], gAppSize);
        close(fd1[1]);
        // Wait for child to finish
        wait(NULL);
    }

    // Child process
    else {
        // Close stdin.
        close(0);
        // Make the pipe the new stdin.
        dup2(fd1[0],0);
        close(fd1[1]); // Close writing end of pipe.
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

void fatal()
{
    char* emsg = "";
    if ( errno != 0 ) {
        emsg = strerror(errno);
        
        //fprintf(stderr, emsg);
        //fprintf(stderr, "\n");
    }

    char* msg0 = "";
    if ( msg ) {
        msg0 = msg;
        //fprintf(stderr, msg);
        //fprintf(stderr, "\n");
    }

    char* e0 = NULL;
    asprintf(&e0, "Couldn't start installer ...\n%s\n%s", emsg, msg0);
    char* en = NULL;
    asprintf(&en, "notify-send \"AppIn\" \"%s\" -i error", e0);
    fprintf(stderr, "%s\n", e0);
    system(en);
    free(en);
    free(e0);
    if ( msg ) {
        free(msg);
        msg_free = 0;
    }
    tidy();
    exit(1);
}

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
            msg_free = 1;
            tmpdir = NULL;
            fatal();
        }
        free (cmd);
        free (tmpdir);
    }
}
