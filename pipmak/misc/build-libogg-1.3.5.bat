SET DirName=%1
SET ObjName=%2
SET BuildDir=..\..\build\%ObjName%
SET SrcDir=..\..\pipmak\code_ext\%DirName%\win32\VS2015

@ECHO ========== Building '%SrcDir%' to '%BuildDir%' ==========

@REM set compiler_warns=
@REM set compiler_defines=/D"USE_DLL"
@REM set compiler_includes=/I "%SrcDir%\..\include"
@REM set compiler_files="%SrcDir%\lib\*.c" "%SrcDir%\*.c"
REM /TC - all sources are c files	https://learn.microsoft.com/en-us/cpp/build/reference/tc-tp-tc-tp-specify-source-file-type?view=msvc-170
REM /LD (implies /MT)

@REM set compiler_flags=/Fe%ObjName% /TC /O2 /Zi /nologo /LD %compiler_warns% %compiler_defines% %compiler_includes% %compiler_files%
@REM set LibFiles=
@REM set linker_flags=%LibFiles%

@REM full cleanup
IF EXIST %BuildDir% RD %BuildDir% /q /s

@REM compile
IF NOT EXIST %BuildDir% MKDIR %BuildDir%
PUSHD %BuildDir%
  msbuild %SrcDir%\libogg.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -p:OutDir=..\..\..\%BuildDir%\
POPD
