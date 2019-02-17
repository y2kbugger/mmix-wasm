#ifndef LIBCONFIG_H
#define LIBCONFIG_H

/* This this the libconfig.h file for MMIXGDB  rename to libconfig.h */

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

/* define this if you need a local copy of mem_tetra ll */
#define MMIX_LOCAL_LL  /* mem_tetra *ll; current place in the simulated memory */

#define MMIX_STO(val,addr) store_data(8,val,addr)
#define MMIX_STT(val,addr) store_data(4,val,addr)
#define MMIX_STW(val,addr) store_data(2,val,addr)
#define MMIX_STB(val,addr) store_data(1,val,addr)

#define MMIX_LDO(val,addr)  load_data(8,&(val),addr,0)
#define MMIX_LDT(val,addr)  load_data(4,&(val),addr,0)
#define MMIX_LDW(val,addr)  load_data(2,&(val),addr,0)
#define MMIX_LDB(val,addr)  load_data(1,&(val),addr,0)

#define MMIX_STO_UNCACHED(val,addr) store_data_uncached(8,val,addr) 
#define MMIX_LDO_UNCACHED(val,addr) load_data_uncached(8,&(val),addr,0)


#define MMIX_FETCH(inst,loc) load_instruction(&inst,loc)
#define MMIX_STORE_IVTC(virt,phys) store_exec_translation(&(virt),&(phys))
#define MMIX_STORE_DVTC(virt,phys) store_data_translation(&(virt),&(phys))

#ifdef WIN32
#define	MMIX_DELAY(ms,d)  ((d) = wait_time(ms))
#else
#define	MMIX_DELAY(ms,d)  (usleep(1000*(ms)), d=(ms))
#endif

/* define this to check for external asynchronous interrupts*/
#define MMIX_GET_INTERRUPT  get_interrupt(&new_Q)

/* this code is executed when MMIX enters the handler for Ctrl-C */
#define   MMIX_CTRL_HANDLER   if (interrupt)(cancel_wait(), show_operating_system=true)

/* this code defines additional command line options */
#define MMIX_OPTIONS  \
 case 'B': \
  { char *p; \
    p = strchr(arg+1,':'); \
    if (p==NULL) \
    { host=localhost; \
      port = atoi(arg+1); \
    }    \
    else \
    { port = atoi(p+1); \
      host = malloc(p+1-arg+1); \
      if (host==NULL) panic("No room for hostname"); \
      strncpy(host,arg+1,p-arg-1); \
      host[p-arg-1]=0; \
    } \
  } \
  return;  \


/* if MMIX_BOOT is defined, mmix-sim will boot from addres #8000...0000
   otherwise it will resume at Main */
#define MMIX_BOOT

/* if MMIX_PRINT is defined the mmixoutput is redirected from stdin or stderr */
#undef MMIX_PRINT

/* this action is executed when there is no mmo file on the command line */
#define MMIX_NO_FILE

/* define this to get the real TRAP implementation not the MMIXWARE fake TRAPS */
#define MMIX_TRAP

/* this is the error display function */
#define MMIX_ERROR(f,m) fprintf(stderr,f,m)

/* define this if you need the tetra inside the mem_node */
#undef MMIX_MEM_TET /* tetra tet; the tetrabyte of simulated memory */

/* these are the functions for the instructions not implemented in the basic mmix simulator */
#define MMIX_WRITE_DCACHE() write_all_data_cache()
#define MMIX_CLEAR_ICACHE() clear_all_instruction_cache()
#define MMIX_CLEAR_DCACHE() clear_all_data_cache()
#define MMIX_UPDATE_VTC(w) update_vtc(w)
#define MMIX_CLEAR_DVTC() clear_all_data_vtc()
#define MMIX_CLEAR_IVTC() clear_all_instruction_vtc()
#define MMIX_PRELOAD_DCACHE(w,xx) preload_data_cache(w,xx)
#define MMIX_PRELOAD_ICACHE(w,xx) prego_instruction(w,xx)
#define MMIX_STORE_DCACHE(w,xx) write_data(w,xx)
#define MMIX_DELETE_DCACHE(w,xx) delete_data(w,xx)
#define MMIX_DELETE_ICACHE(w,xx) delete_instruction(w,xx)

#define MMXIAL_LINE_TRUNCATED     fprintf(stderr,"(say `-b <number>' to increase the length of my input buffer)\n");

/* define this to record file line and location associations while assembling */
#define MMIXAL_LINE_LOC(file_no,line_no,cur_loc)
/* define this to record file line and location associations while loading mmo files 
   undefine to get the default behaviour of storing file_no and line_no in the mem_tetra */
#undef MMIX_LOAD_LINE_LOC

#endif
