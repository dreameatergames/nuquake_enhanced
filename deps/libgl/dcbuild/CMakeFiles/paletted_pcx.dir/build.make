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
include CMakeFiles/paletted_pcx.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/paletted_pcx.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/paletted_pcx.dir/flags.make

CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.obj: CMakeFiles/paletted_pcx.dir/flags.make
CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.obj: ../samples/paletted_pcx/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/paletted_pcx/main.c

CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/paletted_pcx/main.c > CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.i

CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/paletted_pcx/main.c -o CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.s

# Object files for target paletted_pcx
paletted_pcx_OBJECTS = \
"CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.obj"

# External object files for target paletted_pcx
paletted_pcx_EXTERNAL_OBJECTS =

paletted_pcx.elf: CMakeFiles/paletted_pcx.dir/samples/paletted_pcx/main.c.obj
paletted_pcx.elf: CMakeFiles/paletted_pcx.dir/build.make
paletted_pcx.elf: libGLdc.a
paletted_pcx.elf: ../samples/paletted_pcx/romdisk.o
paletted_pcx.elf: CMakeFiles/paletted_pcx.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable paletted_pcx.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/paletted_pcx.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/paletted_pcx.dir/build: paletted_pcx.elf

.PHONY : CMakeFiles/paletted_pcx.dir/build

CMakeFiles/paletted_pcx.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/paletted_pcx.dir/cmake_clean.cmake
.PHONY : CMakeFiles/paletted_pcx.dir/clean

CMakeFiles/paletted_pcx.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/paletted_pcx.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/paletted_pcx.dir/depend

