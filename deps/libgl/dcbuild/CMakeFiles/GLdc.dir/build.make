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
include CMakeFiles/GLdc.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/GLdc.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/GLdc.dir/flags.make

CMakeFiles/GLdc.dir/containers/aligned_vector.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/containers/aligned_vector.c.obj: ../containers/aligned_vector.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/GLdc.dir/containers/aligned_vector.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/containers/aligned_vector.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/containers/aligned_vector.c

CMakeFiles/GLdc.dir/containers/aligned_vector.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/containers/aligned_vector.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/containers/aligned_vector.c > CMakeFiles/GLdc.dir/containers/aligned_vector.c.i

CMakeFiles/GLdc.dir/containers/aligned_vector.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/containers/aligned_vector.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/containers/aligned_vector.c -o CMakeFiles/GLdc.dir/containers/aligned_vector.c.s

CMakeFiles/GLdc.dir/containers/named_array.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/containers/named_array.c.obj: ../containers/named_array.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/GLdc.dir/containers/named_array.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/containers/named_array.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/containers/named_array.c

CMakeFiles/GLdc.dir/containers/named_array.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/containers/named_array.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/containers/named_array.c > CMakeFiles/GLdc.dir/containers/named_array.c.i

CMakeFiles/GLdc.dir/containers/named_array.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/containers/named_array.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/containers/named_array.c -o CMakeFiles/GLdc.dir/containers/named_array.c.s

CMakeFiles/GLdc.dir/containers/stack.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/containers/stack.c.obj: ../containers/stack.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/GLdc.dir/containers/stack.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/containers/stack.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/containers/stack.c

CMakeFiles/GLdc.dir/containers/stack.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/containers/stack.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/containers/stack.c > CMakeFiles/GLdc.dir/containers/stack.c.i

CMakeFiles/GLdc.dir/containers/stack.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/containers/stack.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/containers/stack.c -o CMakeFiles/GLdc.dir/containers/stack.c.s

CMakeFiles/GLdc.dir/GL/draw.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/draw.c.obj: ../GL/draw.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/GLdc.dir/GL/draw.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/draw.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/draw.c

CMakeFiles/GLdc.dir/GL/draw.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/draw.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/draw.c > CMakeFiles/GLdc.dir/GL/draw.c.i

CMakeFiles/GLdc.dir/GL/draw.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/draw.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/draw.c -o CMakeFiles/GLdc.dir/GL/draw.c.s

CMakeFiles/GLdc.dir/GL/error.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/error.c.obj: ../GL/error.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/GLdc.dir/GL/error.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/error.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/error.c

CMakeFiles/GLdc.dir/GL/error.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/error.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/error.c > CMakeFiles/GLdc.dir/GL/error.c.i

CMakeFiles/GLdc.dir/GL/error.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/error.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/error.c -o CMakeFiles/GLdc.dir/GL/error.c.s

CMakeFiles/GLdc.dir/GL/flush.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/flush.c.obj: ../GL/flush.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/GLdc.dir/GL/flush.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/flush.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/flush.c

CMakeFiles/GLdc.dir/GL/flush.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/flush.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/flush.c > CMakeFiles/GLdc.dir/GL/flush.c.i

CMakeFiles/GLdc.dir/GL/flush.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/flush.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/flush.c -o CMakeFiles/GLdc.dir/GL/flush.c.s

CMakeFiles/GLdc.dir/GL/fog.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/fog.c.obj: ../GL/fog.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/GLdc.dir/GL/fog.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/fog.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/fog.c

CMakeFiles/GLdc.dir/GL/fog.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/fog.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/fog.c > CMakeFiles/GLdc.dir/GL/fog.c.i

CMakeFiles/GLdc.dir/GL/fog.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/fog.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/fog.c -o CMakeFiles/GLdc.dir/GL/fog.c.s

CMakeFiles/GLdc.dir/GL/framebuffer.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/framebuffer.c.obj: ../GL/framebuffer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/GLdc.dir/GL/framebuffer.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/framebuffer.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/framebuffer.c

