// Based on:
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include "incbin.h"

INCBIN(App, "app_bundle.tar.xz");

char* tmpdir;

void error(char *s)
{
  perror(s);
  free (tmpdir);
  exit(1);
}

/******************************
// Source - https://stackoverflow.com/a/2256974
// Posted by asveikau, modified by community. See post 'Timeline' for change history
// Retrieved 2026-10-15, License - CC BY-SA 4.0

int remove_directory(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;

        r = 0;
        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            // Skip the names "." and ".." as we don't want to recurse on them.
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);

    return r;
}
******************************/


int main(int argc, char **argv)
{
    // Note, the compiler/linker may issue a security warning, e.g. GCC:
    // "warning: the use of `tmpnam' is dangerous, better use `mkstemp'"
    tmpdir = tempnam(NULL, "AppInstaller");
    printf("temporary file name: %s\n", tmpdir);
    if (mkdir(tmpdir, 0700) != 0) {
        fprintf(stderr, "Failed to create temporary directory: %s\n", tmpdir);
        error("");
    }
    //printf("temporary file name: %s\n", P_tmpdir);

    printf("%p %u %p\n", &gAppData[0], gAppSize, &gAppEnd);
    printf("%p\n", &gAppData[gAppSize]);
    assert(&gAppData[gAppSize] == (const unsigned char*) &gAppEnd);

    // A pipe to send data to the child process
    // In a pipe, xx[0] is for reading, xx[1] is for writing
    int fd1[2];
    if (pipe(fd1) == -1) {
        error("Pipe Failed");
    }

    pid_t p;
    p = fork();
    if (p < 0) {
        error("Fork Failed");
    }

    // Parent process
    else if (p > 0) {
        printf("Spawned 'tar -xZ' as a child process at pid %d\n", p);

        close(fd1[0]); // Close reading end of pipe

        // Write input string and close writing end of first pipe.
        write(fd1[1], &gAppData[0], gAppSize);
        close(fd1[1]);

        //TODO? Wait for child to send a string
        wait(NULL);



    }

    // Child process
    else {
        // Close stdin
        close(0);
        // Make the pipe the new stdin
        dup2(fd1[0],0);

        close(fd1[1]);

        // Over-write the child process with the 'tar' binary
        execlp("tar", "tar", "-xJ", "-C", tmpdir, (char*)NULL);
        error("Could not run 'tar'");
    }

    //TODO: Run installer ...
    system("sleep 5");

    int count;
    char *cmd = NULL;
    const char *cmd0 = "rm -r \"%s\"";
    count = asprintf(&cmd, cmd0, tmpdir);
    printf("The length of the following string will be %i.\n", count);
    printf("%s\n", cmd);
    int sysrc = system(cmd);
    printf("System -> %d\n", sysrc);
    if ( sysrc != 0 ) {
        fprintf(stderr, "Failed to remove temporary directory: %s\n", tmpdir);
        if ( sysrc == -1 ) error("");
    }
    free( cmd );

    free (tmpdir);

    return 0;
}
