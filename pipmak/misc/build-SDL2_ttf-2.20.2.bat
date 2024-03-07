SET DirName=%1
SET ObjName=%2
SET BuildDir=..\..\build\%ObjName%
SET SrcDir=..\..\pipmak\code_ext\%DirName%\VisualC

@ECHO ========== Building '%SrcDir%' to '%BuildDir%' ==========

@REM full cleanup
IF EXIST %BuildDir% RD %BuildDir% /q /s

@REM compile
IF NOT EXIST %BuildDir% MKDIR %BuildDir%
PUSHD %BuildDir%
  msbuild %SrcDir%\SDL_ttf.sln -target:SDL2_ttf -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143
  COPY /Y %SrcDir%\x64\Release\sdl*ttf.lib %BuildDir%
  COPY /Y %SrcDir%\x64\Release\sdl*ttf.pdb %BuildDir%
  COPY /Y %SrcDir%\x64\Release\sdl*ttf.dll %BuildDir%
POPD
