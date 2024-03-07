@ECHO OFF
ECHO CWD==%cd%
SET GitCloneCmd=git.exe clone --recurse-submodules -j 8
SET MsbuildCmd=msbuild.exe
SET DevenvCmd=devenv.exe

CALL vsShell64.bat

PUSHD ..\..
    IF NOT EXIST build (MKDIR build)
    IF NOT EXIST lib (MKDIR lib)
    IF NOT EXIST include (MKDIR include)

    PUSHD pipmak\code_ext
        CALL ..\misc\build-lua503.bat lua-5.0.3 lua503
        CALL ..\misc\build-libogg-1.3.5.bat libogg-1.3.5 libogg
        CALL ..\misc\build-libvorbis-1.3.7.bat libvorbis-1.3.7 libvorbis
        CALL ..\misc\build-SDL2-2.28.5.bat SDL2-2.28.5 SDL2
        MKLINK /D /J SDL SDL2-2.28.5
        CALL ..\misc\build-SDL2_image-2.8.1.bat SDL2_image-2.8.1 SDL2_image
        CALL ..\misc\build-SDL2_ttf-2.20.2.bat SDL2_ttf-2.20.2 SDL2_ttf
        RMDIR SDL
    POPD
POPD