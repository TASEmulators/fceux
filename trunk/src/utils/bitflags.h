#ifndef BITFLAGS_H
#define BITFLAGS_H

// Flag test / set / clear macros for convenience and clarity
#define FL_TEST(var, mask) (var & mask)
#define FL_SET(var, mask) (var |= mask)
#define FL_CLEAR(var, mask) (var &= ~mask)
#define FL_FROMBOOL(var, mask, b) (var = (b)? var|mask:var&(~mask))

#endif // BITFLAGS_H
