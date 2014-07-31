@echo off

for /f "tokens=*" %%a in (
'cscript.exe /nologo "%~dp0\vs_cmdline.vbs" %*'
) do (
SET vs_cmd=%%a
)

IF NOT DEFINED vs_cmd (
echo Visual Studio not found
EXIT /b 1
)

SET vs_cmd_no_quotes="%vs_cmd:"=%"
IF "vs_cmd_no_quotes" == "" (
echo Visual Studio not found
EXIT /b 2
)

%vs_cmd%
if %ERRORLEVEL% GEQ 1 (
echo Build with Visual Studio FAILED
EXIT /b 3
)

EXIT /b 0
