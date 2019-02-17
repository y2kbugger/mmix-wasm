#include <windows.h>

void fatal_error(char *msg)
{   MessageBox(NULL,msg,"FATAL ERROR",MB_OK);
	exit(1);
}