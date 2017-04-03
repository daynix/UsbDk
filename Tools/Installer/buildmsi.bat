SETLOCAL EnableExtensions EnableDelayedExpansion

IF [%UsbDkVersion%] == [] SET UsbDkVersion=99.99.90
IF [%UsbDkVersion%] == [".."] SET UsbDkVersion=99.99.90

if [%1] EQU [NOSIGN] (SET DEBUG_CFG=Debug_NoSign) ELSE (SET DEBUG_CFG=Debug)

pushd ..\..\Install_Debug\x86

del *.msi *.wixobj *.wixpdb

"C:\Program Files (x86)\WiX Toolset v3.8\bin\candle.exe" ..\..\Tools\Installer\UsbDkInstaller.wxs -out UsbDk_Debug.wixobj -dUsbDkVersion=%UsbDkVersion% -dConfig=%DEBUG_CFG%
if !ERRORLEVEL! NEQ 0 exit /B 1
"C:\Program Files (x86)\WiX Toolset v3.8\bin\light.exe" UsbDk_Debug.wixobj -out UsbDk_Debug_%UsbDkVersion%_x86.msi -sw1076
if !ERRORLEVEL! NEQ 0 exit /B 1

popd

pushd ..\..\Install_Debug\x64

del *.msi *.wixobj *.wixpdb

"C:\Program Files (x86)\WiX Toolset v3.8\bin\candle.exe" ..\..\Tools\Installer\UsbDkInstaller.wxs -out UsbDk_Debug.wixobj -dUsbDkVersion=%UsbDkVersion% -dConfig=%DEBUG_CFG% -dUsbDk64Bit=1
if !ERRORLEVEL! NEQ 0 exit /B 1
"C:\Program Files (x86)\WiX Toolset v3.8\bin\light.exe" UsbDk_Debug.wixobj -out UsbDk_Debug_%UsbDkVersion%_x64.msi -sw1076
if !ERRORLEVEL! NEQ 0 exit /B 1

popd

pushd ..\..\Install\x86

del *.msi *.wixobj *.wixpdb

"C:\Program Files (x86)\WiX Toolset v3.8\bin\candle.exe" ..\..\Tools\Installer\UsbDkInstaller.wxs -out UsbDk.wixobj -dUsbDkVersion=%UsbDkVersion% -dConfig=Release
if !ERRORLEVEL! NEQ 0 exit /B 1
"C:\Program Files (x86)\WiX Toolset v3.8\bin\light.exe" UsbDk.wixobj -out UsbDk_%UsbDkVersion%_x86.msi -sw1076
if !ERRORLEVEL! NEQ 0 exit /B 1

popd

pushd ..\..\Install\x64

del *.msi *.wixobj *.wixpdb

"C:\Program Files (x86)\WiX Toolset v3.8\bin\candle.exe" ..\..\Tools\Installer\UsbDkInstaller.wxs -out UsbDk.wixobj -dUsbDkVersion=%UsbDkVersion% -dConfig=Release -dUsbDk64Bit=1
if !ERRORLEVEL! NEQ 0 exit /B 1
"C:\Program Files (x86)\WiX Toolset v3.8\bin\light.exe" UsbDk.wixobj -out UsbDk_%UsbDkVersion%_x64.msi -sw1076
if !ERRORLEVEL! NEQ 0 exit /B 1

popd

echo SUCCEEDED