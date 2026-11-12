// Windows version ...
// This uses the example from
//   https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
// as a basis for unpacking the archive by sending it to stdin of a child
// process running the tar command.

// To suppress console window, compile with:
//   gcc -mwindows -o appinw appinw.c

// The various versions of "%s" in format strings are more than confusing.
// It also seems rather inconsistent. I hope my using "%ls" here handles
// the situation(s) correctly. I understand that Microsoft were in a difficult
// position historically concerning these developments, but it is not nice.
// The price of backward compatibility is high here.

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <windows.h>
#include <shellapi.h>
#include <errno.h>
#include <process.h>
#include <direct.h>
#include "incbin.h"

// For console output mode
#include <fcntl.h>
#include <io.h>

INCBIN(App, "../build/app_bundle.tar.xz"); // apparently INCBIN needs UNIX separators

wchar_t* compose(wchar_t* fmt, ...);
void error(wchar_t* msg);
int unpack(wchar_t* dir);
DWORD xsystem(wchar_t* cmd);
int CreateChildProcess(wchar_t* cmdline);
int WriteToPipe();

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
PROCESS_INFORMATION piProcInfo;

int main(void)
{
    // Set console mode to UTF-16 wide characters.
    // This is only relevant when compiled for console mode.
    {
        int fn = _fileno(stdout);
        if ( fn >= 0 )
            _setmode(_fileno(stdout), _O_U16TEXT);
    }

    wchar_t* msgx;
    // Get a suitable base path for the temporary directory.
    wchar_t* tmp0 = _wgetenv(L"TMP");
    if ( !tmp0 )
        tmp0 = _wgetenv(L"TEMP");
    if ( !tmp0 ) {
        error(L"Temporary folder not found.");
        return 1;
    }

/* Not available with mingw ...
    size_t len0;
    wchar_t* tmp0;
    errno_t err = _wdupenv_s(&tmp0, &len0, L"TMP");
    if ( !err ) {
        if ( tmp0 ) goto tmpok;
        err = _wdupenv_s(&tmp0, &len0, L"TEMP");
        if ( !err && tmp0 ) goto tmpok;
    }
    //wprintf(L"no TEMP\n");
    error(L"Temporary folder not found.");
    return 1;
tmpok:
*/

    msgx = compose(L"Temporary directory: %ls\n", tmp0);

    // Check that there are no quotation marks in tmp0.
    if ( wcsstr(tmp0, L"\"") ) {
        msgx = compose(L"Invalid temporary directory: %ls\n", tmp0);
        error(msgx);
        free(msgx);
        return 1;
    }

    // Create the temporary directory.
    wchar_t* tmpdir = compose(L"%ls\\gradgrind_AppIn", tmp0);
    if ( _wmkdir(tmpdir) != 0 ) {
        free(tmpdir);
        tmpdir = compose(L"%ls\\gradgrind_AppIn_", tmp0);
        if ( _wmkdir(tmpdir) != 0 ) {
            wchar_t* msgx = compose(L"Temporary directory unavailable: %ls\n", tmpdir);
            free(tmpdir);
            tmpdir = NULL;
            error(msgx);
            free(msgx);
            return 1;
        }
        // In this case, app_install should comment on this, actually
        // there should be only one instance per user.
    }

    // Unpack the app bundle.
    int rc = unpack(tmpdir);
    if ( rc == 0 ) {
        // Run the installer.
        wchar_t* cmd = compose(L"\"%ls\\app_install.exe\"", tmpdir);
        _flushall();
        DWORD xrc = xsystem(cmd);
        if ( xrc != 0 ) {
            rc = 1;
            wchar_t* msg = compose(L"Installation failed: %ls", cmd);
            error(msg);
            free(msg);
        }
        free(cmd);
    }

    // Remove the temporary directory (nftw is not available on Windows).
    // The path must be double-0 terminated ...
    size_t len = wcslen(tmpdir) + 2;
    wchar_t* buf = malloc(sizeof(wchar_t) * len);
    //memcpy(buf, tmpdir, sizeof(wchar_t) * (len - 1));
    wmemcpy_s(buf, len, tmpdir, len - 1);
    buf[len - 1] = 0;
    SHFILEOPSTRUCTW shfo = {
        NULL,
        FO_DELETE,
        buf, // double-0 terminated!
        NULL,
        FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION,
        FALSE,
        NULL,
        NULL };
    SHFileOperationW(&shfo);
    if ( _waccess(tmpdir, 0) == 0 ) {
    //int delrc = SHFileOperationW(&shfo);
    //if ( delrc != 0 || shfo.fAnyOperationsAborted ) {
        rc = 1;
        wchar_t* msg = compose(L"Failed to remove temporary directory: %ls", tmpdir);
        error(msg);
        free(msg);
    }
    free(tmpdir);
    ExitProcess(rc);
}

