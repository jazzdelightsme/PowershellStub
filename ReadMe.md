# PowershellStub

## What is it?

A ***tiny*** (8k) EXE that launches the Windows built-in `powershell.exe`
(legacy Windows PowerShell 5.1) with a wrapper script that downloads, verifies,
and runs another script, waits for `powershell.exe` to exit, and returns
`powershell.exe`'s return code as its own.

## But... WHY is it?

This is useful as an "installer" EXE for winget, where the actual install logic
is implemented in a (downloadable) PowerShell script.

For a `winget` package, `winget` needs to get something that it can *run*
(executing a `.ps1` just opens it in a text editor), and it really wants to
download that thing from the interwebs. This EXE supplies that.

In general, the policy for `winget` packages is to avoid executing dynamic
content, hence "script installers" are not natively supported. Although this
EXE allows implementing an installer in script, it aims to follow the spirit of
that policy: just like `winget` verifies hashes of EXEs, using
`PowerShellStub.exe` will verify the hash of your installl script (as well as a
signature, if it exists), only executing the script if its hash matches what
was specified on the command line (in your `winget` package manifest).

You are welcome to use it too, if you like (see [the WingetPathUpdater
manifest](https://github.com/jazzdelightsme/WingetPathUpdater/blob/main/manifests/j/jazzdelightsme/WingetPathUpdater/1.3/jazzdelightsme.WingetPathUpdater.installer.yaml)
for example usage).

## Details

Command line syntax:

`PowershellStub.exe [WhatIf|WhatIfRaw] <InstallMode> <ScriptURL> <ExpectedScriptHash> [<OptionalArgs>`

| Option | Description |
|-------:|:------------|
| `WhatIf` *or* `WhatIfRaw` | **(optional)** if specified, just prints the command line that would be used. |
| *&lt;InstallMode&gt;* | One of `Interactive`, `Silent`, or `SilentWithProgress`. (Corresponds to winget install modes.) |
| *&lt;ScriptURL&gt;* | URL from which to download the PowerShell script to run. |
| *&lt;ExpectedScriptHash&gt;* | Expected SHA256 hash of the install script (encoded as UTF16), in hex. If the downloaded script does not match this hash, it will not be run. |
| *&lt;OptionalArgs&gt;* | **optional** additional arguments to pass along to the script. |

`PowerShellStub.exe` is *extremely* simple--it only parses its arguments, then delegates all the "real work" (such as downloading the install script and verifying its hash) to `powershell.exe`, via a small "wrapper script" that is provided directly on the command line. You can use the `WhatIf` option to see what would be run.

For example, this `PowerShellStub.exe` command line:

`PowershellStub.exe WhatIf Interactive https://example.com/install-the-thing 37b91cb6eebc34c965ac3bbfc59a9ca64738054b7d31131e59698a86e38613d3 -MyCustomArg Something -Blah 'Whatever'`

Shows that it would run the following "wrapper script" (pretty-printed; use `WhatIfRaw` instead of `WhatIf` to see the raw `lpCommandLine` that would be passed to `CreateProcess`):

```powershell
    powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -Command $env:PSModulePath = $null
    Set-StrictMode -Version Latest
    try
    {
        $theScript = (iwr https://example.com/install-the-thing$env:_POWERSHELL_STUB_TEST_URL_SUFFIX -UseBasic -EA Stop).Content
        $bytes = [System.Text.Encoding]::Unicode.GetBytes( $theScript )
        $hash = ([Security.Cryptography.SHA256]::Create().ComputeHash( $bytes ) | %{$_.ToString('x2')}) -join ''
        $expectedHash = '37b91cb6eebc34c965ac3bbfc59a9ca64738054b7d31131e59698a86e38613d3'
        if( $hash -ne $expectedHash )
        {
            throw "Hash mismatch: expected $expectedHash but got $hash"
        }

        $sig = Get-AuthenticodeSignature -Source 'theScript.ps1' -Content $bytes
        if( ($sig.Status -ne 'Valid') -and ($sig.Status -ne 'NotSigned') )
        {
            throw "Signature error: $($sig.Status)"
        }

        Invoke-Expression ". {$theScript} -Interactive -MyCustomArg Something -Blah Whatever -EA Stop"
    }
    catch
    {
        Write-Host $_ -Fore Red
        $rsp = Read-Host 'pausing... press [enter] to finish'
        if( $rsp -eq 'd' )
        {
            $host.EnterNestedPrompt()
        }

        throw
    }
```

Notable details:

 * The script is executed with `StrictMode` on, `RemoteSigned` execution policy, and `-NoProfile`.
 * For `Silent` and `SilentWithProgress` modes, `-NonInteractive` is also passed.
 * The value of the environment variable `$env:_POWERSHELL_STUB_TEST_URL_SUFFIX` is appended to the ScriptUrl. This is handy for testing a new version of the install script without having to update your winget package.
 * The install script is executed directly from memory; not saved to a temporary file.
 * The winget install mode is passed as a switch parameter to the script (e.g. `-Interactive`), followed by the optional parameters, followed by `-EA Stop` (all errors are treated as terminating errors).
 * The downloaded install script does not have to be signed, but if it is, the signature must be valid.
 * If an error occurs in Interactive mode, the wrapper script pauses so you can read the error. If you enter `d` at the prompt, you'll enter a nested shell so you can poke around more (run `exit` to quit the nested shell).
 * `PowerShellStub.exe` returns the return code of the `powershell.exe` process. The exit code for an uncaught exception is `1`.

## How'd you get it so small?

If you search for information about how to create tiny EXEs, you'll see that I
didn't go nearly as far as some people do! But I'm not trying to win a contest;
any small number of kb would be fine with me, so that the whole image can fit
into just a few pages of memory.

Some techniques I used:
 * All the compiler/linker optimization switches on, and favoring "size" over "speed".
 * Don't link any default libs (not even the C runtime--`main` gets no args!).
 * Combine the `.pdata` section with the `.text` section.
 * Reduced section alignment (64).
 * Turn off unneeded things (randomized base address, debug info, etc.).

