# BrokenType

BrokenType is a set of tools designed to test the robustness and security of font rasterization software, especially codebases prone to memory corruption issues (written in C/C++ and similar languages). It consists of three components:

 - **[TrueType program generator](truetype-generator)** - a Python script for generating random, but valid [TrueType programs](https://docs.microsoft.com/en-us/typography/opentype/spec/ttinst).
 - **[TTF/OTF mutator](ttf-otf-mutator)** - a semi-"smart" binary font file mutator written in C++.
 -  **[TTF/OTF font loader for Windows](ttf-otf-windows-loader)** - a utility for loading and comprehensively testing custom fonts in Windows.

The description and usage instructions of the utilities can be found in their corresponding READMEs.

The programs and scripts were successfully used in 2015-2017 to discover and report [20](https://bugs.chromium.org/p/project-zero/issues/list?can=1&q=status:fixed%20finder:mjurczyk%20product:kernel%20methodology:mutation-fuzzing%20font&colspec=ID%20Status%20Restrict%20Reported%20Vendor%20Product%20Finder%20Summary&cells=ids) vulnerabilities in the font rasterization code present in the Windows kernel (`win32k.sys` and `atmfd.dll` drivers), and further [19](https://bugs.chromium.org/p/project-zero/issues/list?can=1&q=status:fixed%20finder:mjurczyk%20uniscribe&colspec=ID%20Status%20Restrict%20Reported%20Vendor%20Product%20Finder%20Summary&cells=ids) security flaws in the user-mode Microsoft Uniscribe library. The fuzzing efforts were discussed in the following Google Project Zero blog posts:

 - [A year of Windows kernel font fuzzing #1: the results](https://googleprojectzero.blogspot.com/2016/06/a-year-of-windows-kernel-font-fuzzing-1_27.html) (June 2016)
 - [A year of Windows kernel font fuzzing #2: the techniques](https://googleprojectzero.blogspot.com/2016/07/a-year-of-windows-kernel-font-fuzzing-2.html) (July 2016)
 - [Notes on Windows Uniscribe Fuzzing](https://googleprojectzero.blogspot.com/2017/04/notes-on-windows-uniscribe-fuzzing.html) (April 2017)

and the ["Reverse engineering and exploiting font rasterizers"](https://j00ru.vexillium.org/talks/44con-reverse-engineering-and-exploiting-font-rasterizers/) talk given in September 2015 at the 44CON conference in London. The two most notable issues found by the tool were [CVE-2015-2426](https://bugs.chromium.org/p/project-zero/issues/detail?id=369) and [CVE-2015-2455](https://bugs.chromium.org/p/project-zero/issues/detail?id=368) - an OTF bug collision with an exploit found in the Hacking Team leak, and a TTF bug collision with KeenTeam's exploit for pwn2own 2015.

## Disclaimer

This is not an official Google product.
