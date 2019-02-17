
#include <windows.h>
#include "winopt.h"
#include "error.h"
#include "option.h"

void win32_param_init(void)
{ int argc;
  char *argv[MAXARG];
  int i;

  option_defaults();
  argc=mk_argv(argv,GetCommandLine(),TRUE);
  do_program(argv[0]);
  if (do_define(argv[1])) i=2; else i=1;
  read_regtab(defined);
  parse_commandline(argc-i, argv+i);
  get_xypos();
  if (vmb_verbose_flag) vmb_debug_mask=0;
  SetWindowText(hMainWnd,defined);
}

