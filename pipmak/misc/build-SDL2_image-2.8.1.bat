SET DirName=%1
SET ObjName=%2
SET BuildDir=..\..\build\%ObjName%
SET LibDir=..\..\lib
SET SrcDir=..\..\pipmak\code_ext\%DirName%\VisualC

@ECHO ========== Building '%SrcDir%' to '%BuildDir%' ==========

@REM full cleanup
IF EXIST %BuildDir% RD %BuildDir% /q /s

@REM compile
IF NOT EXIST %BuildDir% MKDIR %BuildDir%
PUSHD %BuildDir%
  msbuild %SrcDir%\SDL_image.sln -target:SDL2_image -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143
  COPY /Y %SrcDir%\x64\Release\sdl*.lib %LibDir%
  COPY /Y %SrcDir%\x64\Release\sdl*.pdb %BuildDir%
  COPY /Y %SrcDir%\x64\Release\sdl*.dll %BuildDir%
  COPY /Y %SrcDir%\x64\Release\sdl*.pdb ..
  COPY /Y %SrcDir%\x64\Release\sdl*.dll ..
POPD
