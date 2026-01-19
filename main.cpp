//
// PowershellStub: a very small EXE that just launches Windows PowerShell (passing
// arguments along).
//

#include <Windows.h>

inline char8_t* SkipTillAfterQuote( char8_t* str )
{
    while( *str &&
           (*str != '"') )
    {
        str++;
    }

    return ++str;
} // end SkipTillAfterQuote()

inline char8_t* SkipUntilWhitespace( char8_t* str )
{
    while( *str &&
           ((*str != ' ') && (*str != '\t')) )
    {
        str++;
    }

    return str;
} // end SkipUntilWhitespace()

inline char8_t* SkipUntilNotWhitespace( char8_t* str )
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
inline char8_t* FindTheRestOfTheCommandLine()
{
    char8_t* str = (char8_t*) GetCommandLineA();

    if( str[ 0 ] == '"' )
    {
        str = SkipTillAfterQuote( &str[ 1 ] );
    }
    else
    {
        str = SkipUntilWhitespace( str );
    }

    return str;
} // end FindTheRestOfTheCommandLine()

inline char8_t* CopyStr( char8_t* dest, const char8_t* src, const char8_t* pastDestEnd )
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

// TODO: does this really evaluate at compile-time, or do I need to switch to a recursive
// method?
constexpr DWORD MyWcslen( const char8_t* s )
{
    DWORD cch = 0;
    while( *s )
    {
        cch++;
        s++;
    }
    return cch;
}

void Print( const char8_t* s )
{
    WriteConsoleA( g_hStdOut, s, MyWcslen( s ), nullptr, nullptr );
} // end Print()

int MyStrCmpI( const char8_t* s1, const char8_t* s2 )
{
    return lstrcmpiA( (const char*) s1, (const char*) s2 );
}

bool _HandleUsageRequest( const char8_t* installMode )
{
    const char8_t c_Help1[] = u8"help";
    const char8_t c_Help2[] = u8"/?";
    const char8_t c_Help3[] = u8"-?";

    if( (0 == MyStrCmpI( installMode, c_Help1 )) ||
        (0 == MyStrCmpI( installMode, c_Help2 )) ||
        (0 == MyStrCmpI( installMode, c_Help3 )) )
    {
        Print( u8"\nUsage: PowershellStub.exe [WhatIf] <InstallMode> <ScriptURL> <ExpectedScriptHash> [<OptionalArgs>]\n"
               u8"\n"
               u8"This program is intended to be used as an \"installer\" EXE for winget, where the actual\n"
               u8"install logic is implemented in a downloadable PowerShell script. It launches powershell.exe\n"
               u8"with a command line that downloads the script, verifies its hash, verifies its signature\n"
               u8"(if any) and then runs it.\n"
               u8"\n"
               u8"  WhatIf: (optional) if specified, just prints the command line that would be used.\n"
               u8"\n"
               u8"  InstallMode: one of Interactive, Silent, or SilentWithProgress.\n"
               u8"\n"
               u8"  ScriptURL: URL from which to download the PowerShell script to run.\n"
               u8"\n"
               u8"  ExpectedScriptHash: SHA256 hash of the script, in hex.\n"
               u8"\n"
               u8"  OptionalArgs: optional arguments to pass along to the script.\n\n"
             );
        return true;
    }
    return false;
} // end _HandleUsageRequest()

int main() // no C runtime, so no args here
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    g_hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );

    char8_t* commandLineArgs = FindTheRestOfTheCommandLine();

    char8_t* cursor = commandLineArgs;

