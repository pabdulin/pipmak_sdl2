@ECHO OFF
ECHO CWD=%cd%
SET GitCloneCmd=git.exe clone --recurse-submodules -j 8
SET MsbuildCmd=msbuild.exe
SET DevenvCmd=devenv.exe

CALL ensureShell64.bat

PUSHD ..\..
    IF NOT EXIST external (MKDIR external)
    IF NOT EXIST build (MKDIR build)
    IF NOT EXIST lib (MKDIR lib)
    IF NOT EXIST include (MKDIR include)

    PUSHD external
        ECHO CWD=%cd%
        IF NOT EXIST SDL-1.2 (
            %GitCloneCmd% https://github.com/libsdl-org/SDL-1.2.git
            IF NOT EXIST "SDL-1.2\include\SDL_config.h" (
                MOVE "SDL-1.2\include\SDL_config.h.default" "SDL-1.2\include\SDL_config.h"
            )
        )

        IF NOT EXIST "SDL-1.2\VisualC\SDL\SDL.vcxproj.upgrade" (
            %DevenvCmd% "SDL-1.2\VisualC\SDL\SDL.vcxproj" /Upgrade /Out "SDL-1.2\VisualC\SDL\SDL.vcxproj.upgrade"
        )
        %MsbuildCmd% "SDL-1.2\VisualC\SDL\SDL.vcxproj" -p:Configuration=Release

        IF NOT EXIST "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj.upgrade" (
            %DevenvCmd% "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj" /Upgrade /Out "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj.upgrade"
        )
        %MsbuildCmd% "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj" -p:Configuration=Release
    POPD
    
    PUSHD build
        ECHO CWD=%cd%
        COPY ..\external\SDL-1.2\VisualC\SDL\x64\Release\SDL.dll
        COPY ..\external\SDL-1.2\VisualC\SDL\x64\Release\SDL.pdb
        COPY ..\external\SDL-1.2\VisualC\SDLmain\x64\Release\SDLmain.pdb
    POPD
    
    PUSHD lib
        ECHO CWD=%cd%
        IF NOT EXIST SDL (MKDIR SDL)
        COPY ..\external\SDL-1.2\VisualC\SDL\x64\Release\SDL.lib
        COPY ..\external\SDL-1.2\VisualC\SDLmain\x64\Release\SDLmain.lib
    POPD
    
    PUSHD include
        ECHO CWD=%cd%
        IF NOT EXIST SDL (MKDIR SDL)
        COPY ..\external\SDL-1.2\include SDL
    POPD
POPD