# Find Unnecessary Includes

Given a C++ translation unit that successfully compiles, this tool lists
unnecessary or optional `#include` directives in the main source file.  Nested
`#include` directives in the files included from the main source file are not
considered.

Definition: An `#include` directive is *necessary* if its removal will cause a
compile error.

Definition: An `#include` directive is *optional* if its removal (and possibly
accompanied by the addition of other `#include` directives) will not cause
a compile error.
