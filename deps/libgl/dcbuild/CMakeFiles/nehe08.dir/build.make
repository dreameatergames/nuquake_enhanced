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
include CMakeFiles/nehe08.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/nehe08.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/nehe08.dir/flags.make

CMakeFiles/nehe08.dir/samples/nehe08/main.c.obj: CMakeFiles/nehe08.dir/flags.make
CMakeFiles/nehe08.dir/samples/nehe08/main.c.obj: ../samples/nehe08/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/nehe08.dir/samples/nehe08/main.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nehe08.dir/samples/nehe08/main.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe08/main.c

CMakeFiles/nehe08.dir/samples/nehe08/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nehe08.dir/samples/nehe08/main.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe08/main.c > CMakeFiles/nehe08.dir/samples/nehe08/main.c.i

CMakeFiles/nehe08.dir/samples/nehe08/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nehe08.dir/samples/nehe08/main.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe08/main.c -o CMakeFiles/nehe08.dir/samples/nehe08/main.c.s

CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.obj: CMakeFiles/nehe08.dir/flags.make
CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.obj: ../samples/nehe08/pvr-texture.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe08/pvr-texture.c

CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe08/pvr-texture.c > CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.i

CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe08/pvr-texture.c -o CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.s

# Object files for target nehe08
nehe08_OBJECTS = \
"CMakeFiles/nehe08.dir/samples/nehe08/main.c.obj" \
"CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.obj"

# External object files for target nehe08
nehe08_EXTERNAL_OBJECTS =

nehe08.elf: CMakeFiles/nehe08.dir/samples/nehe08/main.c.obj
nehe08.elf: CMakeFiles/nehe08.dir/samples/nehe08/pvr-texture.c.obj
nehe08.elf: CMakeFiles/nehe08.dir/build.make
nehe08.elf: libGLdc.a
nehe08.elf: ../samples/nehe08/romdisk.o
nehe08.elf: CMakeFiles/nehe08.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable nehe08.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nehe08.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/nehe08.dir/build: nehe08.elf

.PHONY : CMakeFiles/nehe08.dir/build

CMakeFiles/nehe08.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/nehe08.dir/cmake_clean.cmake
.PHONY : CMakeFiles/nehe08.dir/clean

CMakeFiles/nehe08.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/nehe08.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/nehe08.dir/depend

