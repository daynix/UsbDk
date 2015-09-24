@echo off

traceview.exe -start UsbDk -rt -flag 0xFF -level 6 -ft 1 -pdb UsbDk.pdb -guid UsbDk.ctl -f UsbDkTrace.etl

echo .
echo Tracing is in progress. Press ENTER to stop it.
echo .

pause

traceview.exe -stop UsbDk
