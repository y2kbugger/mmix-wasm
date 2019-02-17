#include "intchar.h"

void inttochar(int val, unsigned char buffer[4])
{
	buffer[3] =  val;
	val = val >> 8;
	buffer[2] =  val;
	val = val >> 8;
	buffer[1] =  val;
	val = val >> 8;
	buffer[0] =  val;
}

int chartoint(const unsigned char buffer[4])
{
  int val;
  val = buffer[0];
  val = val << 8;
  val = buffer[1] | val;
  val = val << 8;
  val = buffer[2] | val;
  val = val << 8;
  val = buffer[3] | val;
  return val;
}

#if 0
void shorttochar(int val, unsigned char buffer[2])
{
	buffer[1] =  val;
	val = val >> 8;
	buffer[0] =  val;
}
#endif
