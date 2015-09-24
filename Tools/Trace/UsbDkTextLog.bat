@echo off

tracepdb -f UsbDk.pdb
tracefmt.exe -display -p . UsbDkTrace.etl
