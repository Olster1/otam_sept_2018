..\bin\ctime -begin test.ctm
cl /DDESKTOP=1 /DNOMINMAX /I../shared /I../libs/gl3w /IE:/include /Zi main.cpp ../libs/gl3w/GL/gl3w.cpp E:\lib\SDL2.lib E:\lib\SDL2main.lib opengl32.lib shlwapi.lib /Fe../bin/Fitris.exe /link /SUBSYSTEM:WINDOWS
..\bin\ctime -end test.ctm
