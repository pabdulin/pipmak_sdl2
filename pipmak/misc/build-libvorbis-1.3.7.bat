SET DirName=%1
SET ObjName=%2
SET BuildDir=..\..\build\%ObjName%
SET LibDir=..\..\lib
SET SrcDir=..\..\pipmak\code_ext\%DirName%\win32\VS2010

@ECHO ========== Building '%SrcDir%' to '%BuildDir%' ==========

@REM full cleanup
IF EXIST %BuildDir% RD %BuildDir% /q /s

@REM compile
IF NOT EXIST %BuildDir% MKDIR %BuildDir%
PUSHD %BuildDir%
  msbuild %SrcDir%\vorbis_static.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143
  COPY /Y %SrcDir%\x64\Release\*.lib %LibDir%
POPD