#define EXPECT_MORE_CMDLINE \
    if( !*cursor ) \
    { \
        Print( u8"Expected more on the command line." ); \
        ExitProcess( (UINT) -1 ); \
    } \

    EXPECT_MORE_CMDLINE

    cursor = SkipUntilWhitespace( cursor );

    EXPECT_MORE_CMDLINE

    *cursor++ = '\0';
    cursor = SkipUntilNotWhitespace( cursor );

    char8_t* installMode = cursor;

    if( _HandleUsageRequest( installMode ) )
    {
        ExitProcess( (UINT) -2 );
    }

    bool whatIf = false;

    cursor = SkipUntilWhitespace( cursor );

    EXPECT_MORE_CMDLINE

    *cursor++ = '\0';

    if( 0 == MyStrCmpI( installMode, u8"WhatIf" ) )
    {
        whatIf = true;

        cursor = SkipUntilNotWhitespace( cursor );

        EXPECT_MORE_CMDLINE

        installMode = cursor;

        cursor = SkipUntilWhitespace( cursor );

        EXPECT_MORE_CMDLINE

        *cursor++ = '\0';
        cursor = SkipUntilNotWhitespace( cursor );

        EXPECT_MORE_CMDLINE
    }

    cursor = SkipUntilNotWhitespace( cursor );

    char8_t* scriptUrl = cursor;

    cursor = SkipUntilWhitespace( cursor );

    EXPECT_MORE_CMDLINE

    *cursor++ = '\0';
    cursor = SkipUntilNotWhitespace( cursor );

    char8_t* expectedScriptHash = cursor;

    // *Optional* arguments.
    cursor = SkipUntilWhitespace( cursor );

    if( *cursor )
    {
        *cursor++ = '\0';
        cursor = SkipUntilNotWhitespace( cursor );
    }

    char8_t* optionalArgs = cursor;

    const char8_t c_Silent[] = u8"Silent";
    const char8_t c_SilentWithProgress[] = u8"SilentWithProgress";
    const char8_t c_Interactive[] = u8"Interactive";

    bool bIsSilent = 0 == MyStrCmpI( installMode, c_Silent );
    bool bIsSilentWithProgress = 0 == MyStrCmpI( installMode, c_SilentWithProgress );
    bool bIsInteractive = 0 == MyStrCmpI( installMode, c_Interactive );

    if( !bIsSilent && !bIsSilentWithProgress && !bIsInteractive )
    {
        Print( u8"Install mode must be one of Silent, SilentWithProgress, or Interactive. Not '" );
        Print( installMode );
        Print( u8"'.\n" );
        ExitProcess( (UINT) -3 );
    }

    char8_t newCmdLine[ 4096 ];
    char8_t* dst = newCmdLine;
    const char8_t* pastEnd = &newCmdLine[ _countof( newCmdLine ) ];

    const char8_t cmdFrag0[] = u8" -NoProfile -ExecutionPolicy RemoteSigned -Command $env:PSModulePath = $null ; "
                               u8"Set-StrictMode -Version Latest ; "
                               u8"try { "
                                   u8"$theScript = (iwr ";

    const char8_t cmdFrag1[] =     u8"$env:_POWERSHELL_STUB_TEST_URL_SUFFIX -UseBasic -EA Stop).Content ; "
                                   u8"$bytes = [System.Text.Encoding]::Unicode.GetBytes( $theScript ) ; "
                                   u8"$hash = ([Security.Cryptography.SHA256]::Create().ComputeHash( $bytes ) | %{ $_.ToString('x2') }) -join '' ; "
                                   u8"$expectedHash = '";

    const char8_t cmdFrag2[] =     u8"' ; "
                                   u8"if( $hash -ne $expectedHash ) "
                                   u8"{ "
                                       u8"throw \\\"Hash mismatch: expected $expectedHash but got $hash\\\" "
                                   u8"} ; "
                                   u8"$sig = Get-AuthenticodeSignature -Source 'theScript.ps1' -Content $bytes ; "
                                   u8"if( ($sig.Status -ne 'Valid') -and ($sig.Status -ne 'NotSigned') ) "
                                   u8"{ "
                                       u8"throw \\\"Signature error: $($sig.Status)\\\" "
                                   u8"} ; "
                                   u8"Invoke-Expression \\\". { $theScript } -";

    const char8_t cmdFrag3[] =     u8" -EA Stop\\\" ; "
                               u8"} catch { ";

    const char8_t cmdFrag4_Interactive[] =
                                   u8"Write-Host $_ -Fore Red ; "
                                   u8"$rsp = Read-Host 'pausing... press [enter] to finish' ; "
                                   u8"if( $rsp -eq 'd' ) { $host.EnterNestedPrompt() } ; "
                                   u8"throw "
                               u8"}";

    const char8_t cmdFrag4_Silent[] =
                                   u8"Write-Host $_ -Fore Red ; "
                                   u8"Start-Sleep -Seconds 5 ; " // Give people a chance to at least see the error.
                                   u8"throw"
                               u8"}";

    dst = CopyStr( dst, cmdFrag0, pastEnd );
    dst = CopyStr( dst, scriptUrl, pastEnd );
    dst = CopyStr( dst, cmdFrag1, pastEnd );
    dst = CopyStr( dst, expectedScriptHash, pastEnd );
    dst = CopyStr( dst, cmdFrag2, pastEnd );
    dst = CopyStr( dst, installMode, pastEnd );
    dst = CopyStr( dst, u8" ", pastEnd);
    dst = CopyStr( dst, optionalArgs, pastEnd);
    dst = CopyStr( dst, cmdFrag3, pastEnd );

    if( bIsInteractive )
    {
        dst = CopyStr( dst, cmdFrag4_Interactive, pastEnd );
    }
    else
    {
        dst = CopyStr( dst, cmdFrag4_Silent, pastEnd );
    }

    if( whatIf )
    {
        Print( u8"\nWould run powershell.exe with command line:\n\n   " );
        Print( newCmdLine );
        Print( u8"\n\n" );
        ExitProcess( (UINT) -4 );
    }

    if( dst == (pastEnd - 1) )
    {
        Print( u8"Command line too long. Use WhatIf to see it.\n" );
        ExitProcess( (UINT) -5 );
    }

    PROCESS_INFORMATION pi = { };
    STARTUPINFOA si;
    SecureZeroMemory( &si, sizeof( si ) );
    si.cb = (DWORD) sizeof( si );

    const char* szPowershellExe = "C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe";

    BOOL bItWorked = CreateProcessA( szPowershellExe,
                                     (char*) newCmdLine,
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
    }

    // Note that we use ExitProcess to end execution instead of just "return dwRet",
    // because returning from main leads to running RtlExitUserThread, and when running on
    // an ARM64 machine, that was failing in a bizarre way (it appeared that the return
    // code was not being propagated--one speculation was that it might be returning the
    // thread's exit code instead of main's--and when running under a debugger to
    // investigate, the debugger would get stuck and you couldn't break in).
    ExitProcess( dwRet );
}
