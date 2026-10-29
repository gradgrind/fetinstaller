// Windows version ...

// To suppress console window, compile with:
//   gcc -mwindows -o appinw appinw0.c

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>
#include <errno.h>
#include <process.h>
#include <direct.h>
#include "incbin.h"

INCBIN(App, "../build/app_bundle.tar.xz");

char* tmpdir = NULL;
char* compose(char* fmt, ...);
void error(char* msg);
int unpack(char* dir);
int xsystem(char* cmd);

int main(int argc, char **argv)
{
    // Create a temporary directory to receive the unpacked installer.
    // The following code assumes that only one installer should be active
    // at any time (for any one user). If a second installer is started,
    // "app_install" can detect this by means of the trailing "_" on the
    // path name, so it can report the situation, but otherwise be unhindered.
    // A further attempt to start an installer will fail.
    char* tmp0 = getenv("TMP");
    if ( tmp0 == NULL ) {
        error("TMP environment variable not set.");
        return 1;
    }
    tmpdir = compose("%s/gradgrind_AppIn", tmp0);
    if ( _mkdir(tmpdir) != 0 ) {
        free(tmpdir);
        tmpdir = compose("%s/gradgrind_AppIn_", tmp0);
        if ( _mkdir(tmpdir) != 0 ) {
            char* msg = compose("Temporary directory unavailable: '%s'", tmpdir);
            error(msg);
            free(msg);
            free(tmpdir);
            return 1;
        }
        // In this case, app_install should comment on this, actually
        // there should be only one instance per user.
    }

    // Unpack the app bundle.
    if ( unpack(tmpdir) != 0 ) {
        return 1;
    }

    // Run the installer.

    char *cmd = compose("%s\\app_install.exe", tmpdir);
    _flushall();
    int ok = xsystem(cmd);
    if ( ok != 0 ) {
        char* msg = compose("Installation failed: '%s'", cmd);
        error(msg);
        free(msg);
    }
    free(cmd);

    // Remove the temporary directory.
    // The path must be double-0 terminated ...
    size_t len = strlen(tmpdir) + 2;
    char* buf = malloc(sizeof(char) * len);
    memcpy(buf, tmpdir, len - 1);
    buf[len - 1] = 0;
    SHFILEOPSTRUCT shfo = {
        NULL,
        FO_DELETE,
        buf, // double-0 terminated!
        NULL,
        FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION,
        FALSE,
        NULL,
        NULL };
    int delrc = SHFileOperation(&shfo);
    if ( delrc != 0 ) {
        char* msg = compose("Failed: Remove temporary directory '%s'", tmpdir);
        error(msg);
        free(msg);
    }
    free(tmpdir);
    if ( delrc != 0 | ok != 0 ) {
        return 1;
    }
    return 0;
}

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
PROCESS_INFORMATION piProcInfo;

int CreateChildProcess();
void WriteToPipe();

int unpack(char* dir)
{
    // On older Windows versions the built-in "tar" might not support
    // xz archives. In that case it is necessary to install bsdtar.
    // This can be done by placing the executable "bsdtar.exe" in the
    // user's "AppData\Local\Microsoft\WindowsApps", which should already
    // be in the PATH.
    char* cmd0 = "%s -xJf - -C %s";
    _flushall();
    char* tar = "tar";
    if ( xsystem("where bsdtar >nul 2>nul") == 0 ) {
        tar = "bsdtar";
    }

    SECURITY_ATTRIBUTES saAttr;
    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDIN.
    if ( !CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0) ) {
        error("Failed: Stdin CreatePipe");
        return -1;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if ( !SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) ) {
        error("Failed: Stdin SetHandleInformation");
        return -1;
    }

    // Create the child process.
    char* cmd = compose(cmd0, tar, dir);
    if ( CreateChildProcess(cmd) != 0 ) {
        char* msg = compose("Failed: Create child process (%s)", cmd);
        error(msg);
        free(msg);
        free(cmd);
        return -1;
    }

    // Write to the pipe that is the standard input for a child process.
    // Data is written to the pipe's buffers, so it is not necessary to wait
    // until the child process is running before writing data.
    WriteToPipe();

    // Wait until child process exits.
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    DWORD child_cc;
    if ( !GetExitCodeProcess(piProcInfo.hProcess, &child_cc) ) {
        error("Failed: unpacking the app bundle");
        free(cmd);
        return -1;
    }
    //printf("CHILD cc: %d\n", child_cc);
    //printf("Archive written to child STDIN pipe.\n");

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    //printf("\n->End of parent execution.\n");
    free(cmd);
    return 0;
}

