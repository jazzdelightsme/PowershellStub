//
// PowershellStub: a very small EXE that just launches Windows PowerShell (passing
// arguments along).
//

#include <Windows.h>

inline wchar_t* SkipTillAfterQuote( wchar_t* str )
{
    while( *str &&
           (*str != L'"') )
    {
        str++;
    }

    return ++str;
} // end SkipTillAfterQuote()

inline wchar_t* SkipUntilWhitespace( wchar_t* str )
{
    while( *str &&
           ((*str != ' ') && (*str != '\t')) )
    {
        str++;
    }

    return str;
} // end SkipUntilWhitespace()

// Returns a pointer to the part of the current process command line immediately after the
// EXE.
inline wchar_t* FindTheRestOfTheCommandLine()
{
    wchar_t* returnMe = NULL;
    wchar_t* str = GetCommandLine();

    if( str[ 0 ] == L'"' )
    {
        str = SkipTillAfterQuote( &str[ 1 ] );
    }
    else
    {
        str = SkipUntilWhitespace( str );
    }

    return str;
} // end GetFirstArg()

int main() // no C runtime, so no args here
{
    wchar_t* commandLineArgs = FindTheRestOfTheCommandLine();

    PROCESS_INFORMATION pi = { };
    STARTUPINFOW si;
    SecureZeroMemory( &si, sizeof( si ) ); // compiler complained about no memset; whatevs
    si.cb = (DWORD) sizeof( si );

    const wchar_t* wszPowershellExe = L"C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe";

    BOOL bItWorked = CreateProcessW( wszPowershellExe,
                                     commandLineArgs,
                                     nullptr,   // lpProcessAttributes
                                     nullptr,   // lpThreadAttributes
                                     TRUE,      // bInheritHandles
                                     0,         // dwFlags
                                     nullptr,   // lpEnvironment
                                     nullptr,   // lpCurrentDirectory
                                     &si,
                                     &pi );

    if( !bItWorked )
    {
        ExitProcess( GetLastError() );
        return -1; // unreachable
    }

    // Ignoring result...
    WaitForSingleObject( pi.hProcess, INFINITE );

    DWORD dwRet = 0;
    bItWorked = GetExitCodeProcess( pi.hProcess, &dwRet );

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    if( !bItWorked )
    {
        ExitProcess( GetLastError() );
        return -1; // unreachable
    }

    // Note that we use ExitProcess to end execution instead of just "return dwRet",
    // because returning from main leads to running RtlExitUserThread, and when running on
    // an ARM64 machine, that was failing in a bizarre way (it appeared that the return
    // code was not being propagated--one speculation was that it might be returning the
    // thread's exit code instead of main's--and when running under a debugger to
    // investigate, the debugger would get stuck and you couldn't break in).
    ExitProcess( dwRet );
    return -1; // unreachable
}
