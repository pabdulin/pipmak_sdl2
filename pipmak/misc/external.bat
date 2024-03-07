@ECHO OFF
ECHO CWD==%cd%
SET GitCloneCmd=git.exe clone --recurse-submodules -j 8
SET MsbuildCmd=msbuild.exe
SET DevenvCmd=devenv.exe

CALL vsShell64.bat

PUSHD ..\..
    @REM IF NOT EXIST external (MKDIR external)
    IF NOT EXIST build (MKDIR build)
    IF NOT EXIST lib (MKDIR lib)
    IF NOT EXIST include (MKDIR include)

    PUSHD pipmak\code_ext
        ECHO CWD==%cd%
        CALL ..\misc\build-lua503.bat lua-5.0.3 lua503
        CALL ..\misc\build-libogg-1.3.5.bat libogg-1.3.5 libogg
        CALL ..\misc\build-libvorbis-1.3.7.bat libvorbis-1.3.7 libvorbis
        CALL ..\misc\build-SDL2-2.28.5.bat SDL2-2.28.5 SDL2
        MKLINK /D /J SDL SDL2-2.28.5
        CALL ..\misc\build-SDL2_image-2.8.1.bat SDL2_image-2.8.1 SDL2_image
        RMDIR SDL
    POPD 

    @REM     IF NOT EXIST SDL-1.2 (
    @REM         %GitCloneCmd% https://github.com/libsdl-org/SDL-1.2.git
    @REM         IF NOT EXIST "SDL-1.2\include\SDL_config.h" (
    @REM             MOVE "SDL-1.2\include\SDL_config.h.default" "SDL-1.2\include\SDL_config.h"
    @REM         )
    @REM     )

    @REM     IF NOT EXIST "SDL-1.2\VisualC\SDL\SDL.vcxproj.upgrade" (
    @REM         %DevenvCmd% "SDL-1.2\VisualC\SDL\SDL.vcxproj" /Upgrade /Out "SDL-1.2\VisualC\SDL\SDL.vcxproj.upgrade"
    @REM     )
    @REM     %MsbuildCmd% "SDL-1.2\VisualC\SDL\SDL.vcxproj" -p:Configuration=Release

    @REM     IF NOT EXIST "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj.upgrade" (
    @REM         %DevenvCmd% "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj" /Upgrade /Out "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj.upgrade"
    @REM     )
    @REM     %MsbuildCmd% "SDL-1.2\VisualC\SDLmain\SDLmain.vcxproj" -p:Configuration=Release
    @REM POPD
    
    @REM PUSHD build
    @REM     ECHO CWD=%cd%
    @REM     COPY ..\external\SDL-1.2\VisualC\SDL\x64\Release\SDL.dll
    @REM     COPY ..\external\SDL-1.2\VisualC\SDL\x64\Release\SDL.pdb
    @REM     COPY ..\external\SDL-1.2\VisualC\SDLmain\x64\Release\SDLmain.pdb
    @REM POPD
    
    @REM PUSHD lib
    @REM     ECHO CWD=%cd%
    @REM     IF NOT EXIST SDL (MKDIR SDL)
    @REM     COPY ..\external\SDL-1.2\VisualC\SDL\x64\Release\SDL.lib
    @REM     COPY ..\external\SDL-1.2\VisualC\SDLmain\x64\Release\SDLmain.lib
    @REM POPD
    
    @REM PUSHD include
    @REM     ECHO CWD=%cd%
    @REM     IF NOT EXIST SDL (MKDIR SDL)
    @REM     COPY ..\external\SDL-1.2\include SDL
    @REM POPD
POPD