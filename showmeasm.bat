@echo off
@rem usage showmeasm.bat  objectfile  function
@rem e.g.  showmeasm.bat  mathlib.c.o BoxOnPlaneSide

@rem sh-elf-objdump.exe -w -C -d -z -j .text.%2 %1
set PYTHONHOME="D:\Dev\DreamSDK\msys\1.0\opt\toolchains\dc\sh-elf\bin\python27.dll"
set PYTHONPATH="D:\Dev\DreamSDK\msys\1.0\opt\toolchains\dc\sh-elf\bin"
D:\Dev\DreamSDK\msys\1.0\opt\toolchains\dc\sh-elf\bin\sh-elf-gdb.exe -batch -ex "disassemble/rs %2" %1