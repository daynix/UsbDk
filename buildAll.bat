@echo off

SETLOCAL EnableExtensions EnableDelayedExpansion

if [%1] EQU [MSIONLY] goto BUILD_MSI

del *.log

for %%x in (Win7, Win8, Win8.1, XP) do (
  for %%y in (Debug, Release) do (
    for %%z in (win32, x64) do (
      call tools\vs_run.bat UsbDk.sln /Rebuild "%%x %%y|%%z" /Out build%%y_%%x_%%z.log
      if !ERRORLEVEL! NEQ 0 exit /B 1
    )
  )
)

if [%1] EQU [NOMSI] goto NOMSI

:BUILD_MSI

pushd Tools\Installer

SET UsbDkVersion="%USBDK_MAJOR_VERSION%.%USBDK_MINOR_VERSION%.%USBDK_BUILD_NUMBER%"
buildmsi.bat

popd

:NOMSI

ENDLOCAL