int unpack(wchar_t* dir)
{
    // On older Windows versions the built-in "tar" might not support
    // xz archives. In that case it is necessary to install bsdtar.
    // This can be done by placing the executable "bsdtar.exe" in the
    // user's "AppData\Local\Microsoft\WindowsApps", which should already
    // be in the PATH.
    wchar_t* cmd0 = L"%ls -xJf - -C \"%ls\"";
    _flushall();
    wchar_t* tar = L"tar";
    // The command to CreateProcessW, here in xsystem, may not be a constant (see docs).
    wchar_t* bsdtar = compose(L"where bsdtar");
    if ( xsystem(bsdtar) == 0 ) {
        tar = L"bsdtar";
    }

    SECURITY_ATTRIBUTES saAttr;
    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDIN.
    if ( !CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0) ) {
        error(L"Failed: Stdin CreatePipe");
        return 1;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if ( !SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) ) {
        error(L"Failed: Stdin SetHandleInformation");
        return 2;
    }

    // Create the child process.
    wchar_t* cmd = compose(cmd0, tar, dir);
    if ( CreateChildProcess(cmd) != 0 ) {
        wchar_t* msg = compose(L"Failed: Create child process (%ls)", cmd);
        error(msg);
        free(msg);
        free(cmd);
        return 3;
    }

    // Write to the pipe that is the standard input for a child process.
    // Data is written to the pipe's buffers, so it is not necessary to wait
    // until the child process is running before writing data.
    int rc = WriteToPipe();
    if ( rc != 0 )
        return rc;

    // Wait until child process exits.
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    DWORD child_cc;
    if ( !GetExitCodeProcess(piProcInfo.hProcess, &child_cc) ) {
        error(L"Failed: unpacking the app bundle");
        free(cmd);
        return 4;
    }
    //wprintf(L"CHILD cc: %d\n", child_cc);
    //wprintf(L"Archive written to child STDIN pipe.\n");

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    //wprintf(L"\n->End of parent execution.\n");
    free(cmd);
    return 0;
}

int CreateChildProcess(wchar_t* cmdline)
{
    // Create a child process that uses the previously created pipes for STDIN and STDOUT.
    STARTUPINFOW siStartInfo;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

    // Set up members of the STARTUPINFOW structure.
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFOW) );
    siStartInfo.cb = sizeof(STARTUPINFOW);
    siStartInfo.hStdError = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.
    if ( !CreateProcessW(NULL,
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

int WriteToPipe()
{
    // Write the archive to the pipe for the child's STDIN.
    // Stop when there is no more data.
    DWORD bytesWritten;
    //wprintf(L"%p %u %p\n", &gAppData[0], gAppSize, &gAppEnd);
    //wprintf(L"%p\n", &gAppData[gAppSize]);
    BOOL bSuccess = WriteFile(
        g_hChildStd_IN_Wr,
        &gAppData[0],
        gAppSize,
        &bytesWritten,
        NULL);
    BOOL cSuccess = CloseHandle(g_hChildStd_IN_Wr);
    if ( !bSuccess || bytesWritten != gAppSize ) {
        error(L"Failed: Pass archive to unpacker subprocess");
        return -2;
    }
    if ( !cSuccess ) {
        error(L"Failed: StdInWr CloseHandle");
        return -3;
    }
    return 0;
}

// Variadic function to compose strings.
// This works like printf, but returns a string – whose memory must be freed.
wchar_t* compose(wchar_t* fmt, ...)
{
    int count;
    int size;
    wchar_t* cmps = NULL;
    va_list args;
    va_start(args, fmt);
    count = _vscwprintf(fmt, args);
    size = count + 1; // the string will need one more char for the null terminator
    cmps = malloc(sizeof(wchar_t) * size);
    vswprintf_s(cmps, size, fmt, args);
    va_end(args);
    return cmps;
    //free(cmps);
}

void error(wchar_t* msg)
{
    wchar_t* msg0 = msg;
    wchar_t* msg2 = NULL;
    if (errno != 0) {
        const wchar_t* emsg = _wcserror(errno);
        msg2 = compose(L"*** %ls ***\n%ls", emsg, msg);
        msg0 = msg2;
    }
    wprintf(L"%ls\n", msg0);
    MessageBoxW(
        0,          /*   [in, optional] HWND    hWnd,      */
        msg0,       /*   [in, optional] LPCWSTR lpText,    */
        L"AppIn",   /*   [in, optional] LPCWSTR lpCaption, */
        //MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
        MB_ICONERROR);
    free(msg2);
}

DWORD xsystem(wchar_t* cmd)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process.
    if ( !CreateProcessW(
            NULL,               // No module name (use command line)
            cmd,                // Command line
            NULL,               // Process handle not inheritable
            NULL,               // Thread handle not inheritable
            FALSE,              // Set handle inheritance to FALSE
            CREATE_NO_WINDOW,   // Creation flags
            NULL,               // Use parent's environment block
            NULL,               // Use parent's starting directory
            &si,                // Pointer to STARTUPINFO structure
            &pi)                // Pointer to PROCESS_INFORMATION structure
        ) {
        wprintf(L"CreateProcess failed (%ls).\n", cmd);
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
