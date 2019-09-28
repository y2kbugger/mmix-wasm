#ifndef LIBCONFIG_H
#define LIBCONFIG_H

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

/* define this if you need a local copy of mem_tetra ll */
#define MMIX_LOCAL_LL  mem_tetra *ll; /* current place in the simulated memory */

#define MMIX_STO(val,addr) (ll=mem_find(addr),ll->tet=(val).h,(ll+1)->tet=(val).l,true)
#define MMIX_STT(val,addr) (ll=mem_find(addr),ll->tet=(val).l,true)
#define MMIX_STW(val,addr) (ll=mem_find(addr),ll->tet=(ll->tet&(~(0xFFFF<<((1-((addr).l&0x1))<<4))))|(((val).l&0xFFFF)<<((1-((addr).l&0x1))<<4)),true)
#define MMIX_STB(val,addr) (ll=mem_find(addr),ll->tet=(ll->tet&(~(0xFF<<((3-((addr).l&0x3))<<3))))|(((val).l&0xFF)<<((3-((addr).l&0x3))<<3)),true)

#define MMIX_LDO(val,addr) (ll=mem_find(addr),(val).h=ll->tet,(val).l=(ll+1)->tet,true)
#define MMIX_LDT(val,addr) (ll=mem_find(addr),(val).h=0,(val).l=ll->tet,true)
#define MMIX_LDW(val,addr) (ll=mem_find(addr),(val).h=0,(val).l=(ll->tet>>(16*(1-((addr).l&1))))&0xFFFF,true)
#define MMIX_LDB(val,addr) (ll=mem_find(addr),(val).h=0,(val).l=(ll->tet>>(8*(3-((addr).l&3))))&0xFF,true)

#define MMIX_STO_UNCACHED(val,addr) MMIX_STO(val,addr) 
#define MMIX_LDO_UNCACHED(val,addr) MMIX_LDO(val,addr)


#define MMIX_FETCH(inst,loc) (ll=mem_find(loc),inst=ll->tet,true)
#define MMIX_STORE_IVTC(virt,phys)
#define MMIX_STORE_DVTC(virt,phys)

#ifdef WIN32
#define	MMIX_DELAY(ms,d)  (Sleep(ms), d=(ms))
#else
#define	MMIX_DELAY(ms,d)  (usleep(1000*(ms)), d=(ms))
#endif

/* define this to check for external asynchronous interrupts*/
#define MMIX_GET_INTERRUPT

/* this code is executed when MMIX enters the handler for Ctrl-C */
#define   MMIX_CTRL_HANDLER

/* this code defines additional options */
#define MMIX_OPTIONS  

/* if MMIX_BOOT is defined, mmis-sim will boot from addres #8000...0000
   otherwise it will resume at Main */

#undef MMIX_BOOT

#ifdef MMIX_PRINT
extern int mmix_printf(char *format,...);
extern int mmix_fputc(int c, FILE *f);
#define printf(...) mmix_printf(__VA_ARGS__)
#define fprintf(file,...) mmix_printf(__VA_ARGS__)
#define fputc(c,f) mmix_fputc(c,f)
#endif

#define MMIX_NO_FILE if (!*cur_arg) scan_option("?",true) /* exit with usage note */

/* define this to get the real TRAP implementation not the MMIXWARE fake TRAPS */
#undef MMIX_TRAP

/* this is the error display function */
#define MMIX_ERROR(f,m) fprintf(stderr,f,m)

/* define this if you need the tetra inside the mem_node */
#define MMIX_MEM_TET tetra tet; /* the tetrabyte of simulated memory */

/* these are the functions for the instructions not implemented in the basic mmix simulator */
#define MMIX_WRITE_DCACHE() 
#define MMIX_CLEAR_ICACHE() 
#define MMIX_CLEAR_DCACHE() 
#define MMIX_UPDATE_VTC(w) zero_octa 
#define MMIX_CLEAR_DVTC() 
#define MMIX_CLEAR_IVTC() 
#define MMIX_PRELOAD_DCACHE(w,xx) 
#define MMIX_PRELOAD_ICACHE(w,xx) 
#define MMIX_STORE_DCACHE(w,xx) 
#define MMIX_DELETE_DCACHE(w,xx) 
#define MMIX_DELETE_ICACHE(w,xx) 

#define MMXIAL_LINE_TRUNCATED     fprintf(stderr,"(say `-b <number>' to increase the length of my input buffer)\n");

/* MMIXAL has its own notion of file numbers as embedded in the mmo file
   we might want to convert this to a global file number */
#define MMIX_FILE_NO(file_no) (file_no)

/* define this to record file line and location associations while assembling */
#define MMIXAL_LINE_LOC(file_no,line_no,cur_loc)
/* define this to record file line and location associations while loading mmo files 
   undefine to get the default behaviour of storing file_no and line_no in the mem_tetra */
#undef MMIX_LOAD_LINE_LOC(file_no,line_no,cur_loc)
#endif