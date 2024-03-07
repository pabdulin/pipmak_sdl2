@ECHO OFF
ECHO CWD=%cd%

CALL "misc\vsShell64.bat"

SET LibsSDL1=SDL.lib SDLmain.lib SDL_image.lib SDL_ttf.lib
SET LibsSDL2=SDL2.lib SDL2main.lib SDL2_image.lib SDL2_ttf.lib
SET LibsSDL=%LibsSDL2%
SET LibsXiph=libogg.lib libvorbis.lib libvorbisfile.lib
SET LibsWin=Shell32.lib User32.lib Advapi32.lib Comdlg32.lib OpenGL32.Lib GlU32.Lib
SET LinkerPaths=/LIBPATH:..\lib
SET LinkerFiles=legacy_stdio_definitions.lib lua503.lib %LibsSDL% OpenAL32.lib %LibsXiph% %LibsWin%

SET CompilerWarns=
SET CompilerDefines=/D"DEBUG" /D"WIN32"
SET IncludeSDL1=/I..\include\SDL /I..\include\SDL_image /I..\include\SDL_ttf
SET IncludeSDL2=/I..\include\SDL2 /I..\include\SDL2_image /I..\include\SDL2_ttf
SET IncludeSDL=%IncludeSDL2%
SET CompilerIncludePaths=/I..\include %IncludeSDL% /I..\include\AL /I..\include\lua503 /I..\include\physfs
SET CompilerFiles="..\pipmak\code\*.c" "..\pipmak\code\physfs\*.c" "..\pipmak\code\gl3w\*.c"

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
