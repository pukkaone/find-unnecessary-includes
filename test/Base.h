#ifndef BASE_H
#define BASE_H

typedef int Identifier;

enum Season { WINTER, SPRING, SUMMER, FALL };

class Base {
public:
  int dataMember;
  void memberFunction();
};

extern Base base;

template<typename V>
V
max (V x, V y)
{
  return (x > y) ? x : y;
}

void function();

#endif
