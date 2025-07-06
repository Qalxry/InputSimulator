dim exePath
exePath = "E:\Project\SelfProjects\MouseClickSimulator\scripts\MouseClickSimulator.exe"

set ws = WScript.CreateObject("WScript.Shell")

ws.Run exePath & " -k switch_focus -smt 0", 0, True
ws.Run exePath & " -k mouse_left -x 2490 -y 700 -m back", 0, True
