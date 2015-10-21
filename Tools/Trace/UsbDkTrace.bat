@echo off

traceview.exe -start UsbDk -rt -flag 0x7FFFFFFF -level 6 -ft 1 -pdb UsbDk.pdb -guid UsbDk.ctl

mkdir WPP
start traceview.exe -process -rt UsbDk -display -ods -o WPP\\UsbDk.txt -pdb UsbDk.pdb -guid UsbDk.ctl
start dbgview.exe

pause

traceview.exe -stop UsbDk
