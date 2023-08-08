# PowershellStub

## What is it?

A ***tiny*** (less than 2k!) EXE that forwards its arguments on to the Windows built-in `powershell.exe` (legacy Windows PowerShell 5.1), then waits for it to exit, and returns `powershell.exe`'s return code as its own.

## But... WHY is it?

I was creating a `winget` package manifest, and the "installer" that I wanted to wrap was just a script... but `winget` needs to get something that it can *run* (executing a `.ps1` just opens it in a text editor), and it really wants to download that thing from the interwebs. So I provided it something to run, downloadable, in the form of `PowershellStub.exe`. You are welcome to use it, too, if you like (see [the WingetPathUpdater manifest](https://github.com/jazzdelightsme/WingetPathUpdater/blob/main/manifests/j/jazzdelightsme/WingetPathUpdater/1.0/jazzdelightsme.WingetPathUpdater.installer.yaml) for example usage).

## How'd you get it so small?

If you search for information about how to create tiny EXEs, you'll see that I didn't go nearly as far as some people do! But I'm not trying to win a contest; anything under 4k would have been fine with me, so that the whole image can fit into a single page of memory.

Techniques I used:
 * All the compiler/linker optimization switches on, and favoring "size" over "speed".
 * Don't link any default libs (not even the C runtime--`main` gets no args!).
 * Combine the `.pdata` section with the `.text` section.
 * Reduced section alignment (128).
 * Turn off unneeded things (randomized base address, debug info, etc.).

