../bin/ctime -begin test.ctm

ERRORS_OFF=-Wno-c++11-compat-deprecated-writable-strings
clang++ $ERRORS_OFF -Wl,-rpath,@executable_path/ -I ../libs/gl3w -I ../shared/  main.cpp ../libs/gl3w/GL/gl3w.cpp -L../bin -F../bin -framework SDL2 -framework OpenGl -framework CoreFoundation -DDESKTOP=1 -DDEVELOPER_MODE=0 -o ../bin/launcher -g
../bin/ctime -end test.ctm