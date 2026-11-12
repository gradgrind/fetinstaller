// Compile with:
// musl-gcc -static -o appin appin.c

//#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ftw.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include "incbin.h"

INCBIN(App, "../build/app_bundle.tar.xz");

char* msg = NULL; // constant strings, don't free
char* childmsg = NULL; // constant strings, don't free
char* msgx = NULL; // needs freeing
char* tmpdir = NULL; // needs freeing

void fatal();
void tidy();

int main(int argc, char **argv)
{
    // Make a temporary directory inside the user's home directory.
    char* tmp0 = NULL;
    {
        uid_t uid = getuid();
        uid_t euid = geteuid();
        if ( uid != euid ) {
            msg = "UID mismatch.";
            fatal();
        }
        struct passwd* pw = getpwuid(uid);
        if ( !pw ) {
            msg = "User unknown.";
            fatal();
        }
        // Set the temporary base directory.
        asprintf(&tmp0, "%s/.local/tmp", pw->pw_dir);
        struct stat sbuf;
        int rc = stat(tmp0, &sbuf);
        // Check if the directory exists
        if ( rc != 0 ) {
            // Create the directory
            if ( mkdir(tmp0, 0700) != 0 ) {
                asprintf(&msgx, "Failed: create temporary directory base: %s", tmp0);
            }
        } else if ( !S_ISDIR(sbuf.st_mode) ) {
            asprintf(&msgx, "Invalid temporary directory base: %s", tmp0);
        }
        if ( msgx ) {
            free(tmp0);
            fatal();
        }
        errno = 0;
    }

    asprintf(&tmpdir, "%s/gradgrind_AppIn", tmp0);
    if ( mkdir(tmpdir, 0700) != 0 ) {
        free(tmpdir);
        asprintf(&tmpdir, "%s/gradgrind_AppIn_", tmp0);
        if ( mkdir(tmpdir, 0700) != 0 ) {
            asprintf(&msgx, "Temporary directory unavailable: %s", tmpdir);
            free(tmp0);
            free(tmpdir);
            tmpdir = NULL;
            fatal();
        }
        // In this case, app_install should comment on this, actually
        // there should be only one instance (per user).
    }
    free(tmp0);
    errno = 0;

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
        ssize_t s = write(fd1[1], &gAppData[0], gAppSize);
        close(fd1[1]);

        if ( s != gAppSize ) {
            msg = "Failed: swaitpidend package to unpacker";
            // This will probably cause the child process to fail, too.
        }

        // Wait for child to finish
        int stat;
        if ( waitpid(p, &stat, 0) == -1 || !WIFEXITED(stat) || WEXITSTATUS(stat) != 0 ) {
            childmsg = "Failed: unpack process";
        }
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
        exit(1);
    }

    if ( childmsg || msg ) {
        fatal();
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
    }

    char* msg0 = "";
    if ( msg ) {
        msg0 = msg;
    } else if ( msgx ) {
        msg0 = msgx;
    }

    if ( !childmsg ) {
        childmsg = "";
    }
    char* e0 = NULL;
    asprintf(&e0, "Couldn't start installer ...\n%s\n%s\n%s", emsg, msg0, childmsg);

    pid_t px;
    px = fork();
    if (px < 0) {
        // It really shouldn't land here, but just to have it covered ...
        fprintf(stderr, "%s\n\nInternal error: fork failed.\n", e0);
    }

    // Parent process
    else if (px > 0) {
        // Wait for child to finish
        int stat;
        if ( waitpid(px, &stat, 0) == -1 || !WIFEXITED(stat) || WEXITSTATUS(stat) != 0 ) {
            fprintf(stderr, "%s\n", e0);
        }
    }

    // Child process
    else {
        execlp("kdialog", "kdialog", "--title", "AppIn", "--error", e0, (char*)NULL);
        execlp("zenity", "zenity", "--error", "--title", "AppIn", "--text", e0, (char*)NULL);
        execlp("notify-send", "notify-send", "AppIn", e0, "-i", "error", (char*)NULL);
        // Unfortunately no functioning GUI reporter ...
        exit(1);
    }

    free(e0);
    if ( msgx ) {
        free(msgx);
    }
    tidy();
    exit(1);
}

int del_item(
    const char *path,
    const struct stat *statbuf,
    int type,
    struct FTW *ftwb)
{
    return remove(path);
}

void tidy()
{
    // Tidy up: remove temporary directory
    if ( tmpdir ) {
        if ( nftw(tmpdir, del_item, 20, FTW_DEPTH|FTW_MOUNT|FTW_PHYS) < 0 ) {
            const char* msg0 = "Failed: remove temporary directory, %s";
            asprintf(&msgx, msg0, tmpdir);
            free (tmpdir);
            tmpdir = NULL;
            fatal();
        }
        free (tmpdir);
    }
}
