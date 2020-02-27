@echo off

SETLOCAL EnableExtensions EnableDelayedExpansion

set _f=UsbDk
if [%1] EQU [MSIONLY] goto BUILD_MSI
if [%2] EQU [NOSIGN] (SET DEBUG_CFG=Debug_NoSign) ELSE (SET DEBUG_CFG=Debug)

del *.log

for %%x in (Win7, Win8, Win8.1, Win10, XP) do (
  for %%y in (%DEBUG_CFG%, Release) do (
    for %%z in (win32, x64) do (
      call tools\vs_run.bat UsbDk.sln /Rebuild "%%x %%y|%%z" /Out build%%y_%%x_%%z.log
      if !ERRORLEVEL! NEQ 0 exit /B 1
    )
  )
)

pushd Install
call :maketmf Release
if !ERRORLEVEL! NEQ 0 exit /B 1
popd

pushd Install_Debug
call :maketmf %DEBUG_CFG%
if !ERRORLEVEL! NEQ 0 exit /B 1
popd

if [%1] EQU [NOMSI] goto NOMSI
goto BUILD_MSI

:maketmf
del *.tmf *.mof
call :make1tmf x64\Win10%1
call :make1tmf x86\Win10%1
call :make1tmf x64\Win8.1%1
call :make1tmf x86\Win8.1%1
call :make1tmf x64\Win8%1
call :make1tmf x86\Win8%1
call :make1tmf x64\Win7%1
call :make1tmf x86\Win7%1
call :make1tmf x64\XP%1
call :make1tmf x86\XP%1
goto :eof

:make1tmf
pushd %1
echo Making TMF in %1
"C:\Program Files (x86)\Windows Kits\10\bin\x86\tracepdb.exe" -s -o .\%_f%.tmf
popd
type %1\%_f%.tmf >> %_f%.tmf
del %1\%_f%.??f
goto :eof

:BUILD_MSI

pushd Tools\Installer

SET UsbDkVersion="%USBDK_MAJOR_VERSION%.%USBDK_MINOR_VERSION%.%USBDK_BUILD_NUMBER%"
buildmsi.bat %2

popd

:NOMSI

ENDLOCAL
