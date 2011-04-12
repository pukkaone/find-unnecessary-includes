#include "Base.h"

namespace {

template<typename V>
V
max (V v1, V v2)
{
  return (v1 > v2) ? v1 : v2;
}

int i = max(1, 2);

}//namespace
