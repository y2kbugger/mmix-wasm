#if 0
#include <stdlib.h> 
#include <ctype.h> 
#include <string.h> 
#include <signal.h> 
#include "libconfig.h"
#include <time.h>
#endif
#include <stdio.h> 
#include <setjmp.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "vmb.h"
#include "mmixlib/libtype.h"
#include "mmixlib/libglobals.h"
#include "mmixlib/mmixlib.h"
#include "mmix-bus.h"

char localhost[]="localhost";
char *host=localhost;
int port=9002;
device_info vmb;

int main(int argc,char *argv[])
{
	char**boot_argv;
	int boot_argc;
	mmix_lib_initialize();
	g[255].h= 0;
	g[255].l= setjmp(mmix_exit);
	if(g[255].l!=0)
		goto end_simulation;

	myself= argv[0];
	for(cur_arg= argv+1;*cur_arg&&(*cur_arg)[0]=='-';cur_arg++)
		scan_option(*cur_arg+1,true);
	argc-= (int)(cur_arg-argv);

	init_mmix_bus(host,port,"MMIX CPU");

	if (!vmb.connected) {fprintf(stderr,"Not connected\n"); return 0;}
	if (vmb.power) vmb_raise_reset(&vmb);
	mmix_initialize();

	boot_argv= cur_arg;
	boot_argc= argc;
boot:
	argc= boot_argc;
	cur_arg= boot_argv;

	vmb.reset_flag=0;
	printf("Power...");
	while (!vmb.power)
	{  vmb_wait_for_power(&vmb);
	if (!vmb.connected){ fprintf(stderr,"Power but not connected"); return 0;}
	}
	printf("ON\n");
#ifdef WIN32
	Sleep(50); /* give all devices some time to power up before loading the application */
#else
	usleep(50000);
#endif
	mmix_boot();
  

	if (cur_arg!=NULL && cur_arg[0]!=0) mmix_load_file(*cur_arg);
	mmix_commandline(argc,cur_arg);

	while(vmb.connected){
		if(interrupt&&!breakpoint)breakpoint= interacting= true,interrupt= false;
		else if(!(inst_ptr.h&sign_bit)||show_operating_system||
			(inst_ptr.h==0x80000000&&inst_ptr.l==0))
		{breakpoint= false;
		if(interacting){
			if(!mmix_interact())goto end_simulation;
		}
		}
		if(halted)break;
		do
		{if(!resuming)
			mmix_fetch_instruction();
		mmix_perform_instruction();
		mmix_trace();
		mmix_dynamic_trap();
		if(resuming&&op!=RESUME)resuming= false;
		}while((vmb.connected && vmb.power && !vmb.reset_flag &&
			!interrupt && !breakpoint) || 
			resuming);
		if(interact_after_break)
			interacting= true,interact_after_break= false;
		if(!vmb.power || vmb.reset_flag || g[rQ].l&g[rK].l&RE_BIT)
		{breakpoint= true;
		goto boot;
		}
	}
end_simulation:if(profiling)mmix_profile();
	if(interacting||profiling||showing_stats)show_stats(true);
	mmix_finalize();
	return g[255].l;
}
