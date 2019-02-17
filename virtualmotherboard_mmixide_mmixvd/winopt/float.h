#ifndef _FLOAT_H_
#define _FLOAT_H_
#include "uint64.h"


/* decide which kind of Float implementation you need */
#define HOST_FLOAT

#ifdef HOST_FLOAT

/* define 32 and 64 bit host floating point types */
typedef float f32;
/*
typedef short float f32;
typedef double f32;
*/
static char f32_dummy[sizeof(f32)==4];

typedef double f64;
/*
typedef float f64;
typedef long double f64;
typedef long long double f64;
*/

static char f64_dummy[sizeof(f64)==8];


#endif

/* get 64 bit float from string */
extern uint64_t f64_from_str(char *str);
/* put a 64 bit float into a string with size number of characters, 
   return number of characters written. */
extern int f64_to_str(char *str, uint64_t f, int size);

/* convert between 32 and 64 bit floats */
extern uint64_t f64_from_f32(uint32_t f);
extern uint32_t f32_from_f64(uint64_t f);

/* convert between double and 64 bit floats */
extern uint64_t f64_from_double(double f);
extern double double_from_f64(uint64_t f);


#endif
