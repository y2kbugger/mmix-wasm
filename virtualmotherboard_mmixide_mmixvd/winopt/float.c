#include <windows.h>
#include <stdio.h>
#include "winopt.h"
#include "float.h"
#pragma warning(disable : 4996)


/* get 64 bit float from string */
uint64_t f64_from_str(char *str)
{ union { uint64_t u; f64 f;} x;
  float f;
  sscanf(str," %f",&f);
  x.f=f;
  return x.u;
}

/* put a 64 bit float into a string with size number of characters, 
   return number of characters written. */
extern int f64_to_str(char *str, uint64_t f, int size)
{ int w;
  union { uint64_t u; f64 f;} x;
  double d;
  x.u=f;
  d=x.f;
  w=sprintf(str,"%*g",size,d); 
  return w;
}
/* convert between 32 and 64 bit floats */
extern uint64_t f64_from_f32(uint32_t f)
{ union { uint64_t u; f64 f;} x64;
  union { uint32_t u; f32 f;} x32;
  x32.u=f;
  x64.f=x32.f;
  return x64.u;
}
extern uint32_t f32_from_f64(uint64_t f)
{ union { uint64_t u; f64 f;} x64;
  union { uint32_t u; f32 f;} x32;
  x64.u=f;
  x32.f=(f32)x64.f;
  return x32.u;
}
/* convert between double and 64 bit floats */
extern uint64_t f64_from_double(double f)
{ union { uint64_t u; f64 f;} x64;
  x64.f=f;
  return x64.u;
}
extern double double_from_f64(uint64_t f)
{ union { uint64_t u; f64 f;} x64;
  x64.u=f;
  return x64.f;
}