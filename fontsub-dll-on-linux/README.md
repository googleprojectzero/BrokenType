# Linux DLL loader for the Windows Font Subsetting Library (FontSub.dll)

This project is an equivalent of the [Windows FontSub Harness](https://github.com/googleprojectzero/BrokenType/tree/master/ttf-fontsub-loader), but aimed to run on Linux, by introducing a thin PE loading component. It is capable of mapping the PE sections in the Linux address space, setting the requested memory access rights, redirecting `msvcrt.dll` imports to the corresponding libc functions, providing custom stubs for other imports, and handling basic relocations. As it turns out, the `FontSub.dll` library is simple and self-contained enough that this is sufficient to have it run on Linux.

The benefit to this approach is that with the harness, all typical Linux fuzzing tools become available:
  - You can use [AFL](http://lcamtuf.coredump.cx/afl/), [honggfuzz](https://github.com/google/honggfuzz), or your favourite Linux-based fuzzer.
  - You can use custom allocators, either by compiling them in, or injecting them through `LD_PRELOAD` (e.g. AFL's [libdislocator](https://github.com/google/afl/tree/master/libdislocator)).
  - You can use performance improvements such as a "Fork Server", which are easier to implement on Linux.
  - You can use additional DBI such as [Intel Pin](https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool) or [DynamoRIO](https://www.dynamorio.org/), which in our experience work most reliably on Linux.

The program is meant to serve as a proof-of-concept and reference for quickly developing simple Linux DLL loaders for specialized tasks. Another more complex and mature project of this kind is Tavis Ormandy's [loadlibrary](https://github.com/taviso/loadlibrary).

We used a similar tool for fuzzing a JPEG2000 decoder in VMware Workstation in 2016, see slides 195-208 of the [Windows Metafiles - An Analysis of the EMF Attack Surface & Recent Vulnerabilities](https://j00ru.vexillium.org/slides/2016/metafiles_full.pdf) presentation.

## Requirements

The tool uses the [pe-parse](https://github.com/trailofbits/pe-parse) library for a majority of the PE-related work. You also obviously need the Windows `FontSub.dll` system file to execute.

## Building

With `pe-parse` installed in the system, simply type `make` in the project directory:

```
$ make
g++ -c -o loader.o loader.cc -m32 -DDEBUG
g++ -o loader loader.o -lpe-parser-library -ldl -m32
$
```

## Usage

Before using the loader, please note that some recent versions of the FontSub library contain telemetry functions such as `LogCreateFontPackage` and `LogMergeFontPackage`, called at the end of the exported routines, e.g.:

```
.text:0000000180001345 loc_180001345:                          ; CODE XREF: CreateFontPackage+C1
.text:0000000180001345                                         ; CreateFontPackage+161
.text:0000000180001345                 movzx   ebx, r10w
.text:0000000180001349                 mov     ecx, ebx        ; unsigned int
.text:000000018000134B                 call    LogCreateFontPackage
.text:0000000180001350                 mov     eax, ebx
```

These functions will call unimplemented Win32 API imports and crash the loader. Since their execution is not essential to the correct functioning of the subsetter, you might consider disabling them in your copy of the DLL.

Then, just pass the path of the library, path of the input TrueType, and optionally the desired image base:

```
$ ./loader fontsub.dll times.ttf 0x20000000
[+] Provided fontsub.dll file successfully parsed.
[*] Attempting to map section .text    base 20001000, size 88576
[+] SUCCESS!
[*] Attempting to map section .data    base 20017000, size 1536
[+] SUCCESS!
[*] Attempting to map section .idata   base 20018000, size 2048
[+] SUCCESS!
[*] Attempting to map section .rsrc    base 20019000, size 1024
[+] SUCCESS!
[*] Attempting to map section .reloc   base 2001a000, size 2560
[+] SUCCESS!
[...]
[*] Resolving the MSVCRT.DLL!memmove import.
[+] Function memmove found in libc at address 0xf7b53690.
[*] Resolving the MSVCRT.DLL!memcmp import.
[+] Function memcmp found in libc at address 0xf7b688e0.
[*] Resolving the MSVCRT.DLL!memcpy import.
[+] Function memcpy found in libc at address 0xf7b53050.
[*] Resolving the MSVCRT.DLL!memset import.
[+] Function memset found in libc at address 0xf7b51e20.
[...]
[*] Setting desired access rights for section .text
[+] SUCCESS!
[*] Setting desired access rights for section .data
[+] SUCCESS!
[*] Setting desired access rights for section .idata
[+] SUCCESS!
[*] Setting desired access rights for section .rsrc
[+] SUCCESS!
[*] Setting desired access rights for section .reloc
[+] SUCCESS!
[+] Located CreateFontPackage at 0x20002500, MergeFontPackage at 0x20002630.
[A] malloc(0x168) ---> 0x573eebb0
[A] malloc(0x121b) ---> 0x573f3780
[A] malloc(0x14) ---> 0x573ed7d0
[A] malloc(0x490) ---> 0x573eed20
[A] malloc(0xfae) ---> 0x573f49a0
[A] free(0x573eed20)
[A] free(0x573f49a0)
[...]
[A] free(0xf79a1010)
[+] CreateFontPackage([ 1162804 bytes ], TTFCFP_SUBSET) ---> 0 (344996 bytes)
[A] realloc((nil), 0x543a4) ---> 0x573f7a20
[A] free(0x573f7a20)
[+] MergeFontPackage(NULL, [ 344996 bytes ], TTFMFP_SUBSET) ---> 0 (344996 bytes)
$
```

There are a lot of debug messages in the above output. If you build the tool without `-DDEBUG`, the output is much cleaner:

```
$ ./loader fontsub.dll times.ttf 0x20000000
[+] CreateFontPackage([ 1162804 bytes ], TTFCFP_SUBSET) ---> 0 (344996 bytes)
[+] MergeFontPackage(NULL, [ 344996 bytes ], TTFMFP_SUBSET) ---> 0 (344996 bytes)
$
```

## Limitations

The harness currently only supports x86 `FontSub.dll` modules, but adding support for x64 images should be relatively straightforward.

However, in a more general scenario, there are a number of differences between the Windows and Linux runtime environments, and the MSVC/gcc/clang compilers. They may result in compatbility problems with varying degrees of difficulty to solve. For instance:
  - `sizeof(long)` is 4 on 64-bit Windows, but 8 on 64-bit Linux.
  - Both systems have [different](https://en.wikipedia.org/wiki/X86_calling_conventions#x86-64_calling_conventions) calling conventions on the x64 architecture. This can be somewhat mitigated by using the `ms_abi` attribute for external function prototypes.
  - Accessing the TEB/PEB Windows structures through segment registers like `fs` or `gs` will most likely crash the loader, if some kind of emulation is not developed. They may be directly referenced in code to make use of the [SEH](https://docs.microsoft.com/en-us/windows/win32/debug/structured-exception-handling) mechanism (in built-in functions such as `_SEH_prolog4` and `_SEH_epilog4`), dynamic stack allocations (`alloca()`, to get the stack base address) etc.
  - The exception handling architecture is vastly different in Windows/Linux, and so a target which makes extensive use of them may be problematic. Furthermore, standard C++ exceptions are implemented with the `RaiseException` API on Windows, which is not provided by the harness at the moment.

Last but not least, all external Windows API functions used by the library in question must be emulated or at least stubbed out, which may range from a simple to an almost impossible task.

