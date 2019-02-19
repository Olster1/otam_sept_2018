../bin/ctime -begin test.ctm

ERRORS_OFF=-Wno-c++11-compat-deprecated-writable-strings
clang++ $ERRORS_OFF -Wl,-rpath,@executable_path/ -O2 -I ../libs/gl3w -I ../shared/  main.cpp ../libs/gl3w/GL/gl3w.cpp -L../bin -F../bin -framework SDL2 -framework OpenGl -framework CoreFoundation -DDEVELOPER_MODE=0 -DDESKTOP=1 -o ../bin/game 
../bin/ctime -end test.ctm
