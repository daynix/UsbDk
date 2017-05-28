echo off
pushd "%~dp0"
logman stop usbdkm -ets >nul 2>&1
logman delete usbdkm -ets >nul 2>&1
logman create trace usbdkm -o usbdk.etl -ow -ets
logman update usbdkm -p {88e1661f-48b6-410f-b096-ba84e9f0656f} 0x7fffffff 6 -ets
echo Recording started.
echo Reproduce the problem, then press ENTER
pause > nul
logman stop usbdkm -ets
dir usbdk.etl
echo Please collect usbdk.etl file now
pause
popd
