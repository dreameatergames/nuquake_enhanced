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
include CMakeFiles/zclip.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/zclip.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/zclip.dir/flags.make

CMakeFiles/zclip.dir/samples/zclip/main.c.obj: CMakeFiles/zclip.dir/flags.make
CMakeFiles/zclip.dir/samples/zclip/main.c.obj: ../samples/zclip/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/zclip.dir/samples/zclip/main.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/zclip.dir/samples/zclip/main.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/samples/zclip/main.c

CMakeFiles/zclip.dir/samples/zclip/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zclip.dir/samples/zclip/main.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/samples/zclip/main.c > CMakeFiles/zclip.dir/samples/zclip/main.c.i

CMakeFiles/zclip.dir/samples/zclip/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zclip.dir/samples/zclip/main.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/samples/zclip/main.c -o CMakeFiles/zclip.dir/samples/zclip/main.c.s

# Object files for target zclip
zclip_OBJECTS = \
"CMakeFiles/zclip.dir/samples/zclip/main.c.obj"

# External object files for target zclip
zclip_EXTERNAL_OBJECTS =

zclip.elf: CMakeFiles/zclip.dir/samples/zclip/main.c.obj
zclip.elf: CMakeFiles/zclip.dir/build.make
zclip.elf: libGLdc.a
zclip.elf: ../samples/zclip/romdisk.o
zclip.elf: CMakeFiles/zclip.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable zclip.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/zclip.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/zclip.dir/build: zclip.elf

.PHONY : CMakeFiles/zclip.dir/build

CMakeFiles/zclip.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/zclip.dir/cmake_clean.cmake
.PHONY : CMakeFiles/zclip.dir/clean

CMakeFiles/zclip.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/zclip.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/zclip.dir/depend

