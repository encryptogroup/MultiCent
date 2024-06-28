# Graphiti

This directory contains the implementation of the Graphiti framework for secure graph computation. The protocol is implemented in C++17 and [CMake](https://cmake.org/) is used as the build system.

## External Dependencies
The following libraries need to be installed separately and should be available to the build system and compiler.

- [GMP](https://gmplib.org/)
- [NTL](https://www.shoup.net/ntl/) (11.0.0 or later)
- [Boost](https://www.boost.org/) (1.72.0 or later)
- [Nlohmann JSON](https://github.com/nlohmann/json)
- [EMP Tool](https://github.com/emp-toolkit/emp-tool)

### Docker
All required dependencies to compile and run the project are available through the docker image.
To build and run the docker image, execute the following commands from the root directory of the repository:

```sh
# Build the Asterisk Docker image.
#
# Building the Docker image requires at least 4GB RAM. This needs to be set 
# explicitly in case of Windows and MacOS.
docker build -t asterisk .

# Create and run a container.
#
# This should start the shell from within the container.
docker run -it -v $PWD:/code asterisk

# The following command changes the working directory to the one containing the 
# source code and should be run on the shell started using the previous command.
cd /code
```

## Compilation
The project uses [CMake](https://cmake.org/) for building the source code. 
To compile, run the following commands from the root directory of the repository:

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

make benchmarks
```
