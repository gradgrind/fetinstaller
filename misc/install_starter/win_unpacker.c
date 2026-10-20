// Uses information from:
// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
// Uses: https://github.com/graphitemaster/incbin
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include "incbin.h"

//INCBIN(App, "app_bundle.tar.xz");
INCBIN(App, "app_bundle.tar.xz");
char* szCmdline = "tar -xJ";

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
PROCESS_INFORMATION piProcInfo;

void CreateChildProcess(void);
void WriteToPipe(void);
void ErrorExit(PCTSTR);

int main(int argc, char *argv[])
{
    // On older machines the built-in "tar" might not support xz archives.
    // In that case it is necessary to install bsdtar. This can be done by placing
    // the executable "bsdtar.exe" in the user's "AppData\Local\Microsoft\WindowsApps",
    // which should already be in the PATH.
    _flushall();
    if ( system("where bsdtar >nul 2>nul") == 0 ) {
        szCmdline = "bsdtar -xJ";
    }

    SECURITY_ATTRIBUTES saAttr;

   printf("\n->Start of parent execution.\n");

// Set the bInheritHandle flag so pipe handles are inherited.

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

// Create a pipe for the child process's STDIN.

   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
      ErrorExit("Stdin CreatePipe");

// Ensure the write handle to the pipe for STDIN is not inherited.

   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit("Stdin SetHandleInformation");

// Create the child process.

   CreateChildProcess();

// Write to the pipe that is the standard input for a child process.
// Data is written to the pipe's buffers, so it is not necessary to wait
// until the child process is running before writing data.

   WriteToPipe();

    // Wait until child process exits.
    WaitForSingleObject( piProcInfo.hProcess, INFINITE );

    DWORD child_cc;
    if ( !GetExitCodeProcess(piProcInfo.hProcess, &child_cc) ) {
        ErrorExit("CreateProcess");
    }
    printf("CHILD cc: %d\n", child_cc);
   
    printf("\n");
    printf("Archive written to child STDIN pipe.\n");

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

   printf("\n->End of parent execution.\n");

// Any remaining open handles are cleaned up when this process terminates.
// To avoid resource leaks in a larger application, close all handles explicitly.

   return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
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
        szCmdline,     // command line
        NULL,          // process security attributes
        NULL,          // primary thread security attributes
        TRUE,          // handles are inherited
        0,             // creation flags
        NULL,          // use parent's environment
        NULL,          // use parent's current directory
        &siStartInfo,  // STARTUPINFO pointer
        &piProcInfo    // receives PROCESS_INFORMATION
    ) ) {
        // If an error occurs, exit the application.
        ErrorExit("CreateProcess");
    }

    // Close handles to the stdin pipe, no longer needed by the child process.
    // If not explicitly closed, there is no way to recognize that the
    // child process has ended.

    CloseHandle(g_hChildStd_IN_Rd);
}

void WriteToPipe(void)

// Write the archive to the pipe for the child's STDIN.
// Stop when there is no more data.
{
    DWORD bytesWritten;
    BOOL bSuccess = FALSE;

    printf("%p %u %p\n", &gAppData[0], gAppSize, &gAppEnd);
    printf("%p\n", &gAppData[gAppSize]);

   if ( !WriteFile(
        g_hChildStd_IN_Wr,
        &gAppData[0],
        gAppSize,
        &bytesWritten,
        NULL) ) {
            ErrorExit("WriteFile: Pass archive to unpacker subprocess");
    }

// Close the pipe handle so the child process stops reading.
    if ( ! CloseHandle(g_hChildStd_IN_Wr) )
        ErrorExit("StdInWr CloseHandle");
}

void ErrorExit(PCTSTR lpszFunction)

// Format a readable error message, display a message box,
// and exit from the application.
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(char));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(char),
        "%s failed with error %d: %s",
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, "Error", MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}
