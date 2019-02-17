#include <windows.h>


#define BB_HEIGHT (24+8)
extern void bb_init(HINSTANCE hInst);
/* call this before using button bars */
extern HWND bb_CreateButtonBar(LPCTSTR lpWindowName, DWORD dwStyle, 
						int x, int y, int nWidth, int nHeight,
                        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
/* call this to create a button bar window. Parameters and Return values are like CreateWindow. 
   call DestroyWindow on the return value to destroy this window again */


extern HWND bb_CreateButton(HWND hButtonBar,HANDLE hImg, int command, unsigned char group, unsigned char id, 
					 unsigned char active, unsigned char visible, char *tip);
/* this creates a button with the Icon on its face and adds it to the button bar
   different buttons should have different ids, if id == 0 an id will be chosen 
   Call DestroyWindow on the return value to remove this button again */   


extern void bb_set_group(HWND hButtonBar,unsigned char group, int active, int visible);
/* this changes active/visible status of the whole group */
extern void bb_set_id(HWND hButtonBar,unsigned char id, int active, int visible);
/* this changes active/visible status of one button */