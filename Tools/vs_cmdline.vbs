Dim strCmdLine, strTemp
Set WshShell = Wscript.CreateObject("Wscript.Shell")

On Error Resume Next
strCmdLine = WshShell.RegRead("HKLM\SOFTWARE\Microsoft\VisualStudio\12.0\InstallDir")
' In case of error assume WoW64 case
If Err <> 0 Then
  On Error Goto 0
  strCmdLine = WshShell.RegRead("HKLM\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\12.0\InstallDir")
End If

On Error Goto 0
strCmdLine = chr(34) + strCmdLine + "devenv.com" + chr(34)
For i = 0 to (Wscript.Arguments.Count - 1)
  strTemp = Wscript.Arguments(i)
  If InStr(strTemp, " ") Or InStr(strTemp, "|") Then
      strCmdLine = strCmdLine + " " + chr(34) + strTemp + chr(34)
  Else
      strCmdLine = strCmdLine + " " + strTemp
  End If
Next

WScript.Echo strCmdLine + vbCrLf
