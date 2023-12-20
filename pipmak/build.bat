@ECHO OFF
ECHO CWD=%cd%

CALL "misc\ensureShell64.bat"

SET LibsWin=Shell32.lib User32.lib Advapi32.lib Comdlg32.lib OpenGL32.Lib GlU32.Lib
SET LibsSDL1=SDL.lib SDLmain.lib SDL_image.lib SDL_ttf.lib
SET LibsSDL2=
SET LibsXiph=libogg.lib libvorbis.lib libvorbisfile.lib
SET LinkerPaths=/LIBPATH:..\lib
SET LinkerFiles=lua503.lib %LibsSDL1% OpenAL32.lib %LibsXiph% %LibsWin%

SET CompilerWarns=
SET CompilerDefines=/D"DEBUG" /D"WIN32" /D"_USE_MATH_DEFINES"
SET CompilerFiles="..\pipmak\code\*.c" "..\pipmak\code\physfs\*.c" "..\pipmak\code\gl3w\*.c"
SET CompilerIncludePaths=/I..\include /I..\include\SDL /I..\include\SDL_image /I..\include\SDL_ttf /I..\include\AL /I..\include\lua503 /I..\include\physfs

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

IF NOT EXIST ..\build (MKDIR ..\build)
PUSHD ..\build
    cl.exe %CompilerFlags% %LinkerFlags%
    DEL *.obj
POPD
