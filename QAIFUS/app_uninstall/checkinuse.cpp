/* Check whether a process is running from a given directory.
 * This can be helpful to warn when trying to uninstall packages which are curently in use.
 */

#include "checkinuse.h"

#if defined Q_OS_WIN

// Based on example from:
//    https://learn.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes

#include <windows.h>
#include <tlhelp32.h>
#include <wchar.h>

//  Forward declarations:
BOOL GetProcessList();
BOOL ListProcessModules(DWORD dwPID);

uint dirInUse(QDir dir, qint64 pid0)
{
    //QString dirpath{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Programs")};
    // pid0 is not used here, any process using the directory must be found.

    HANDLE hProcessSnap;
    HANDLE hProcess;
    PROCESSENTRY32W pe32;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if( hProcessSnap == INVALID_HANDLE_VALUE ) {
        return 0;
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if( !Process32FirstW(hProcessSnap, &pe32) ) {
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return 0;
    }

    // Now walk the snapshot of processes, and check each process in turn
    DWORD pid = 0;
    do {
        //wprintf( L"\nPROCESS NAME:  %ls", pe32.szExeFile );

        // Go through the modules associated with this process
        DWORD dwPID = pe32.th32ProcessID;
        HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
        MODULEENTRY32W me32;

        // Take a snapshot of all modules in the specified process.
        hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
        if ( hModuleSnap == INVALID_HANDLE_VALUE ) {
            continue;
        }

        // Set the size of the structure before using it.
        me32.dwSize = sizeof(MODULEENTRY32W);

        // Retrieve information about the first module,
        // and exit if unsuccessful
        if ( !Module32FirstW(hModuleSnap, &me32) )
        {
            CloseHandle(hModuleSnap);           // clean the snapshot object
            continue;
        }

        // Now walk the module list of the process, looking for file within `dir`
        do {
            QString f{QDir::fromNativeSeparators(QString::fromWCharArray(me32.szExePath))};
            QString fr{dir.relativeFilePath(f)};
            if ( !fr.startsWith("..") && dir.isRelativePath(fr) ) {
                pid = dwPID;
                break;
            }
        } while( Module32NextW(hModuleSnap, &me32) );

        CloseHandle(hModuleSnap);
        if ( pid != 0 )
            break;
    } while ( Process32NextW(hProcessSnap, &pe32) );

    CloseHandle(hProcessSnap);
    return pid;
}

#elif defined Q_OS_LINUX

uint dirInUse(QDir dir, qint64 pid0)
{
    //QString dirpath{QDir::home().absoluteFilePath(".local/apps")};
    QDirListing dirList("/proc/", QDirListing::IteratorFlag::DirsOnly);
    for (const auto &dirEntry : dirList) {
        QDir pdir{dirEntry.absoluteFilePath()};
        QDirListing mapfiles(
            pdir.absoluteFilePath("map_files"),
            QDirListing::IteratorFlag::ResolveSymlinks | QDirListing::IteratorFlag::IncludeBrokenSymlinks);
        for (const auto &link : mapfiles) {
            QString f{link.fileInfo().symLinkTarget()};
            QString fr{dir.relativeFilePath(f)};
            if ( !fr.startsWith("..") && dir.isRelativePath(fr) ) {
                bool ok;
                qint64 pid = dirEntry.fileName().toLongLong(&ok);
                if ( ok && pid != pid0 )
                    return pid;
            }
        }
    }
    return 0;
}

#else

//TODO ...

#endif
