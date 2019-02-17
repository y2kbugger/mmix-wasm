#include <windows.h>
#include "winopt.h"

extern int xpos, ypos;

regtable regtab= {{"xpos",&xpos,TYPE_DWORD},{"ypos",&ypos,TYPE_DWORD},{NULL,NULL,0}};


void set_pos_key(HWND hWnd,char *program)
{ set_xypos(hWnd);
  write_regtab(program);
}

void get_pos_key(int *xpos, int *ypos, char *program)
{
  read_regtab(program);
  get_xypos();
}