int CreateChildProcess(char* cmdline)
{
    // Create a child process that uses the previously created pipes for STDIN and STDOUT.
    STARTUPINFO siStartInfo;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.
    if ( !CreateProcess(NULL,
        cmdline,            // command line
        NULL,               // process security attributes
        NULL,               // primary thread security attributes
        TRUE,               // handles are inherited
        CREATE_NO_WINDOW,   // creation flags
        NULL,               // use parent's environment
        NULL,               // use parent's current directory
        &siStartInfo,       // STARTUPINFO pointer
        &piProcInfo         // receives PROCESS_INFORMATION
    ) ) {
        // If an error occurs, exit the application.
        return -1;
    }

    // Close handles to the stdin pipe, no longer needed by the child process.
    // If not explicitly closed, there is no way to recognize that the
    // child process has ended.
    CloseHandle(g_hChildStd_IN_Rd);
    return 0;
}

void WriteToPipe()
{
    // Write the archive to the pipe for the child's STDIN.
    // Stop when there is no more data.
    DWORD bytesWritten;
    //printf("%p %u %p\n", &gAppData[0], gAppSize, &gAppEnd);
    //printf("%p\n", &gAppData[gAppSize]);
    if ( !WriteFile(
            g_hChildStd_IN_Wr,
            &gAppData[0],
            gAppSize,
            &bytesWritten,
            NULL) ) {
        error("Failed: Pass archive to unpacker subprocess");
        ExitProcess(1);
    }

    // Close the pipe handle so the child process stops reading.
    if ( !CloseHandle(g_hChildStd_IN_Wr) ) {
        error("Failed: StdInWr CloseHandle");
        ExitProcess(1);
    }
}

// Variadic function to compose strings.
// This works like printf, but returns a string – whose memory must be freed.
char* compose(char* fmt, ...)
{
    int count;
    int size;
    char* cmps = NULL;
    va_list args;
    va_start(args, fmt);
    count = _vscprintf(fmt, args);
    size = count + 1; // the string will need one more char for the null terminator
    cmps = malloc(sizeof(char) * size);
    vsprintf_s(cmps, size, fmt, args);
    //printf("%s\n", cmps);
    return cmps;
    //free(cmps);
}

void error(char *msg)
{
    char* msg2 = NULL;
    if ( errno != 0 ) {
        const char* emsg = strerror(errno);
        msg2 = compose("*** %s ***\n%s", emsg, msg);
    }
    MessageBox(
        0,          /*   [in, optional] HWND    hWnd,      */
        msg,        /*   [in, optional] LPCTSTR lpText,    */
        "AppIn",    /*   [in, optional] LPCTSTR lpCaption, */
        //MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
        MB_ICONERROR);
    free(msg2);
}

int xsystem(char* cmd)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

     // Start the child process.
    if( !CreateProcess(
            NULL,               // No module name (use command line)
            cmd,                // Command line
            NULL,               // Process handle not inheritable
            NULL,               // Thread handle not inheritable
            FALSE,              // Set handle inheritance to FALSE
            CREATE_NO_WINDOW,   // No creation flags
            NULL,               // Use parent's environment block
            NULL,               // Use parent's starting directory
            &si,                // Pointer to STARTUPINFO structure
            &pi)                // Pointer to PROCESS_INFORMATION structure
        ) {
        //printf("CreateProcess failed (%s).\n", cmd);
        return -1;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD ec;
    GetExitCodeProcess(pi.hProcess, &ec);

    // Close process and thread handles.
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return ec;
}
