// Based on example from:
//    https://learn.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes

#include <windows.h>
#include <tlhelp32.h>
#include <wchar.h>
#include <stdio.h>

//  Forward declarations:
BOOL GetProcessList( );
BOOL ListProcessModules( DWORD dwPID );

int main( void )
{
    GetProcessList( );
    return 0;
}

BOOL GetProcessList( )
{
    HANDLE hProcessSnap;
    HANDLE hProcess;
    PROCESSENTRY32W pe32;
    DWORD dwPriorityClass;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        return( FALSE );
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( PROCESSENTRY32W );

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if( !Process32FirstW( hProcessSnap, &pe32 ) )
    {
        CloseHandle( hProcessSnap );          // clean the snapshot object
        return( FALSE );
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do
    {
        wprintf( L"\nPROCESS NAME:  %ls", pe32.szExeFile );

        // List the modules associated with this process
        ListProcessModules( pe32.th32ProcessID );

    } while( Process32NextW( hProcessSnap, &pe32 ) );

    CloseHandle( hProcessSnap );
    return( TRUE );
}

BOOL ListProcessModules( DWORD dwPID )
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32W me32;

    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
    if( hModuleSnap == INVALID_HANDLE_VALUE )
    {
        return( FALSE );
    }

    // Set the size of the structure before using it.
    me32.dwSize = sizeof( MODULEENTRY32W );

    // Retrieve information about the first module,
    // and exit if unsuccessful
    if( !Module32FirstW( hModuleSnap, &me32 ) )
    {
        CloseHandle( hModuleSnap );           // clean the snapshot object
        return( FALSE );
    }

    // Now walk the module list of the process,
    // and display information about each module
    do
    {
        //_tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
        wprintf( L"\n     Executable     = %ls",     me32.szExePath );
        //_tprintf( TEXT("\n     Process ID     = 0x%08X"),         me32.th32ProcessID );
        //_tprintf( TEXT("\n     Ref count (g)  = 0x%04X"),     me32.GlblcntUsage );
        //_tprintf( TEXT("\n     Ref count (p)  = 0x%04X"),     me32.ProccntUsage );
        //_tprintf( TEXT("\n     Base address   = 0x%08X"), (DWORD) me32.modBaseAddr );
        //_tprintf( TEXT("\n     Base size      = %d"),             me32.modBaseSize );

    } while( Module32NextW( hModuleSnap, &me32 ) );

    CloseHandle( hModuleSnap );
    return( TRUE );
}
