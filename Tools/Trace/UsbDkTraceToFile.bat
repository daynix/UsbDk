@echo off

traceview.exe -start UsbDk -rt -flag 0x7FFFFFFF -level 6 -ft 1 -guid UsbDk.ctl -f UsbDkTrace.etl

echo .
echo Tracing is in progress. Press ENTER to stop it.
echo .

pause

traceview.exe -stop UsbDk
