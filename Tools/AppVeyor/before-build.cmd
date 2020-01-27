@echo off
pushd "%~dp0"
curl -L -s https://drive.google.com/uc?id=17ZK6a1kxhTR-YOkWpAOqNJZA7UBUGkmu --output Microsoft.DriverKit.Build.Tasks.14.0.dll
copy /y Microsoft.DriverKit.Build.Tasks.14.0.dll "C:\Program Files (x86)\Windows Kits\10\build\bin"
popd
