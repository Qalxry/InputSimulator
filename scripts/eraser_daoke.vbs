dim exePath
exePath = "E:\Project\SelfProjects\InputSimulator\scripts\input_simulator.exe"

set ws = WScript.CreateObject("WScript.Shell")

ws.Run exePath & " -k mouse_right", 0, True
ws.Run exePath & " -k mouse_right", 0, True
ws.Run exePath & " -k mouse_right", 0, True
ws.Run exePath & " -k key_n", 0, True
ws.Run exePath & " -k key_down", 0, True
ws.Run exePath & " -k key_down", 0, True        
ws.Run exePath & " -k key_down", 0, True        
ws.Run exePath & " -k key_down", 0, True        
ws.Run exePath & " -k key_down", 0, True        
ws.Run exePath & " -k key_down", 0, True        
ws.Run exePath & " -k key_down", 0, True        
ws.Run exePath & " -k key_enter", 0, True