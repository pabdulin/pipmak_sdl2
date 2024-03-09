@ECHO OFF
ECHO CWD=%cd%

SET BuildDir=..\build\pipmak
SET RelDir=..\..

CALL "misc\vsShell64.bat"

SET LibsSDL2=SDL2.lib SDL2main.lib SDL2_image.lib SDL2_ttf.lib
SET LibsSDL=%LibsSDL2%
SET LibsXiph=libogg.lib libvorbis_static.lib libvorbisfile_static.lib
SET LibsWin=Shell32.lib User32.lib Advapi32.lib Comdlg32.lib OpenGL32.Lib GlU32.Lib
SET LinkerPaths=/LIBPATH:%RelDir%\lib
SET LinkerFiles=legacy_stdio_definitions.lib lua503.lib %LibsSDL% OpenAL32.lib %LibsXiph% %LibsWin%

SET CompilerWarns=
SET CompilerDefines=/D"DEBUG" /D"WIN32"
SET IncludeSDL2=/I%RelDir%\include\SDL2 /I%RelDir%\include\SDL2_image /I%RelDir%\include\SDL2_ttf
SET IncludeSDL=%IncludeSDL2%
SET CompilerIncludePaths=/I%RelDir%\include %IncludeSDL% /I%RelDir%\include\AL /I%RelDir%\include\lua503 /I%RelDir%\include\physfs\src /I%RelDir%\include\physfs\extras
SET CompilerFiles="%RelDir%\pipmak\code\*.c" "%RelDir%\pipmak\code\physfs\src\*.c" "%RelDir%\pipmak\code\physfs\extras\*.c" "%RelDir%\pipmak\code\gl3w\*.c"

REM /Fe - Name EXE File, see: https://learn.microsoft.com/en-us/cpp/build/reference/fe-name-exe-file?view=msvc-170
REM /MD, /MT, /LD - Use Run-Time Library, see: https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-170
REM /Od - disable optimizations, see: https://learn.microsoft.com/en-us/cpp/build/reference/o-options-optimize-code?view=msvc-170
REM /Zi - produces a separate PDB file, see: https://learn.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format?view=msvc-170
REM /nologo - be quiet
SET CompilerFlags=/Fepipmak /MD /Od /Zi /nologo %CompilerWarns% %CompilerDefines% %CompilerIncludePaths% %CompilerFiles%

REM /link - Pass Options to Linker
REM /LTCG - Link-time code generation, see: https://learn.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation?view=msvc-170
REM /SUBSYSTEM - Specify Subsystem, see: https://learn.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-170
SET LinkerFlags=/link /LTCG:OFF /SUBSYSTEM:CONSOLE %LinkerPaths% %LinkerFiles%

IF NOT EXIST %BuildDir% (MKDIR %BuildDir%)
PUSHD %BuildDir%
    cl.exe %CompilerFlags% %LinkerFlags%
    DEL *.obj
    IF NOT EXIST pipmak_data (xcopy.exe %RelDir%\pipmak\resources\ pipmak_data\ /E)
POPD
