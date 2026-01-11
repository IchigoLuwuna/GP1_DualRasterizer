export PATH=/opt/msvc/bin/x64:$PATH
CC=cl CXX=cl cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Windows
make
