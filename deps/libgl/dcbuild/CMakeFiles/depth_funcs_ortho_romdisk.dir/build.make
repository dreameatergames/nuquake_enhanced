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

# Utility rule file for depth_funcs_ortho_romdisk.

# Include the progress variables for this target.
include CMakeFiles/depth_funcs_ortho_romdisk.dir/progress.make

CMakeFiles/depth_funcs_ortho_romdisk: ../samples/depth_funcs_ortho/romdisk.o


../samples/depth_funcs_ortho/romdisk.o: ../samples/depth_funcs_ortho/romdisk.img
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating ../samples/depth_funcs_ortho/romdisk.o"
	cd /home/dreamdev/projects/kos/libraries/GLdc/samples/depth_funcs_ortho && /home/dreamdev/projects/kos/kallistios/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

../samples/depth_funcs_ortho/romdisk.img:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Generating ../samples/depth_funcs_ortho/romdisk.img"
	/home/dreamdev/projects/kos/kallistios/utils/genromfs/genromfs -f /home/dreamdev/projects/kos/libraries/GLdc/samples/depth_funcs_ortho/romdisk.img -d /home/dreamdev/projects/kos/libraries/GLdc/samples/depth_funcs_ortho/romdisk -v

depth_funcs_ortho_romdisk: CMakeFiles/depth_funcs_ortho_romdisk
depth_funcs_ortho_romdisk: ../samples/depth_funcs_ortho/romdisk.o
depth_funcs_ortho_romdisk: ../samples/depth_funcs_ortho/romdisk.img
depth_funcs_ortho_romdisk: CMakeFiles/depth_funcs_ortho_romdisk.dir/build.make

.PHONY : depth_funcs_ortho_romdisk

# Rule to build all files generated by this target.
CMakeFiles/depth_funcs_ortho_romdisk.dir/build: depth_funcs_ortho_romdisk

.PHONY : CMakeFiles/depth_funcs_ortho_romdisk.dir/build

CMakeFiles/depth_funcs_ortho_romdisk.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/depth_funcs_ortho_romdisk.dir/cmake_clean.cmake
.PHONY : CMakeFiles/depth_funcs_ortho_romdisk.dir/clean

CMakeFiles/depth_funcs_ortho_romdisk.dir/depend:
	cd /home/dreamdev/projects/kos/libraries/GLdc/dcbuild && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild /home/dreamdev/projects/kos/libraries/GLdc/dcbuild/CMakeFiles/depth_funcs_ortho_romdisk.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/depth_funcs_ortho_romdisk.dir/depend