CMakeFiles/GLdc.dir/GL/framebuffer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/framebuffer.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/framebuffer.c > CMakeFiles/GLdc.dir/GL/framebuffer.c.i

CMakeFiles/GLdc.dir/GL/framebuffer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/framebuffer.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/framebuffer.c -o CMakeFiles/GLdc.dir/GL/framebuffer.c.s

CMakeFiles/GLdc.dir/GL/glu.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/glu.c.obj: ../GL/glu.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/GLdc.dir/GL/glu.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/glu.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/glu.c

CMakeFiles/GLdc.dir/GL/glu.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/glu.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/glu.c > CMakeFiles/GLdc.dir/GL/glu.c.i

CMakeFiles/GLdc.dir/GL/glu.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/glu.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/glu.c -o CMakeFiles/GLdc.dir/GL/glu.c.s

CMakeFiles/GLdc.dir/GL/immediate.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/immediate.c.obj: ../GL/immediate.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/GLdc.dir/GL/immediate.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/immediate.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/immediate.c

CMakeFiles/GLdc.dir/GL/immediate.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/immediate.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/immediate.c > CMakeFiles/GLdc.dir/GL/immediate.c.i

CMakeFiles/GLdc.dir/GL/immediate.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/immediate.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/immediate.c -o CMakeFiles/GLdc.dir/GL/immediate.c.s

CMakeFiles/GLdc.dir/GL/lighting.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/lighting.c.obj: ../GL/lighting.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/GLdc.dir/GL/lighting.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/lighting.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/lighting.c

CMakeFiles/GLdc.dir/GL/lighting.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/lighting.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/lighting.c > CMakeFiles/GLdc.dir/GL/lighting.c.i

CMakeFiles/GLdc.dir/GL/lighting.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/lighting.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/lighting.c -o CMakeFiles/GLdc.dir/GL/lighting.c.s

CMakeFiles/GLdc.dir/GL/matrix.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/matrix.c.obj: ../GL/matrix.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object CMakeFiles/GLdc.dir/GL/matrix.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/matrix.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/matrix.c

CMakeFiles/GLdc.dir/GL/matrix.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/matrix.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/matrix.c > CMakeFiles/GLdc.dir/GL/matrix.c.i

CMakeFiles/GLdc.dir/GL/matrix.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/matrix.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/matrix.c -o CMakeFiles/GLdc.dir/GL/matrix.c.s

CMakeFiles/GLdc.dir/GL/state.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/state.c.obj: ../GL/state.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object CMakeFiles/GLdc.dir/GL/state.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/state.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/state.c

CMakeFiles/GLdc.dir/GL/state.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/state.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/state.c > CMakeFiles/GLdc.dir/GL/state.c.i

CMakeFiles/GLdc.dir/GL/state.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/state.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/state.c -o CMakeFiles/GLdc.dir/GL/state.c.s

CMakeFiles/GLdc.dir/GL/texture.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/texture.c.obj: ../GL/texture.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object CMakeFiles/GLdc.dir/GL/texture.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/texture.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/texture.c

CMakeFiles/GLdc.dir/GL/texture.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/texture.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/texture.c > CMakeFiles/GLdc.dir/GL/texture.c.i

CMakeFiles/GLdc.dir/GL/texture.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/texture.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/texture.c -o CMakeFiles/GLdc.dir/GL/texture.c.s

CMakeFiles/GLdc.dir/GL/util.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/util.c.obj: ../GL/util.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building C object CMakeFiles/GLdc.dir/GL/util.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/util.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/util.c

CMakeFiles/GLdc.dir/GL/util.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/util.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/util.c > CMakeFiles/GLdc.dir/GL/util.c.i

CMakeFiles/GLdc.dir/GL/util.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/util.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/util.c -o CMakeFiles/GLdc.dir/GL/util.c.s

CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.obj: ../GL/yalloc/yalloc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Building C object CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/yalloc/yalloc.c

CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/yalloc/yalloc.c > CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.i

CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/yalloc/yalloc.c -o CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.s

CMakeFiles/GLdc.dir/version.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/version.c.obj: version.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_17) "Building C object CMakeFiles/GLdc.dir/version.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/version.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/version.c

CMakeFiles/GLdc.dir/version.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/version.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/version.c > CMakeFiles/GLdc.dir/version.c.i

CMakeFiles/GLdc.dir/version.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/version.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/version.c -o CMakeFiles/GLdc.dir/version.c.s

CMakeFiles/GLdc.dir/GL/platforms/sh4.c.obj: CMakeFiles/GLdc.dir/flags.make
CMakeFiles/GLdc.dir/GL/platforms/sh4.c.obj: ../GL/platforms/sh4.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_18) "Building C object CMakeFiles/GLdc.dir/GL/platforms/sh4.c.obj"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/GLdc.dir/GL/platforms/sh4.c.obj   -c /home/dreamdev/projects/kos/libraries/GLdc/GL/platforms/sh4.c

CMakeFiles/GLdc.dir/GL/platforms/sh4.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLdc.dir/GL/platforms/sh4.c.i"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dreamdev/projects/kos/libraries/GLdc/GL/platforms/sh4.c > CMakeFiles/GLdc.dir/GL/platforms/sh4.c.i

CMakeFiles/GLdc.dir/GL/platforms/sh4.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLdc.dir/GL/platforms/sh4.c.s"
	/opt/toolchains/dc/sh-elf/bin/sh-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dreamdev/projects/kos/libraries/GLdc/GL/platforms/sh4.c -o CMakeFiles/GLdc.dir/GL/platforms/sh4.c.s

# Object files for target GLdc
GLdc_OBJECTS = \
"CMakeFiles/GLdc.dir/containers/aligned_vector.c.obj" \
"CMakeFiles/GLdc.dir/containers/named_array.c.obj" \
"CMakeFiles/GLdc.dir/containers/stack.c.obj" \
"CMakeFiles/GLdc.dir/GL/draw.c.obj" \
"CMakeFiles/GLdc.dir/GL/error.c.obj" \
"CMakeFiles/GLdc.dir/GL/flush.c.obj" \
"CMakeFiles/GLdc.dir/GL/fog.c.obj" \
"CMakeFiles/GLdc.dir/GL/framebuffer.c.obj" \
"CMakeFiles/GLdc.dir/GL/glu.c.obj" \
"CMakeFiles/GLdc.dir/GL/immediate.c.obj" \
"CMakeFiles/GLdc.dir/GL/lighting.c.obj" \
"CMakeFiles/GLdc.dir/GL/matrix.c.obj" \
"CMakeFiles/GLdc.dir/GL/state.c.obj" \
"CMakeFiles/GLdc.dir/GL/texture.c.obj" \
"CMakeFiles/GLdc.dir/GL/util.c.obj" \
"CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.obj" \
"CMakeFiles/GLdc.dir/version.c.obj" \
"CMakeFiles/GLdc.dir/GL/platforms/sh4.c.obj"

# External object files for target GLdc
GLdc_EXTERNAL_OBJECTS =

libGLdc.a: CMakeFiles/GLdc.dir/containers/aligned_vector.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/containers/named_array.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/containers/stack.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/draw.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/error.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/flush.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/fog.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/framebuffer.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/glu.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/immediate.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/lighting.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/matrix.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/state.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/texture.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/util.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/yalloc/yalloc.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/version.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/GL/platforms/sh4.c.obj
libGLdc.a: CMakeFiles/GLdc.dir/build.make
libGLdc.a: CMakeFiles/GLdc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_19) "Linking C static library libGLdc.a"
	$(CMAKE_COMMAND) -P CMakeFiles/GLdc.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/GLdc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/GLdc.dir/build: libGLdc.a

.PHONY : CMakeFiles/GLdc.dir/build

CMakeFiles/GLdc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/GLdc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/GLdc.dir/clean

CMakeFiles/GLdc.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/GLdc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/GLdc.dir/depend

