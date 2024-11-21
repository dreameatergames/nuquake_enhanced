Building guide - Bruce


source /opt/toolchains/dc/kos/environ.sh
meson setup  build_dc --cross-file sh4-dreamcast-kos
meson compile -C build_dc


cdi / Dreamshell iso guide.

chmod +x makecdi.sh
./makecdi.sh







GLDC  --- 


GLDC causes some issues so you have to build it yourself and place the files in  deps/libgl replacing the old ones. 

git clone https://gitlab.com/simulant/GLdc

modify line 1799 in texture.c  FASTCPY(targetData, conversionBuffer, destBytes); to this memcpy(targetData, conversionBuffer, destBytes);

you also want to modify these lines in cmakelists.txt 
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -fexpensive-optimizations -fomit-frame-pointer -finline-functions")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++14 -O3 -g0 -s -fomit-frame-pointer -fstrict-aliasing")


change the -03 to -02 which should solve the stretched out triangles i.e change to.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -fexpensive-optimizations -fomit-frame-pointer -finline-functions")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++14 -O2 -g0 -s -fomit-frame-pointer -fstrict-aliasing")

note - not a fix and some issues still occur. 

then build gldc and replace the lib and include files in deps/libgl 

mkdir dcbuild
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/Dreamcast.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make






Note -  to uncomment sound to have it play on flycast look in  /src/client/snd.dma.c  and just uncomment the function S_Init() and sound will play in flycast only. For it to work on the dreamcast I suspect you have to build  a proper cdi / dreamshell ISO with CDDA tracks set up correctly. (I don't know)


Note - makecdi.sh has some dependencies.. mkdcdisc and mkdcdisc.