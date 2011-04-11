# Find Unnecessary Includes

Definition: An `#include` directive is *necessary* if its removal will cause a
compile error.

Definition: An `#include` directive is *optional* if its removal (and possibly
accompanied by the addition of other `#include` directives) will not cause
a compile error.

Given a C++ translation unit that successfully compiles, this tool lists
necessary or optional `#include` directives in the main source file.  Only
`#include` directives in the main source file are considered.  Nested
`#include` directives are not considered.
