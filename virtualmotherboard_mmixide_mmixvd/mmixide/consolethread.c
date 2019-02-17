#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "vmb.h"
#include "error.h"

// maximum mumber of lines the output console should have

static const WORD MAX_CONSOLE_LINES = 500;
extern void vmb_atexit(void);


typedef struct {
	int (*proc)(void *);
	void *param; } console_params;

DWORD dwMMIXThreadId=0;

void mmix_exit(int returncode)
{ FreeConsole();
  vmb_atexit();
  ExitThread(returncode);
}


static DWORD WINAPI ConsoleThreadProc(LPVOID cp)
{ int hConHandle;
  HANDLE lStdHandle;
  CONSOLE_SCREEN_BUFFER_INFO coninfo;
  FILE *fp;
  FILE stdoutOld, stdinOld, stderrOld;
  int returncode;
  int (*proc)(void*) = ((console_params*)cp)->proc;
  void *param=((console_params*)cp)->param;
  if (AllocConsole()==0)
    vmb_debugi(VMB_DEBUG_FATAL,"Unable to allocate Console (%X)",GetLastError());
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&coninfo);
  coninfo.dwSize.Y = MAX_CONSOLE_LINES;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),coninfo.dwSize);

  lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "w" );
  stdoutOld=*stdout;
  *stdout = *fp;
  setvbuf( stdout, NULL, _IONBF, 0 );

  lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
  hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "r" );
  stdinOld=*stdin;
  *stdin = *fp;
  setvbuf( stdin, NULL, _IONBF, 0 );

  lStdHandle =  GetStdHandle(STD_ERROR_HANDLE);
  hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
  fp = _fdopen( hConHandle, "w" );
  stderrOld=*stderr;
  *stderr = *fp;
  setvbuf( stderr, NULL, _IONBF, 0 );

  returncode = (*proc)(param);
  *stderr=stderrOld;
  *stdin=stdinOld;
  *stdout=stdoutOld;

  FreeConsole();
  vmb_atexit();

  return returncode;
}

void ConsoleThread(int proc(void *), void *param)
{
  HANDLE h;
  static console_params cp;
  cp.param=param;
  cp.proc=proc;

  h = CreateThread(
			NULL,              // default security attributes
            0,                 // use default stack size  
            ConsoleThreadProc,        // thread function 
            &cp,             // argument to thread function 
            0,                 // use default creation flags 
            &dwMMIXThreadId);   // returns the thread identifier 
    CloseHandle(h);
}

