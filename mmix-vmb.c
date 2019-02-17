#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "abstime.h"
#include "libconfig.h"
#include <time.h>
#include "libtype.h"
#include "libglobals.h"
#include "mmixlib.h"
#include "address.h"
#include "mmix-bus.h"
#include "breaks.h"
#include "vmb.h"
device_info vmb = {0};
char localhost[]="localhost";
int port=9002; /* on which port to connect to the bus */
char *host=localhost; /* on which host to connect to the bus */


int main(argc,argv)
  int argc;
  char *argv[];
{
  char **boot_cur_arg;
  int boot_argc;
  g[255].h=0;
  g[255].l=setjmp(mmix_exit);
  if (g[255].l!=0)
   goto end_simulation;
  mmix_lib_initialize();
  mmix_commandline(argc,argv);

  if (host==NULL) panic("No Bus given. Use Option -B[host:]port");
  init_mmix_bus(host,port,"MMIX CPU");

  mmix_initialize();
 
  boot_cur_arg = cur_arg;
  boot_argc = argc;
  if (vmb.power)  
    vmb_raise_reset(&vmb);

boot:

  argc = boot_argc;
  cur_arg = boot_cur_arg;
  
  fprintf(stderr,"Power...");
  while (!vmb.power)
  {  vmb_wait_for_power(&vmb);
     if (!vmb.connected) goto end_simulation;
  }
  fprintf(stderr,"ON\n");
  MMIX_CLEAR_DVTC();
  MMIX_CLEAR_IVTC();
  MMIX_CLEAR_DCACHE();
  MMIX_CLEAR_ICACHE();

  mmix_boot();
  mmix_load_file(*cur_arg);
  mmix_commandline(argc, argv);
  write_all_data_cache();
  clear_all_instruction_cache();
  if (interacting) mem_find(g[rWW])->bkpt|=exec_bit;

  while (true) {
    if (interrupt && !breakpoint) breakpoint=interacting=true, interrupt=false;
    else if (!(inst_ptr.h&sign_bit) || show_operating_system || 
          (inst_ptr.h==0x80000000 && inst_ptr.l==0))
    { breakpoint=false;
      if (interacting)
		if (!mmix_interact()) goto end_simulation;
    }
    if (halted) break;
    { if (!resuming)
        mmix_fetch_instruction();       
      mmix_perform_instruction();
      mmix_trace();
      mmix_dynamic_trap();
      if (resuming && op!=RESUME) resuming=false; 
      if (!resuming && (!vmb.power||!vmb.reset)) break; 
    } while (resuming || (!interrupt && !breakpoint));
    if (interact_after_break) 
       interacting=true, interact_after_break=false;
    if (!vmb.power||(g[rQ].l&g[rK].l&RE_BIT))
      goto boot;
  }
  end_simulation: if (profiling) mmix_profile();
  if (interacting || profiling || showing_stats) show_stats(true);
  mmix_finalize();
  mmix_lib_finalize();
  return g[255].l; /* provide rudimentary feedback for non-interactive runs */
}
