#include "Base.h"
#include "BaseFactory.h"

int
f ()
{
  return createBase().dataMember;
}
