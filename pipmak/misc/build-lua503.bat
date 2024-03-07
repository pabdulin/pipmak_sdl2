SET DirName=%1
SET ObjName=%2
SET BuildDir=..\..\build\%ObjName%
SET SrcDir=..\..\pipmak\code_ext\%DirName%\src

@ECHO ========== Building '%SrcDir%' to '%BuildDir%' ==========

set compiler_warns=
set compiler_defines=/D"USE_DLL"
set compiler_includes=/I "%SrcDir%\..\include"
set compiler_files="%SrcDir%\lib\*.c" "%SrcDir%\*.c"
REM /TC - all sources are c files	https://learn.microsoft.com/en-us/cpp/build/reference/tc-tp-tc-tp-specify-source-file-type?view=msvc-170
REM /LD (implies /MT)

set compiler_flags=/Fe%ObjName% /TC /O2 /Zi /nologo /LD %compiler_warns% %compiler_defines% %compiler_includes% %compiler_files%
set LibFiles=
set linker_flags=%LibFiles%

@REM full cleanup
IF EXIST %BuildDir% RD %BuildDir% /q /s

@REM compile
IF NOT EXIST %BuildDir% MKDIR %BuildDir%
PUSHD %BuildDir%
  cl.exe %compiler_flags% /link %linker_flags%
POPD
