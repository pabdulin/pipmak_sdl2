@ECHO OFF
SET VsShell64="%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
IF NOT DEFINED DevEnvDir (
  IF EXIST %VsShell64% (CALL %VsShell64%)
)
