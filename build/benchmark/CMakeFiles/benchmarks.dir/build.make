# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/cris/Desktop/Bhavish/Graphiti

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/cris/Desktop/Bhavish/Graphiti/build

# Utility rule file for benchmarks.

# Include any custom commands dependencies for this target.
include benchmark/CMakeFiles/benchmarks.dir/compiler_depend.make

# Include the progress variables for this target.
include benchmark/CMakeFiles/benchmarks.dir/progress.make

benchmarks: benchmark/CMakeFiles/benchmarks.dir/build.make
.PHONY : benchmarks

# Rule to build all files generated by this target.
benchmark/CMakeFiles/benchmarks.dir/build: benchmarks
.PHONY : benchmark/CMakeFiles/benchmarks.dir/build

benchmark/CMakeFiles/benchmarks.dir/clean:
	cd /home/cris/Desktop/Bhavish/Graphiti/build/benchmark && $(CMAKE_COMMAND) -P CMakeFiles/benchmarks.dir/cmake_clean.cmake
.PHONY : benchmark/CMakeFiles/benchmarks.dir/clean

benchmark/CMakeFiles/benchmarks.dir/depend:
	cd /home/cris/Desktop/Bhavish/Graphiti/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/cris/Desktop/Bhavish/Graphiti /home/cris/Desktop/Bhavish/Graphiti/benchmark /home/cris/Desktop/Bhavish/Graphiti/build /home/cris/Desktop/Bhavish/Graphiti/build/benchmark /home/cris/Desktop/Bhavish/Graphiti/build/benchmark/CMakeFiles/benchmarks.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : benchmark/CMakeFiles/benchmarks.dir/depend

