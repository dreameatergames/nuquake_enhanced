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
include CMakeFiles/nehe06.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/nehe06.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/nehe06.dir/flags.make

CMakeFiles/nehe06.dir/samples/nehe06/main.c.obj: CMakeFiles/nehe06.dir/flags.make
CMakeFiles/nehe06.dir/samples/nehe06/main.c.obj: ../samples/nehe06/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/nehe06.dir/samples/nehe06/main.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nehe06.dir/samples/nehe06/main.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe06/main.c

CMakeFiles/nehe06.dir/samples/nehe06/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nehe06.dir/samples/nehe06/main.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe06/main.c > CMakeFiles/nehe06.dir/samples/nehe06/main.c.i

CMakeFiles/nehe06.dir/samples/nehe06/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nehe06.dir/samples/nehe06/main.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe06/main.c -o CMakeFiles/nehe06.dir/samples/nehe06/main.c.s

CMakeFiles/nehe06.dir/samples/loadbmp.c.obj: CMakeFiles/nehe06.dir/flags.make
CMakeFiles/nehe06.dir/samples/loadbmp.c.obj: ../samples/loadbmp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/nehe06.dir/samples/loadbmp.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nehe06.dir/samples/loadbmp.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/loadbmp.c

CMakeFiles/nehe06.dir/samples/loadbmp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nehe06.dir/samples/loadbmp.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/loadbmp.c > CMakeFiles/nehe06.dir/samples/loadbmp.c.i

CMakeFiles/nehe06.dir/samples/loadbmp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nehe06.dir/samples/loadbmp.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/loadbmp.c -o CMakeFiles/nehe06.dir/samples/loadbmp.c.s

# Object files for target nehe06
nehe06_OBJECTS = \
"CMakeFiles/nehe06.dir/samples/nehe06/main.c.obj" \
"CMakeFiles/nehe06.dir/samples/loadbmp.c.obj"

# External object files for target nehe06
nehe06_EXTERNAL_OBJECTS =

nehe06.elf: CMakeFiles/nehe06.dir/samples/nehe06/main.c.obj
nehe06.elf: CMakeFiles/nehe06.dir/samples/loadbmp.c.obj
nehe06.elf: CMakeFiles/nehe06.dir/build.make
nehe06.elf: libGLdc.a
nehe06.elf: ../samples/nehe06/romdisk.o
nehe06.elf: CMakeFiles/nehe06.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable nehe06.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nehe06.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/nehe06.dir/build: nehe06.elf

.PHONY : CMakeFiles/nehe06.dir/build

CMakeFiles/nehe06.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/nehe06.dir/cmake_clean.cmake
.PHONY : CMakeFiles/nehe06.dir/clean

CMakeFiles/nehe06.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/nehe06.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/nehe06.dir/depend

