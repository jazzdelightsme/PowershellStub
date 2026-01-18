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

inline wchar_t* SkipUntilNotWhitespace( wchar_t* str )
{
    while( *str &&
           ((*str == ' ') || (*str == '\t')) )
    {
        str++;
    }

    return str;
} // end SkipUntilNotWhitespace()


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
} // end FindTheRestOfTheCommandLine()

inline wchar_t* CopyStr( wchar_t* dest, const wchar_t* src, const wchar_t* pastDestEnd )
{
    // The -1 is to leave room for the terminating null.
    while( (dest < (pastDestEnd - 1)) && *src )
    {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = 0;
    return dest;
} // end CopyStr()

HANDLE g_hStdOut = 0;

#define _countof(_Array) ((sizeof(_Array) / sizeof(_Array[0])))

DWORD MyWcslen( const wchar_t* s )
{
    DWORD cch = 0;
    while( *s )
    {
        cch++;
        s++;
    }
    return cch;
}

void Print( const wchar_t* s )
{
    WriteConsoleW( g_hStdOut, s, MyWcslen( s ), nullptr, nullptr );
} // end Print()

int main() // no C runtime, so no args here
{
    g_hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
    Print( L"Well hello there.\n" );

    wchar_t newCmdLine[ 16384 ];
    wchar_t* dst = newCmdLine;
    const wchar_t* pastEnd = &newCmdLine[ _countof( newCmdLine ) ];

    const wchar_t cmdFrag0[] = L"-NoProfile -ExecutionPolicy RemoteSigned -Command $env:PSModulePath = $null ; "
                               L"try { "
                                   L"$outerArgs = $args ; "
                                   L"$theScript = (iwr ";

    const wchar_t cmdFrag1[] =     L"$env:_POWERSHELL_STUB_TEST_URL_SUFFIX -UseBasic -EA Stop).Content ; "
                                   L"$bytes = [System.Text.Encoding]::Unicode.GetBytes( $theScript ) ; "
                                   L"$hash = ([Security.Cryptography.SHA256]::Create().ComputeHash( $bytes ) | %{ $_.ToString('x2') }) -join '' ; "
                                   L"$expectedHash = '";

    const wchar_t cmdFrag2[] =     L"' ; "
                                   L"if( $hash -ne $expectedHash ) "
                                   L"{ "
                                       L"throw \"Hash mismatch: expected $expectedHash but got $hash\" "
                                   L"} ; "
                                   L"$sig = Get-AuthenticodeSignature -Source 'theScript.ps1' -Content $bytes ; "
                                   L"if( ($sig.Status -ne 'Valid') -and ($sig.Status -ne 'NotSigned') ) "
                                   L"{ "
                                       L"throw \"Signature error: $($sig.Status)\" "
                                   L"} ; "
                                   L"Invoke-Expression \\\". { $theScript } -I  @outerArgs -EA Stop\\\" ; "
                               L"} catch { ";

    dst = CopyStr( dst, L"", pastEnd );

    wchar_t* commandLineArgs = FindTheRestOfTheCommandLine();

    // Expected arguments:
    //
    //  1. InstallMode: one of Interactive, Silent, or SilentWithProgress.
    //  2. Script URL.
    //  3. Expected script hash.
    //
    // TODO: extra args
    //

    wchar_t* cursor = commandLineArgs;

#define EXPECT_MORE_CMDLINE \
    if( !*cursor ) \
    { \
        Print( L"Expected more on the command line." ); \
        ExitProcess( -1 ); \
        return -1; \
    } \

    EXPECT_MORE_CMDLINE

    cursor = SkipUntilWhitespace( cursor );

    EXPECT_MORE_CMDLINE

    *cursor++ = L'\0';
    cursor = SkipUntilNotWhitespace( cursor );

    wchar_t* installMode = cursor;

    cursor = SkipUntilWhitespace( cursor );

    EXPECT_MORE_CMDLINE

    *cursor++ = L'\0';
    cursor = SkipUntilNotWhitespace( cursor );

    wchar_t* scriptUrl = cursor;

    cursor = SkipUntilWhitespace( cursor );

    EXPECT_MORE_CMDLINE

    *cursor++ = L'\0';
    cursor = SkipUntilNotWhitespace( cursor );

    wchar_t* expectedScriptHash = cursor;

    // TODO: optional/extra args



    Print( L"Install mode: " );
    Print( installMode );
    Print( L"\n" );

    Print( L"Script URL: " );
    Print( scriptUrl );
    Print( L"\n" );

    Print( L"Expected script hash: " );
    Print( expectedScriptHash );
    Print( L"\n" );

    ExitProcess( 0 );
    return 0;

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
