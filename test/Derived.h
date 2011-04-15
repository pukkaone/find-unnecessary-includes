#ifndef DERIVED_H
#define DERIVED_H

#include "Base.h"
#include "Base.h"

class Derived: public Base
{
public:
  // hides memberFunction declared in Base
  void memberFunction();
};

#endif
