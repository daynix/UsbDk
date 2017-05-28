@echo off

SETLOCAL EnableExtensions EnableDelayedExpansion

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
"C:\Program Files (x86)\Windows Kits\10\bin\x86\tracepdb.exe" -s -o .\UsbDk.tmf
if !ERRORLEVEL! NEQ 0 exit /B 1
popd

pushd Install_Debug
"C:\Program Files (x86)\Windows Kits\10\bin\x86\tracepdb.exe" -s -o .\UsbDk.tmf
if !ERRORLEVEL! NEQ 0 exit /B 1
popd

if [%1] EQU [NOMSI] goto NOMSI

:BUILD_MSI

pushd Tools\Installer

SET UsbDkVersion="%USBDK_MAJOR_VERSION%.%USBDK_MINOR_VERSION%.%USBDK_BUILD_NUMBER%"
buildmsi.bat %2

popd

:NOMSI

ENDLOCAL
