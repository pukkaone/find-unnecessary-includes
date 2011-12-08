# Find Unnecessary Includes

Given a C++ translation unit that compiles without errors, this tool lists
unnecessary `#include` directives in the main source file.  Nested
`#include` directives in the files included from the main source file are not
considered.


### Unnecessary #include directives

Definition: An `#include` directive is *unnecessary* if the translation unit
compiles without errors after you remove it.


### Replaceable #include directives

Definition: An `#include` directive is *replaceable* if it can be replaced with
one or more `#include` directives for different headers without causing a
compile error.

For example, suppose the header file `Base.h` defines a base class.  The header
file `Derived.h` defines a class deriving from the base class, so it includes
`Base.h`.  The main source file includes `Derived.h` but does not use any
symbols defined in `Derived.h` and uses only symbols defined in `Base.h`.  The
tool will say `Derived.h` is replaceable, and suggest the main source file
directly include `Base.h` instead.


## Build Instructions


### Prerequisites

* Git
* CMake 2.8 or better


### Check out source

The tool uses clang to preprocess and parse C++ source code.  Download the llvm
and clang sources to the working tree by running the commands:

    git submodule init
    git submodule update


### Build on UNIX-like systems

Run the commands:

    mkdir build
    cd build
    cmake ..
    make

To run the tests:

    make test


### Build using Visual C++ 2010

Run the commands:

    mkdir build
    cd build
    cmake -G "Visual Studio 10" ..

In Visual C++, open the `find-unnecessary-includes.sln` solution.  Build the
`find-unnecessary-includes` project.

To run the tests, build the `RUN_TESTS` project.
