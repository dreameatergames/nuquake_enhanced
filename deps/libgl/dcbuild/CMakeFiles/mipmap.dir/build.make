# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dreamdev/projects/kos/libraries/GLdc

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dreamdev/projects/kos/libraries/GLdc/dcbuild

# Include any dependencies generated for this target.
include CMakeFiles/mipmap.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/mipmap.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mipmap.dir/flags.make

CMakeFiles/mipmap.dir/samples/mipmap/main.c.obj: CMakeFiles/mipmap.dir/flags.make
CMakeFiles/mipmap.dir/samples/mipmap/main.c.obj: ../samples/mipmap/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/mipmap.dir/samples/mipmap/main.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mipmap.dir/samples/mipmap/main.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/mipmap/main.c

CMakeFiles/mipmap.dir/samples/mipmap/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mipmap.dir/samples/mipmap/main.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/mipmap/main.c > CMakeFiles/mipmap.dir/samples/mipmap/main.c.i

CMakeFiles/mipmap.dir/samples/mipmap/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mipmap.dir/samples/mipmap/main.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/mipmap/main.c -o CMakeFiles/mipmap.dir/samples/mipmap/main.c.s

CMakeFiles/mipmap.dir/samples/loadbmp.c.obj: CMakeFiles/mipmap.dir/flags.make
CMakeFiles/mipmap.dir/samples/loadbmp.c.obj: ../samples/loadbmp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/mipmap.dir/samples/loadbmp.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mipmap.dir/samples/loadbmp.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/loadbmp.c

CMakeFiles/mipmap.dir/samples/loadbmp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mipmap.dir/samples/loadbmp.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/loadbmp.c > CMakeFiles/mipmap.dir/samples/loadbmp.c.i

CMakeFiles/mipmap.dir/samples/loadbmp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mipmap.dir/samples/loadbmp.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/loadbmp.c -o CMakeFiles/mipmap.dir/samples/loadbmp.c.s

# Object files for target mipmap
mipmap_OBJECTS = \
"CMakeFiles/mipmap.dir/samples/mipmap/main.c.obj" \
"CMakeFiles/mipmap.dir/samples/loadbmp.c.obj"

# External object files for target mipmap
mipmap_EXTERNAL_OBJECTS =

mipmap.elf: CMakeFiles/mipmap.dir/samples/mipmap/main.c.obj
mipmap.elf: CMakeFiles/mipmap.dir/samples/loadbmp.c.obj
mipmap.elf: CMakeFiles/mipmap.dir/build.make
mipmap.elf: libGLdc.a
mipmap.elf: ../samples/mipmap/romdisk.o
mipmap.elf: CMakeFiles/mipmap.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable mipmap.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mipmap.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mipmap.dir/build: mipmap.elf

.PHONY : CMakeFiles/mipmap.dir/build

CMakeFiles/mipmap.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mipmap.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mipmap.dir/clean

CMakeFiles/mipmap.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/mipmap.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mipmap.dir/depend

