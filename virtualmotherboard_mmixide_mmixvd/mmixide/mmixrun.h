
extern int mmix_active(void);
/* returns true if the mmix thread is running */
extern void mmix_run(void);
/* runs the mmix simulator */
extern void mmix_debug(void);
/* runs the mmix simulator in debug mode */

extern int mmix_assemble(int file_no);
/* runs the mmix assembler returns error count */

extern char *get_mmo_name(char *full_mms_name);

extern int mmix_connect(void);
/* connects to the vmb bus */

extern void mmix_stop(void);
/* stopps a running mmix process */

void mmix_stopped(octa loc);
/* called from the mmix thread if it stopps in mmix_interact */

extern void mmix_continue(unsigned char command);
/* called from the GUI to make the mmix thread continue */

extern void mmix_status(int status);
/* called from the GUI thread to display the status od the mmix thread */
/* definitions needed for mmix-sim */


extern void show_trace_window(void);

#define MMIX_DISCONNECTED 0
#define MMIX_CONNECTED    1
#define MMIX_OFF          2
#define MMIX_ON           3
#define MMIX_STOPPED      4
#define MMIX_RUNNING      5
#define MMIX_HALTED       6