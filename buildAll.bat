@echo off

SETLOCAL EnableExtensions EnableDelayedExpansion

del *.log

for %%x in (Win7, Win8, Win8.1, XP) do (
  for %%y in (Debug, Release) do (
    for %%z in (win32, x64) do (
      call tools\vs_run.bat UsbDk.sln /Rebuild "%%x %%y|%%z" /Out build%%y_%%x_%%z.log
      if !ERRORLEVEL! NEQ 0 exit /B 1
    )
  )
)

ENDLOCAL
