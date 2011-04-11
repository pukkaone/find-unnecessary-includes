# Find Unnecessary Includes

Given a C++ translation unit that successfully compiles, this tool lists
unnecessary or optional `#include` directives in the main source file.  Nested
`#include` directives in the files included from the main source file are not
considered.

Definition: An `#include` directive is *necessary* if removing it will cause a
compile error.

Definition: An `#include` directive is *optional* if removing it (along with
possibly adding other `#include` directives) will not cause a compile error.


## Build Instructions


### Prerequisites

* Git
* CMake 2.8 or better


### Check out source

After cloning the project Git repository, run the commands:

    git submodule init
    git submodule update


### Build on UNIX-like systems

Run the commands:

    mkdir build
    cd build
    cmake ..
    make


### Build using Visual C++ 2008

Run the commands:

    mkdir build
    cd build
    cmake -G "Visual Studio 9 2008" ..

In Visual C++, open the `find-unnecessary-includes.sln` solution.  Build the
`ALL_BUILD` project.
