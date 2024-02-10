
default: help
all : help

help:
	@echo " \
	                         __  \n\
	 _____ _____ __    _____|  | \n\
	|  |  |   __|  |  |  _  |  | \n\
	|     |   __|  |__|   __|__| \n\
	|__|__|_____|_____|__|  |__| \n\
	========================================"
	@echo " \
	Building:							    \n\
		dc\t\tSega Dreamcast (elf, bin)		\n\
		win\t\tWindows (exe, mingw64)		\n\
		build-all\tBuilds each platform     \n\
		\n\
	Clean:									\n\
		clean-dc\t\t						\n\
		clean-win\t\t						\n\
		clean-all\t\t					    \n\
		\n\
	Testing:										\n\
		test-dc\t\tBuild cdi and launch NullDC		\n\
		test-win\tRun .exe							\n\
		test-all\tRuns all Three  					\n\
	"

dc:
	@echo " \
	                                          \n\
	 ____                                _    \n\
	|    \ ___ ___ ___ _____ ___ ___ ___| |_  \n\
	|  |  |  _| -_| .'|     |  _| .'|_ -|  _| \n\
	|____/|_| |___|__,|_|_|_|___|__,|___|_|   \n\
	========================================"
	@docker run --rm -v "${PWD}:/${PWD}" -u `id -u`:`id -g` -w "${PWD}" haydenkow/nu_dckos make -j11 -f Makefile.dc

win:
	@echo " \
	                                \n\
	 _ _ _ _       _                \n\
	| | | |_|___ _| |___ _ _ _ ___  \n\
	| | | | |   | . | . | | | |_ -| \n\
	|_____|_|_|_|___|___|_____|___| \n\
	========================================"
	@docker run --rm -v "${PWD}:/${PWD}" -u `id -u`:`id -g` -w "${PWD}" kazade/windows-sdk mingw64-make -j11 -f Makefile.win

# clean house
clean-dc:
	dc-docker make -f Makefile.dc clean

# clean house
clean-win:
	win-docker make -f Makefile.win clean

test-dc: dc
	cp build_dc/1ST_READ.BIN build_cdi/data_hb/
	cmd.exe /C "cd build_cdi && (build_image.bat > nul 2>&1) && run_image.bat  "

test-win: win
	cmd.exe /C "cd D:\Dev\Dreamcast\UB_SHARE\clean_nuQuake\build_win && ""D:\Dev\TDM-GCC-64\bin\gdb.exe"" -q -command debug-win.txt  ""D:\Dev\Dreamcast\UB_SHARE\clean_nuQuake\build_win\glquake.exe"" "

clean-all: clean-dc clean-win

build-all: dc win

test-all: test-dc test-win

%.sh:

%: %.sh
