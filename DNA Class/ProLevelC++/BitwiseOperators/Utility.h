//******************************************//
// Utility code for project
//******************************************//

#ifndef _UTILITY_H_DEFINED_
#define _UTILITY_H_DEFINED_

#include <type_traits> // Needed for underlying_type_t below
 
// This is a "macro". The next few lines are actually a single line - the \ continues the line. It defines the
// word ENUM_FLAG_OPERATORS to mean all the following code. It's basically a way of pasting in this code by
// just typing a single word. Macros should be avoided mostly. However, they can be useful to save typing where
// there is no natural C++ way to do the equivalent.
//
// These lines allow us to use bitwise operators on enum values without lots of casting. See BitwiseOperators.cpp for the example
#define ENUM_FLAG_OPERATORS(E) \
using E_INT = std::underlying_type_t<E>; \
inline E  operator& (E a, E b)   { return static_cast<E>(static_cast<E_INT>(a) & static_cast<E_INT>(b)); } \
inline E  operator| (E a, E b)   { return static_cast<E>(static_cast<E_INT>(a) | static_cast<E_INT>(b)); } \
inline E  operator^ (E a, E b)   { return static_cast<E>(static_cast<E_INT>(a) ^ static_cast<E_INT>(b)); } \
inline E  operator~ (E a)        { return static_cast<E>(~static_cast<E_INT>(a)); } \
inline E& operator&=(E& a, E b)  { a = a & b; return a; } \
inline E& operator|=(E& a, E b)  { a = a | b; return a; } \
inline E& operator^=(E& a, E b)  { a = a ^ b; return a; } \
inline bool operator==(E a, E_INT b)  { return static_cast<E_INT>(a) == b; } \
inline bool operator==(E_INT a, E b)  { return static_cast<E_INT>(b) == a; } \
inline bool operator!=(E a, E_INT b)  { return static_cast<E_INT>(a) != b; } \
inline bool operator!=(E_INT a, E b)  { return static_cast<E_INT>(b) != a; } \
inline E_INT operator!(E a)  { return !static_cast<E_INT>(a); }


#endif