@echo off
set CommonCompilerFlags= -Od -MTd -nologo -GR- -Gm- -EHa- -Oi -WX -W4 -wd4189 -wd4201 -wd4100 -wd4505 -FC -Z7
cl -wd4577 -O2 -Oi /DDEVELOPER_MODE=0 /DDESKTOP=1 /DNOMINMAX /Fe../bin/feohthefitter.exe /I../shared /I ../SDL2 /I../libs/gl3w /IE:/include /Zi main.cpp ../libs/gl3w/GL/gl3w.cpp /link ..\bin\SDL2.lib ..\bin\SDL2main.lib opengl32.lib shlwapi.lib /SUBSYSTEM:WINDOWS ./myres.res