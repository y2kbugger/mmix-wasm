#include "emscripten.h"
#include "mmixlib/abstime.h"
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "mmixlib/libconfig.h"
#include "mmixlib/libtype.h"
#include "mmixlib/libglobals.h"
#include "mmixlib/mmixlib.h"

EMSCRIPTEN_KEEPALIVE
int mmixal_wasm() {
    return mmixal("y2k.mms", "y2k.mmo", "y2k.mml");
}

EMSCRIPTEN_KEEPALIVE bool get_halted() { return halted; }
EMSCRIPTEN_KEEPALIVE void set_halted(bool v) { halted = v; }

EMSCRIPTEN_KEEPALIVE int get_breakpoint() { return breakpoint; }
EMSCRIPTEN_KEEPALIVE void set_breakpoint(int v) { breakpoint = v; }

EMSCRIPTEN_KEEPALIVE bool get_interrupt() { return interrupt; }
EMSCRIPTEN_KEEPALIVE void set_interrupt(bool v) { interrupt = v; }

EMSCRIPTEN_KEEPALIVE bool get_resuming() { return resuming; }
EMSCRIPTEN_KEEPALIVE void set_resuming(bool v) { resuming = v; }

EMSCRIPTEN_KEEPALIVE bool get_interacting() { return interacting; }
EMSCRIPTEN_KEEPALIVE void set_interacting(bool v) { interacting = v; }

EMSCRIPTEN_KEEPALIVE bool get_interact_after_break() { return interact_after_break; }
EMSCRIPTEN_KEEPALIVE void set_interact_after_break(bool v) { interact_after_break = v; }

EMSCRIPTEN_KEEPALIVE bool get_profiling() { return profiling; }
EMSCRIPTEN_KEEPALIVE void set_profiling(bool v) { profiling = v; }

EMSCRIPTEN_KEEPALIVE bool get_showing_stats() { return showing_stats; }
EMSCRIPTEN_KEEPALIVE void set_showing_stats(bool v) { showing_stats = v; }

EMSCRIPTEN_KEEPALIVE
int get_trace_bit() {
    // this is actually a preprocessor builtin
    // defined bitmask (1<<3)
    // Therefor, no setter
    return trace_bit;
}

EMSCRIPTEN_KEEPALIVE
int print_mystr(char*mystr)
{
    printf("%s", mystr);
    return strlen(mystr);
}

EMSCRIPTEN_KEEPALIVE
void y2k_mmix_load_file(){
    mmix_load_file("y2k.mmo");
}

EMSCRIPTEN_KEEPALIVE
int get_general_register(int reg_number, bool high) {
    octa z;
    if(reg_number>=G) z=g[reg_number];
    else if(reg_number<L) z=l[(O+reg_number)&lring_mask];
    if (high)
    {
        return z.h;
    }else {
        return z.l;
    }
}

EMSCRIPTEN_KEEPALIVE
int mmix_sim()
{
    mmix_lib_initialize();
    /* g[255].h= 0; */
    /* g[255].l= 0; */
    mmix_initialize();
boot:
    mmix_boot();
    mmix_load_file("y2k.mmo");
    /* mmix_commandline(argc,argv); */

    /* set_breakpoint(0); */
    /* if(get_interacting()) */
    /*     mmix_interact(); */

    while(true){
        /* if(get_interrupt()&&!get_breakpoint()) */
        /* { */
        /*     set_breakpoint(breakpoint | trace_bit); */
        /*     set_interacting(true); */
        /*     set_interrupt(false); */
        /* } */
        /* else */
        /* { */
        /*     set_breakpoint(0); */
        /*     if(get_interacting()) */
        /*         mmix_interact(); */
        /* } */
        if(get_halted())
        {
            printf("zasd");
            break;
        }

        do
        {
            if(!get_resuming())
                mmix_fetch_instruction();
            mmix_perform_instruction();
            mmix_trace();
            mmix_dynamic_trap();
            if(get_resuming() && op != RESUME)
                set_resuming(false);
        }
        while(get_resuming()||(!get_interrupt()&&!get_breakpoint()));

        /* if(get_interact_after_break()) */
        /* { */
        /*     set_interacting(true); */
        /*     set_interact_after_break(false); */
        /* } */
        /* if(g[rQ].l&g[rK].l&RE_BIT) */
        /* { */
        /*     set_breakpoint(get_breakpoint() | get_trace_bit()); */
        /*     goto boot; */
        /* } */
    }
    end_simulation:
    if(get_profiling())
        mmix_profile();
    if(get_interacting()||get_profiling()||get_showing_stats())
        show_stats(true);
    show_stats(true);
    mmix_finalize();
    return g[255].l;
}
#line 2918 "./mmixware/mmix-sim.w"
