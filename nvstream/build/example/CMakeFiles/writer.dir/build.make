# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.3

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
CMAKE_COMMAND = /home/pradeep/installations/cmake-3.3.1-Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /home/pradeep/installations/cmake-3.3.1-Linux-x86_64/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pradeep/checkout/nvm-yuma/yuma/nvstream

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build

# Include any dependencies generated for this target.
include example/CMakeFiles/writer.dir/depend.make

# Include the progress variables for this target.
include example/CMakeFiles/writer.dir/progress.make

# Include the compile flags for this target's objects.
include example/CMakeFiles/writer.dir/flags.make

example/CMakeFiles/writer.dir/writer.cc.o: example/CMakeFiles/writer.dir/flags.make
example/CMakeFiles/writer.dir/writer.cc.o: ../example/writer.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object example/CMakeFiles/writer.dir/writer.cc.o"
	cd /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/writer.dir/writer.cc.o -c /home/pradeep/checkout/nvm-yuma/yuma/nvstream/example/writer.cc

example/CMakeFiles/writer.dir/writer.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/writer.dir/writer.cc.i"
	cd /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/pradeep/checkout/nvm-yuma/yuma/nvstream/example/writer.cc > CMakeFiles/writer.dir/writer.cc.i

example/CMakeFiles/writer.dir/writer.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/writer.dir/writer.cc.s"
	cd /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/pradeep/checkout/nvm-yuma/yuma/nvstream/example/writer.cc -o CMakeFiles/writer.dir/writer.cc.s

example/CMakeFiles/writer.dir/writer.cc.o.requires:

.PHONY : example/CMakeFiles/writer.dir/writer.cc.o.requires

example/CMakeFiles/writer.dir/writer.cc.o.provides: example/CMakeFiles/writer.dir/writer.cc.o.requires
	$(MAKE) -f example/CMakeFiles/writer.dir/build.make example/CMakeFiles/writer.dir/writer.cc.o.provides.build
.PHONY : example/CMakeFiles/writer.dir/writer.cc.o.provides

example/CMakeFiles/writer.dir/writer.cc.o.provides.build: example/CMakeFiles/writer.dir/writer.cc.o


# Object files for target writer
writer_OBJECTS = \
"CMakeFiles/writer.dir/writer.cc.o"

# External object files for target writer
writer_EXTERNAL_OBJECTS =

example/writer: example/CMakeFiles/writer.dir/writer.cc.o
example/writer: example/CMakeFiles/writer.dir/build.make
example/writer: src/libyuma.a
example/writer: example/CMakeFiles/writer.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable writer"
	cd /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/writer.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
example/CMakeFiles/writer.dir/build: example/writer

.PHONY : example/CMakeFiles/writer.dir/build

example/CMakeFiles/writer.dir/requires: example/CMakeFiles/writer.dir/writer.cc.o.requires

.PHONY : example/CMakeFiles/writer.dir/requires

example/CMakeFiles/writer.dir/clean:
	cd /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example && $(CMAKE_COMMAND) -P CMakeFiles/writer.dir/cmake_clean.cmake
.PHONY : example/CMakeFiles/writer.dir/clean

example/CMakeFiles/writer.dir/depend:
	cd /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pradeep/checkout/nvm-yuma/yuma/nvstream /home/pradeep/checkout/nvm-yuma/yuma/nvstream/example /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example /home/pradeep/checkout/nvm-yuma/yuma/nvstream/build/example/CMakeFiles/writer.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : example/CMakeFiles/writer.dir/depend

