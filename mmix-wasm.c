#include "emscripten.h"
#include "mmixlib/abstime.h"
#include <setjmp.h>
#include "mmixlib/libconfig.h"
#include "mmixlib/libtype.h"
#include "mmixlib/libglobals.h"
#include "mmixlib/mmixlib.h"

EMSCRIPTEN_KEEPALIVE
int mmixal_wasm() {
    return mmixal("y2k.mms", "y2k.mmo", "y2k.mml");
}

EMSCRIPTEN_KEEPALIVE
int mmix_wasm(int argc, char*argv[])
{
/* char**boot_cur_arg; */
/* int boot_argc; */
mmix_lib_initialize();
g[255].h= 0;
g[255].l= setjmp(mmix_exit);
if(g[255].l!=0)
goto end_simulation;
/*170:*/
#line 2930 "./mmixware/mmix-sim.w"

/* myself= argv[0]; */
/* for(cur_arg= argv+1;*cur_arg&&(*cur_arg)[0]=='-';cur_arg++) */
/* scan_option(*cur_arg+1,true); */
#line 2452 "./mmixlib.ch"
MMIX_NO_FILE;
/* argc-= (int)(cur_arg-argv); */
#line 2936 "./mmixware/mmix-sim.w"

/*:170*/
#line 2393 "./mmixlib.ch"
;

mmix_initialize();

/* boot_cur_arg= cur_arg; */
/* boot_argc= argc; */
boot:
/* argc= boot_argc; */
/* cur_arg= boot_cur_arg; */

mmix_boot();

/* mmix_load_file(*cur_arg); */
mmix_load_file("y2k.mmo");
/* mmix_commandline(argc,argv); */
breakpoint= 0;
if(interacting)mmix_interact();
while(true){
if(interrupt&&!breakpoint)breakpoint|= trace_bit,interacting= true,interrupt= false;
else
#ifdef MMIX_TRAP
if(!(inst_ptr.h&sign_bit)||show_operating_system)
#endif
{breakpoint= 0;
if(interacting)mmix_interact();
}
if(halted)break;
do
{if(!resuming)
mmix_fetch_instruction();
mmix_perform_instruction();
mmix_trace();
mmix_dynamic_trap();
if(resuming&&op!=RESUME)resuming= false;
}while(resuming||(!interrupt&&!breakpoint));
if(interact_after_break)
interacting= true,interact_after_break= false;
if(g[rQ].l&g[rK].l&RE_BIT)
{breakpoint|= trace_bit;
goto boot;
}
}
end_simulation:if(profiling)mmix_profile();
if(interacting||profiling||showing_stats)show_stats(true);
mmix_finalize();
return g[255].l;
}
#line 2918 "./mmixware/mmix-sim.w"
