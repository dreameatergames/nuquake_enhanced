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
include CMakeFiles/nehe02de.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/nehe02de.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/nehe02de.dir/flags.make

CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.obj: CMakeFiles/nehe02de.dir/flags.make
CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.obj: ../samples/nehe02de/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe02de/main.c

CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe02de/main.c > CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.i

CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/nehe02de/main.c -o CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.s

# Object files for target nehe02de
nehe02de_OBJECTS = \
"CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.obj"

# External object files for target nehe02de
nehe02de_EXTERNAL_OBJECTS =

nehe02de.elf: CMakeFiles/nehe02de.dir/samples/nehe02de/main.c.obj
nehe02de.elf: CMakeFiles/nehe02de.dir/build.make
nehe02de.elf: libGLdc.a
nehe02de.elf: ../samples/nehe02de/romdisk.o
nehe02de.elf: CMakeFiles/nehe02de.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable nehe02de.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nehe02de.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/nehe02de.dir/build: nehe02de.elf

.PHONY : CMakeFiles/nehe02de.dir/build

CMakeFiles/nehe02de.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/nehe02de.dir/cmake_clean.cmake
.PHONY : CMakeFiles/nehe02de.dir/clean

CMakeFiles/nehe02de.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/nehe02de.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/nehe02de.dir/depend

